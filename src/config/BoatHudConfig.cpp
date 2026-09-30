// SPDX-License-Identifier: CC0-1.0

#include "config/BoatHudConfig.h"

#include <algorithm>
#include <array>
#include <exception>
#include <filesystem>
#include <string_view>

#include "ll/api/Config.h"
#include "ll/api/mod/NativeMod.h"

namespace boat_hud::config {
namespace {

template <std::size_t Size>
bool isOneOf(std::string const& value, std::array<std::string_view, Size> const& supported) noexcept {
    return std::ranges::find(supported, value) != supported.end();
}

bool updateConfig(BoatHudConfig& config, nlohmann::ordered_json& data) {
    int const previousVersion = data.value("version", 0);
    if (!ll::config::defaultConfigUpdater(config, data)) return false;

    if (previousVersion < 2) data["hideVanillaHud"] = false;
    if (previousVersion < 3) {
        std::string const speedBar = data.value("speedBar", "packed");
        if (speedBar == "progressive" || speedBar == "custom") data["speedBar"] = "packed";
    }
    return true;
}

} // namespace

ConfigService::ConfigService(ll::mod::NativeMod& mod) : mMod(mod), mPath(mod.getConfigDir() / "config.json") {}

bool ConfigService::load() noexcept {
    try {
        std::filesystem::create_directories(mPath.parent_path());
        bool const currentVersion = ll::config::loadConfig(mConfig, mPath, updateConfig);
        validate();
        if (!currentVersion && !save()) {
            return false;
        }
        return true;
    } catch (std::exception const& exception) {
        mMod.getLogger().error("Failed to load configuration: {}", exception.what());
        return false;
    } catch (...) {
        mMod.getLogger().error("Failed to load configuration because of an unknown error");
        return false;
    }
}

bool ConfigService::save() noexcept {
    try {
        validate();
        if (!ll::config::saveConfig(mConfig, mPath)) {
            mMod.getLogger().error("Failed to write configuration to {}", mPath.string());
            return false;
        }
        return true;
    } catch (std::exception const& exception) {
        mMod.getLogger().error("Failed to save configuration: {}", exception.what());
        return false;
    } catch (...) {
        mMod.getLogger().error("Failed to save configuration because of an unknown error");
        return false;
    }
}

BoatHudConfig& ConfigService::get() noexcept { return mConfig; }

BoatHudConfig const& ConfigService::get() const noexcept { return mConfig; }

void ConfigService::validate() noexcept {
    static constexpr std::array<std::string_view, 3> Layouts{"race", "classic", "compact"};
    static constexpr std::array<std::string_view, 4> SpeedUnits{"ms", "kmh", "mph", "knots"};
    static constexpr std::array<std::string_view, 2> AccelerationUnits{"g", "mss"};
    static constexpr std::array<std::string_view, 3> SpeedBars{"packed", "mixed", "blue"};
    static constexpr std::array<std::string_view, 4> InputModes{"off", "icons", "trace", "icons_and_trace"};

    if (!isOneOf(mConfig.layout, Layouts)) mConfig.layout = "race";
    if (!isOneOf(mConfig.speedUnit, SpeedUnits)) mConfig.speedUnit = "kmh";
    if (!isOneOf(mConfig.accelerationUnit, AccelerationUnits)) mConfig.accelerationUnit = "g";
    if (!isOneOf(mConfig.speedBar, SpeedBars)) mConfig.speedBar = "packed";
    if (!isOneOf(mConfig.inputDisplayMode, InputModes)) mConfig.inputDisplayMode = "icons_and_trace";

    mConfig.speedSmoothingResponse = std::clamp(mConfig.speedSmoothingResponse, 1.0, 60.0);
    mConfig.offsetX                = std::clamp(mConfig.offsetX, -1000, 1000);
    mConfig.offsetY                = std::clamp(mConfig.offsetY, 0, 1000);
    mConfig.traceLengthTicks       = std::clamp(mConfig.traceLengthTicks, 20, 120);
    mConfig.cameraAggressiveness   = std::clamp(mConfig.cameraAggressiveness, 4.0, 70.0);
    mConfig.cameraSmoothing        = std::clamp(mConfig.cameraSmoothing, 0.0, 0.9);
    mConfig.cameraMinimumSpeed     = std::clamp(mConfig.cameraMinimumSpeed, 0.0, 10.0);
}

} // namespace boat_hud::config
