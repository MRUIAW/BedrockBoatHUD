// SPDX-License-Identifier: CC0-1.0

#include "session/BoatSession.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "mc/world/actor/Actor.h"

namespace boat_hud::session {
namespace {

constexpr double TicksPerSecond      = 20.0;
constexpr double StationaryThreshold = 1.0e-6;
constexpr double RadiansToDegrees    = 180.0 / std::numbers::pi;
constexpr double DegreesToRadians    = std::numbers::pi / 180.0;
constexpr double BoatYawOffset       = 90.0;

} // namespace

BoatSession::BoatSession(int traceLength) {
    auto const safeLength = static_cast<std::size_t>(std::max(traceLength, 1));
    mSnapshot.steeringTrace.assign(safeLength, 0.0f);
    mSnapshot.throttleTrace.assign(safeLength, 0.0f);
}

void BoatSession::update(Actor& boat, input::InputState const& input, std::int64_t pingMilliseconds) noexcept {
    auto const   velocity               = boat.getVelocity();
    auto const   position               = boat.getPosition();
    double const horizontalSpeedPerTick = std::hypot(static_cast<double>(velocity.x), static_cast<double>(velocity.z));
    double const speed                  = horizontalSpeedPerTick * TicksPerSecond;
    double const facingAngle            = normalizeAngle(static_cast<double>(boat.getRotation().y) - BoatYawOffset);
    double const travelAngle =
        std::atan2(-static_cast<double>(velocity.x), static_cast<double>(velocity.z)) * RadiansToDegrees;

    mSnapshot.positionX        = position.x;
    mSnapshot.positionY        = position.y;
    mSnapshot.positionZ        = position.z;
    mSnapshot.speed            = speed;
    mSnapshot.input            = input;
    mSnapshot.pingMilliseconds = pingMilliseconds;

    if (mHasPreviousSample) {
        mSnapshot.longitudinalAcceleration = (speed - mPreviousSpeed) * TicksPerSecond;
        mSnapshot.angularVelocity          = normalizeAngle(facingAngle - mPreviousFacingAngle) * TicksPerSecond;

        double const travelDelta      = (travelAngle - mPreviousTravelAngle) * DegreesToRadians;
        mSnapshot.lateralAcceleration = std::sin(travelDelta / 2.0) * mPreviousSpeed * 2.0 * TicksPerSecond;
    } else {
        mSnapshot.longitudinalAcceleration = 0.0;
        mSnapshot.lateralAcceleration      = 0.0;
        mSnapshot.angularVelocity          = 0.0;
        mHasPreviousSample                 = true;
    }

    mSnapshot.slipAngle =
        horizontalSpeedPerTick <= StationaryThreshold ? 0.0 : normalizeAngle(facingAngle - travelAngle);
    ++mSnapshot.elapsedTicks;
    pushTrace(mSnapshot.steeringTrace, input.steering);
    pushTrace(mSnapshot.throttleTrace, input.throttle);

    mPreviousSpeed       = speed;
    mPreviousFacingAngle = facingAngle;
    mPreviousTravelAngle = travelAngle;
}

void BoatSession::setFramesPerSecond(int framesPerSecond) noexcept {
    mSnapshot.framesPerSecond = std::max(framesPerSecond, 0);
}

void BoatSession::setCheckpointResult(
    std::size_t checkpointIndex,
    std::size_t checkpointCount,
    double      timeDelta,
    double      speedDelta
) noexcept {
    mSnapshot.checkpointResultAvailable = true;
    mSnapshot.checkpointIndex           = checkpointIndex;
    mSnapshot.checkpointCount           = checkpointCount;
    mSnapshot.checkpointTimeDelta       = timeDelta;
    mSnapshot.checkpointSpeedDelta      = speedDelta;
}

hud::HudSnapshot const& BoatSession::snapshot() const noexcept { return mSnapshot; }

double BoatSession::normalizeAngle(double angle) noexcept {
    double wrapped = std::fmod(angle + 180.0, 360.0);
    if (wrapped < 0.0) wrapped += 360.0;
    return wrapped - 180.0;
}

void BoatSession::pushTrace(std::deque<float>& trace, float value) noexcept {
    if (trace.empty()) return;
    trace.pop_front();
    trace.push_back(value);
}

} // namespace boat_hud::session
