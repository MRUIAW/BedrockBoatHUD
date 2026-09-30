// SPDX-License-Identifier: CC0-1.0
#pragma once

namespace boat_hud::input {

/// Represents the normalized movement controls sampled for one client Tick.
struct InputState {
    /// Steering uses positive values for left and negative values for right.
    float steering = 0.0f;

    /// Throttle uses positive values for forward and negative values for reverse.
    float throttle = 0.0f;

    /// Indicates that the player currently requests a left turn.
    bool left = false;

    /// Indicates that the player currently requests a right turn.
    bool right = false;

    /// Indicates that the player currently requests forward movement.
    bool forward = false;

    /// Indicates that the player currently requests reverse movement.
    bool backward = false;
};

} // namespace boat_hud::input
