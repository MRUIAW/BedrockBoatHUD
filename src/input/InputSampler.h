// SPDX-License-Identifier: CC0-1.0
#pragma once

#include "input/InputState.h"

class IClientInstance;

namespace boat_hud::input {

/// Reads the effective Bedrock movement state so remapped keys and controllers remain supported.
class InputSampler {
public:
    /// Samples the current movement input of the local client.
    /// @param client Client instance that owns the movement component.
    /// @return Normalized movement input, or a neutral state when input is unavailable.
    /// @throws Nothing.
    [[nodiscard]] InputState sample(IClientInstance& client) const noexcept;
};

} // namespace boat_hud::input
