// SPDX-License-Identifier: CC0-1.0
#pragma once

#include <filesystem>
#include <string>

namespace ll::mod {
class NativeMod;
}

namespace boat_hud::config {

/// Stores the user-facing settings serialized to config.json.
struct BoatHudConfig {
    /// Configuration schema version used by LeviLamina's migration support.
    int version = 3;

    /// Controls whether the HUD is rendered while the local player drives a boat.
    bool enabled = true;

    /// Selects the fixed layout preset: race, classic, or compact.
    std::string layout = "race";

    /// Selects the displayed speed unit: ms, kmh, mph, or knots.
    std::string speedUnit = "kmh";

    /// Selects the displayed acceleration unit: g or mss.
    std::string accelerationUnit = "g";

    /// Selects the BoatHUD speed bar profile: packed, mixed, or blue.
    std::string speedBar = "packed";

    /// Selects the input presentation: off, icons, trace, or icons_and_trace.
    std::string inputDisplayMode = "icons_and_trace";

    /// Enables a numeric ping readout next to the connection indicator.
    bool showPingNumber = true;

    /// Enables the HUD render-rate readout.
    bool showFps = true;

    /// Enables the local player name in layouts with enough room.
    bool showPlayerName = false;

    /// Hides vanilla HUD elements that overlap the Race layout.
    bool hideVanillaHud = false;

    /// Enables frame-rate-independent smoothing for the displayed speed.
    bool smoothDisplayedSpeed = true;

    /// Controls how quickly the displayed speed converges to the sampled speed.
    double speedSmoothingResponse = 12.0;

    /// Moves the HUD horizontally from its selected anchor in GUI pixels.
    int offsetX = 0;

    /// Moves the HUD upward from the bottom of the viewport in GUI pixels.
    int offsetY = 36;

    /// Controls the number of Tick samples retained for each input trace.
    int traceLengthTicks = 40;

    /// Enables Hibiii-style automatic look-ahead while driving.
    bool cameraAssistEnabled = false;

    /// Defines the speed in metres per second at which look-ahead reaches full strength.
    double cameraAggressiveness = 60.0;

    /// Defines the retained player-look fraction used to smooth camera assistance.
    double cameraSmoothing = 0.45;

    /// Prevents camera assistance below this horizontal speed in metres per second.
    double cameraMinimumSpeed = 0.5;

    /// Enables writing a CSV sample stream for each boat-driving session.
    bool telemetryEnabled = false;

    /// Enables external checkpoint timing data.
    bool checkpointsEnabled = false;

    /// Selects the checkpoint CSV file, relative to the mod configuration directory unless absolute.
    std::string checkpointFile = "checkpoints.csv";

    /// Re-enters the first checkpoint after the final checkpoint is crossed.
    bool circularTrack = false;
};

/// Loads, validates, and persists the mod's versioned JSON configuration.
class ConfigService {
public:
    /// Creates a configuration service rooted in the mod's configuration directory.
    /// @param mod Owning native mod whose paths and logger are used.
    explicit ConfigService(ll::mod::NativeMod& mod);

    /// Loads config.json, creates defaults when absent, and validates every value.
    /// @return True when a usable configuration is available.
    /// @throws Nothing; parse and I/O errors are logged and converted to false.
    bool load() noexcept;

    /// Saves the current in-memory configuration.
    /// @return True when config.json was written successfully.
    /// @throws Nothing; I/O errors are logged and converted to false.
    bool save() noexcept;

    /// Returns the mutable configuration used by commands and key handlers.
    /// @return Mutable configuration reference.
    [[nodiscard]] BoatHudConfig& get() noexcept;

    /// Returns the active configuration.
    /// @return Constant configuration reference.
    [[nodiscard]] BoatHudConfig const& get() const noexcept;

private:
    /// Replaces invalid or unsafe configuration values with supported values.
    void validate() noexcept;

    ll::mod::NativeMod&   mMod;
    std::filesystem::path mPath;
    BoatHudConfig         mConfig;
};

} // namespace boat_hud::config
