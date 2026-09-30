// SPDX-License-Identifier: CC0-1.0

#include "camera/CameraAssist.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "mc/client/game/IClientInstance.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/deps/core/math/Vec2.h"
#include "mc/world/actor/Actor.h"

namespace boat_hud::camera {
namespace {

constexpr float TicksPerSecond   = 20.0f;
constexpr float RadiansToDegrees = 180.0f / std::numbers::pi_v<float>;
constexpr float BoatYawOffset    = 90.0f;

} // namespace

void CameraAssist::reset() noexcept { mLastUpdateTime = {}; }

void CameraAssist::update(IClientInstance& client, Actor& boat, config::BoatHudConfig const& config) noexcept {
    if (!config.cameraAssistEnabled) return;

    auto* player = client.getLocalPlayer();
    if (player == nullptr) return;

    auto const  velocity               = boat.getVelocity();
    float const horizontalSpeedPerTick = std::hypot(velocity.x, velocity.z);
    float const speed                  = horizontalSpeedPerTick * TicksPerSecond;
    if (speed < static_cast<float>(config.cameraMinimumSpeed)) return;

    float const boatYaw         = normalizeAngle(boat.getRotation().y - BoatYawOffset);
    float const velocityYaw     = std::atan2(-velocity.x, velocity.z) * RadiansToDegrees;
    float const lookAheadWeight = std::clamp(speed / static_cast<float>(config.cameraAggressiveness), 0.0f, 1.0f);
    float const targetYaw       = angleLerp(boatYaw, velocityYaw, lookAheadWeight);
    auto const  playerRotation  = player->getRotation();

    auto const now          = std::chrono::steady_clock::now();
    float      frameSeconds = 1.0f / 60.0f;
    if (mLastUpdateTime.time_since_epoch().count() != 0) {
        frameSeconds = static_cast<float>(std::chrono::duration<double>(now - mLastUpdateTime).count());
        frameSeconds = std::clamp(frameSeconds, 0.0f, 0.1f);
    }
    mLastUpdateTime = now;

    float const tickRetention  = std::clamp(static_cast<float>(config.cameraSmoothing), 0.0f, 0.9f);
    float const frameRetention = tickRetention <= 0.0f ? 0.0f : std::pow(tickRetention, frameSeconds * TicksPerSecond);
    float const newYaw         = angleLerp(playerRotation.y, targetYaw, 1.0f - frameRetention);
    float const yawDelta       = normalizeAngle(newYaw - playerRotation.y);

    player->localPlayerTurn(Vec2{0.0f, yawDelta});
}

float CameraAssist::normalizeAngle(float angle) noexcept {
    float wrapped = std::fmod(angle + 180.0f, 360.0f);
    if (wrapped < 0.0f) wrapped += 360.0f;
    return wrapped - 180.0f;
}

float CameraAssist::angleLerp(float from, float to, float progress) noexcept {
    return from + normalizeAngle(to - from) * std::clamp(progress, 0.0f, 1.0f);
}

} // namespace boat_hud::camera
