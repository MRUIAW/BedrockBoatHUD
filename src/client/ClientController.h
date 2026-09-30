// SPDX-License-Identifier: CC0-1.0
#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "camera/CameraAssist.h"
#include "checkpoint/CheckpointManager.h"
#include "hud/HudRenderer.h"
#include "hud/HudVisibilityGuard.h"
#include "input/InputSampler.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/event/input/KeyInputEvent.h"
#include "session/BoatSession.h"
#include "settings/SettingsOverlay.h"
#include "telemetry/TelemetryWriter.h"

namespace ll::mod {
class NativeMod;
}
class Actor;
class IClientInstance;

namespace boat_hud::config {
class ConfigService;
}

namespace boat_hud::client {

/// Coordinates client events, boat-driving sessions, input sampling, and HUD rendering.
class ClientController {
public:
    /// Creates a controller bound to the owning mod and configuration service.
    /// @param mod Owning native mod used for logging and data paths.
    /// @param configService Validated configuration provider.
    ClientController(ll::mod::NativeMod& mod, config::ConfigService& configService);

    /// Registers client lifecycle, Tick, render, and key callbacks.
    /// @return True when every required listener was registered.
    /// @throws Nothing; registration errors are logged and converted to false.
    bool enable() noexcept;

    /// Removes callbacks, restores vanilla HUD elements, and destroys the active session.
    /// @return True after cleanup completes.
    /// @throws Nothing.
    bool disable() noexcept;

private:
    /// Handles one client-level Tick and updates the driving state machine.
    /// @throws Nothing.
    void onTick() noexcept;

    /// Handles BoatHUD shortcuts directly from the client keyboard event stream.
    /// @param event Cancellable LeviLamina keyboard event.
    /// @param client Active client instance used by the selected action.
    /// @throws Nothing.
    void onKeyInput(ll::event::KeyInputEvent& event, IClientInstance& client) noexcept;

    /// Handles one UI ScreenView after it has rendered.
    /// @param screenName Internal Bedrock screen name associated with the callback.
    /// @param context UI renderer for the current ScreenView.
    /// @param client Client associated with the UI renderer.
    /// @throws Nothing.
    void onUiRender(std::string const& screenName, MinecraftUIRenderContext& context, IClientInstance& client) noexcept;

    /// Returns the local player's boat when the player occupies the first passenger seat.
    /// @param client Client used to resolve the local player.
    /// @return Controlled boat, or nullptr when no driving session should be active.
    /// @throws Nothing.
    [[nodiscard]] Actor* findDrivenBoat(IClientInstance& client) noexcept;

    /// Writes a driving-state diagnostic only when the state changes.
    /// @param state Human-readable state for the current local player and vehicle.
    /// @throws Nothing.
    void reportDrivingState(std::string state) noexcept;

    /// Ends the current session and restores vanilla HUD state.
    /// @param client Optional client used to restore GuiData immediately.
    /// @throws Nothing.
    void endSession(IClientInstance* client) noexcept;

    ll::mod::NativeMod&                 mMod;
    config::ConfigService&              mConfigService;
    camera::CameraAssist                mCameraAssist;
    checkpoint::CheckpointManager       mCheckpointManager;
    input::InputSampler                 mInputSampler;
    hud::HudRenderer                    mRenderer;
    hud::HudVisibilityGuard             mVisibilityGuard;
    settings::SettingsOverlay           mSettingsOverlay;
    telemetry::TelemetryWriter          mTelemetryWriter;
    std::optional<session::BoatSession> mSession;
    std::vector<ll::event::ListenerPtr> mListeners;
    std::vector<std::string>            mObservedScreenNames;
    std::string                         mLastDrivingState;
    std::array<bool, 256>               mPressedKeys{};
    bool                                mTelemetryStartAttempted = false;
};

} // namespace boat_hud::client
