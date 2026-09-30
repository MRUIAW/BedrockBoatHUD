// SPDX-License-Identifier: CC0-1.0
#pragma once

#include <chrono>

#include "config/BoatHudConfig.h"

class Actor;
class IClientInstance;

namespace boat_hud::camera {

/// Applies Hibiii-style look-ahead by rotating the local player toward the boat's travel direction.
class CameraAssist {
public:
    /// Clears frame timing when camera assistance stops or a driving session ends.
    /// @throws Nothing.
    void reset() noexcept;

    /// Updates the local player's yaw for one rendered gameplay frame.
    /// @param client Client that owns the local player.
    /// @param boat Boat currently controlled by the local player.
    /// @param config Validated camera-assistance settings.
    /// @throws Nothing.
    void update(IClientInstance& client, Actor& boat, config::BoatHudConfig const& config) noexcept;

private:
    /// Wraps an angular delta to the range [-180, 180).
    /// @param angle Angle in degrees.
    /// @return Equivalent shortest angular delta.
    [[nodiscard]] static float normalizeAngle(float angle) noexcept;

    /// Interpolates across the shortest arc between two angles.
    /// @param from Starting angle in degrees.
    /// @param to Target angle in degrees.
    /// @param progress Clamped interpolation factor.
    /// @return Interpolated angle in degrees.
    [[nodiscard]] static float angleLerp(float from, float to, float progress) noexcept;

    std::chrono::steady_clock::time_point mLastUpdateTime{};
};

} // namespace boat_hud::camera
