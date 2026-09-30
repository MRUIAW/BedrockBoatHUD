// SPDX-License-Identifier: CC0-1.0
#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>

#include "input/InputState.h"

namespace boat_hud::hud {

/// Immutable-by-convention data consumed by the HUD renderer for the current driving session.
struct HudSnapshot {
    /// Horizontal boat speed in metres per second.
    double speed = 0.0;

    /// Change in speed along the path in metres per second squared.
    double longitudinalAcceleration = 0.0;

    /// Signed turning acceleration in metres per second squared.
    double lateralAcceleration = 0.0;

    /// Signed difference between boat heading and travel direction in degrees.
    double slipAngle = 0.0;

    /// Signed change in boat heading in degrees per second.
    double angularVelocity = 0.0;

    /// Boat X coordinate at the current sample.
    double positionX = 0.0;

    /// Boat Y coordinate at the current sample.
    double positionY = 0.0;

    /// Boat Z coordinate at the current sample.
    double positionZ = 0.0;

    /// Server round-trip latency in milliseconds.
    std::int64_t pingMilliseconds = 0;

    /// HUD render rate measured by the renderer.
    int framesPerSecond = 0;

    /// Number of elapsed client Tick samples in this session.
    std::uint64_t elapsedTicks = 0;

    /// Indicates that at least one valid checkpoint result is available.
    bool checkpointResultAvailable = false;

    /// One-based index of the most recently crossed checkpoint.
    std::size_t checkpointIndex = 0;

    /// Number of checkpoints loaded for the active course.
    std::size_t checkpointCount = 0;

    /// Signed difference from the reference checkpoint time in seconds.
    double checkpointTimeDelta = 0.0;

    /// Signed difference from the reference checkpoint speed in metres per second.
    double checkpointSpeedDelta = 0.0;

    /// Effective input sampled for the current Tick.
    input::InputState input;

    /// Oldest-to-newest steering history.
    std::deque<float> steeringTrace;

    /// Oldest-to-newest throttle history.
    std::deque<float> throttleTrace;
};

} // namespace boat_hud::hud
