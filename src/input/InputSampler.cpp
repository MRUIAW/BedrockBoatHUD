// SPDX-License-Identifier: CC0-1.0

#include "input/InputSampler.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "mc/client/input/ClientMoveInputHandler.h"
#include "mc/entity/components/MoveInputComponent.h"
#include "mc/input/MoveInputState.h"

namespace boat_hud::input {
namespace {

constexpr float AnalogDeadZone = 0.1f;

bool hasFlag(MoveInputState const& state, MoveInputState::Flag flag) noexcept {
    return state.mFlagValues->test(static_cast<std::size_t>(flag));
}

float lengthSquared(Vec2 const& value) noexcept { return value.x * value.x + value.y * value.y; }

} // namespace

InputState InputSampler::sample(IClientInstance& client) const noexcept {
    auto* movement = ClientMoveInputHandler::getMoveInput(client);
    if (movement == nullptr) return {};

    auto const& state       = *movement->mInputState;
    bool const  digitalLeft = hasFlag(state, MoveInputState::Flag::Left) || hasFlag(state, MoveInputState::Flag::UpLeft)
                           || hasFlag(state, MoveInputState::Flag::DownLeft);
    bool const  digitalRight  = hasFlag(state, MoveInputState::Flag::Right)
                             || hasFlag(state, MoveInputState::Flag::UpRight)
                             || hasFlag(state, MoveInputState::Flag::DownRight);
    bool const digitalForward = hasFlag(state, MoveInputState::Flag::Up) || hasFlag(state, MoveInputState::Flag::UpLeft)
                             || hasFlag(state, MoveInputState::Flag::UpRight);
    bool const digitalBackward = hasFlag(state, MoveInputState::Flag::Down)
                              || hasFlag(state, MoveInputState::Flag::DownLeft)
                              || hasFlag(state, MoveInputState::Flag::DownRight);

    auto const  stateAnalog   = *state.mAnalogMoveVector;
    auto const  componentMove = *movement->mMove;
    auto const  analog        = lengthSquared(stateAnalog) > lengthSquared(componentMove) ? stateAnalog : componentMove;
    float const analogSteering = std::clamp(-analog.x, -1.0f, 1.0f);
    float const analogThrottle = std::clamp(analog.y, -1.0f, 1.0f);

    // A movement vector may also represent the digital input in a different coordinate convention.
    // Direction flags take precedence per axis so the same input cannot activate the opposite icon.
    bool const hasDigitalSteering = digitalLeft || digitalRight;
    bool const hasDigitalThrottle = digitalForward || digitalBackward;

    InputState result;
    result.left     = hasDigitalSteering ? digitalLeft : analogSteering > AnalogDeadZone;
    result.right    = hasDigitalSteering ? digitalRight : analogSteering < -AnalogDeadZone;
    result.forward  = hasDigitalThrottle ? digitalForward : analogThrottle > AnalogDeadZone;
    result.backward = hasDigitalThrottle ? digitalBackward : analogThrottle < -AnalogDeadZone;

    result.steering = hasDigitalSteering ? (digitalLeft ? 1.0f : 0.0f) - (digitalRight ? 1.0f : 0.0f) : analogSteering;
    result.throttle = hasDigitalThrottle ? (digitalForward ? 1.0f : 0.0f) - (digitalBackward ? 0.125f : 0.0f)
                                         : (analogThrottle >= 0.0f ? analogThrottle : analogThrottle * 0.125f);
    return result;
}

} // namespace boat_hud::input
