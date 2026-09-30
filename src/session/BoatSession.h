// SPDX-License-Identifier: CC0-1.0
#pragma once

#include <cstddef>
#include <cstdint>

#include "hud/HudSnapshot.h"
#include "input/InputState.h"

class Actor;

namespace boat_hud::session {

/// Owns the Tick-based measurements and input history for one continuous driving session.
class BoatSession {
public:
    /// Starts an empty session and creates zero-filled input traces.
    /// @param traceLength Number of Tick samples retained by each trace.
    explicit BoatSession(int traceLength);

    /// Samples the boat and derives speed, acceleration, angular velocity, and slip angle.
    /// @param boat Boat currently controlled by the local player.
    /// @param input Effective movement input for this Tick.
    /// @param pingMilliseconds Current server ping in milliseconds.
    /// @throws Nothing.
    void update(Actor& boat, input::InputState const& input, std::int64_t pingMilliseconds) noexcept;

    /// Updates the render-rate value without changing Tick-derived data.
    /// @param framesPerSecond HUD render callbacks observed during the previous second.
    /// @throws Nothing.
    void setFramesPerSecond(int framesPerSecond) noexcept;

    /// Publishes the most recent checkpoint comparison to the renderer-facing snapshot.
    /// @param checkpointIndex One-based index of the crossed checkpoint.
    /// @param checkpointCount Total loaded checkpoints.
    /// @param timeDelta Signed time difference from the reference in seconds.
    /// @param speedDelta Signed speed difference from the reference in metres per second.
    /// @throws Nothing.
    void setCheckpointResult(
        std::size_t checkpointIndex,
        std::size_t checkpointCount,
        double      timeDelta,
        double      speedDelta
    ) noexcept;

    /// Returns the renderer-facing state of this session.
    /// @return Current HUD snapshot.
    [[nodiscard]] hud::HudSnapshot const& snapshot() const noexcept;

private:
    /// Wraps an angle to the inclusive-negative, exclusive-positive 180-degree interval.
    /// @param angle Angle in degrees.
    /// @return Equivalent angle in the range [-180, 180).
    [[nodiscard]] static double normalizeAngle(double angle) noexcept;

    /// Pushes a value into a fixed-length trace.
    /// @param trace Trace receiving the newest sample.
    /// @param value Newest input sample.
    /// @throws Nothing.
    static void pushTrace(std::deque<float>& trace, float value) noexcept;

    hud::HudSnapshot mSnapshot;
    bool             mHasPreviousSample   = false;
    double           mPreviousSpeed       = 0.0;
    double           mPreviousFacingAngle = 0.0;
    double           mPreviousTravelAngle = 0.0;
};

} // namespace boat_hud::session
