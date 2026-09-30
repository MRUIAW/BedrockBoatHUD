// SPDX-License-Identifier: CC0-1.0

#include "telemetry/TelemetryWriter.h"

#include <chrono>
#include <ctime>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

#include "ll/api/mod/NativeMod.h"

namespace boat_hud::telemetry {
namespace {

std::string makeTimestamp() {
    auto const        now  = std::chrono::system_clock::now();
    std::time_t const time = std::chrono::system_clock::to_time_t(now);
    std::tm           localTime{};
    localtime_s(&localTime, &time);

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S");
    return stream.str();
}

} // namespace

TelemetryWriter::TelemetryWriter(ll::mod::NativeMod& mod) : mMod(mod) {}

TelemetryWriter::~TelemetryWriter() { stop(); }

bool TelemetryWriter::start() noexcept {
    stop();
    try {
        auto const directory = mMod.getDataDir() / "telemetry";
        std::filesystem::create_directories(directory);
        mCurrentPath = directory / (makeTimestamp() + ".csv");
        mStream.open(mCurrentPath, std::ios::out | std::ios::trunc);
        mStream.imbue(std::locale::classic());
        if (!mStream.is_open()) {
            mMod.getLogger().error("Failed to open telemetry file {}", mCurrentPath.string());
            return false;
        }

        mStream << "# bedrock_boathud_telemetry_version=1\n";
        mStream << "tick,time_seconds,speed_mps,longitudinal_acceleration_mps2,"
                   "lateral_acceleration_mps2,slip_angle_degrees,angular_velocity_dps,"
                   "steering,throttle,x,y,z,ping_ms,fps\n";
        mStream.flush();
        if (!mStream.good()) {
            mMod.getLogger().error("Failed to write telemetry header to {}", mCurrentPath.string());
            stop();
            return false;
        }
        mMod.getLogger().info("Telemetry started at {}", mCurrentPath.string());
        return true;
    } catch (std::exception const& exception) {
        mMod.getLogger().error("Failed to start telemetry: {}", exception.what());
    } catch (...) {
        mMod.getLogger().error("Failed to start telemetry because of an unknown error");
    }
    stop();
    return false;
}

bool TelemetryWriter::write(hud::HudSnapshot const& snapshot) noexcept {
    if (!mStream.is_open()) return false;

    try {
        mStream << snapshot.elapsedTicks << ',' << std::fixed << std::setprecision(3)
                << static_cast<double>(snapshot.elapsedTicks) / 20.0 << ',' << std::setprecision(6) << snapshot.speed
                << ',' << snapshot.longitudinalAcceleration << ',' << snapshot.lateralAcceleration << ','
                << snapshot.slipAngle << ',' << snapshot.angularVelocity << ',' << snapshot.input.steering << ','
                << snapshot.input.throttle << ',' << snapshot.positionX << ',' << snapshot.positionY << ','
                << snapshot.positionZ << ',' << snapshot.pingMilliseconds << ',' << snapshot.framesPerSecond << '\n';
        if (mStream.good()) return true;
        mMod.getLogger().error("Telemetry stream failed while writing {}", mCurrentPath.string());
    } catch (std::exception const& exception) {
        mMod.getLogger().error("Failed to write telemetry: {}", exception.what());
    } catch (...) {
        mMod.getLogger().error("Failed to write telemetry because of an unknown error");
    }
    stop();
    return false;
}

void TelemetryWriter::stop() noexcept {
    if (!mStream.is_open()) return;
    mStream.flush();
    mStream.close();
    mMod.getLogger().info("Telemetry stopped at {}", mCurrentPath.string());
}

bool TelemetryWriter::isActive() const noexcept { return mStream.is_open(); }

} // namespace boat_hud::telemetry
