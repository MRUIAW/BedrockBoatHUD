// SPDX-License-Identifier: CC0-1.0
#pragma once

#include <memory>

#include "ll/api/mod/NativeMod.h"

namespace boat_hud::client {
class ClientController;
}
namespace boat_hud::config {
class ConfigService;
}

namespace boat_hud {

/// Owns the mod-wide configuration and client controller for Bedrock BoatHUD.
class BoatHudMod {
public:
    /// Returns the single native mod entry-point instance.
    /// @return Process-wide BoatHUD mod instance.
    static BoatHudMod& getInstance();

    /// Creates the mod entry point from LeviLamina's current native mod context.
    BoatHudMod();

    /// Returns the LeviLamina mod object used by all services.
    /// @return Owning native mod.
    [[nodiscard]] ll::mod::NativeMod& getSelf() const noexcept;

    /// Initializes paths and the versioned configuration.
    /// @return True when non-game initialization succeeds.
    bool load();

    /// Registers client callbacks and input bindings.
    /// @return True when BoatHUD can enter its active state.
    bool enable();

    /// Restores client state and removes every registered listener.
    /// @return True after cleanup completes.
    bool disable();

private:
    ll::mod::NativeMod&                       mSelf;
    std::unique_ptr<config::ConfigService>    mConfigService;
    std::unique_ptr<client::ClientController> mClientController;
};

} // namespace boat_hud
