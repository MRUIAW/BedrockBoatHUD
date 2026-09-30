// SPDX-License-Identifier: CC0-1.0
#pragma once

#include <cstddef>
#include <string_view>

#include "ll/api/event/input/KeyInputEvent.h"
#include "ll/api/event/input/MouseInputEvent.h"
#include "localization/Localization.h"

class IClientInstance;
class MinecraftUIRenderContext;

namespace boat_hud::config {
class ConfigService;
}

namespace boat_hud::settings {

/// Renders and controls the BoatHUD-styled configuration screen.
class SettingsOverlay {
public:
    /// Creates a settings screen backed by the persistent BoatHUD configuration.
    /// @param configService Configuration provider updated by menu interactions.
    explicit SettingsOverlay(config::ConfigService& configService);

    /// Pushes the native mouse-owning settings scene.
    /// @param client Active client instance.
    /// @return True when the scene was queued successfully.
    /// @throws Nothing.
    [[nodiscard]] bool open(IClientInstance& client) noexcept;

    /// Saves configuration and pops the native settings scene.
    /// @param client Active client instance.
    /// @throws Nothing.
    void close(IClientInstance& client) noexcept;

    /// Reports whether the settings screen currently owns input.
    /// @return True while the settings screen is visible or queued.
    [[nodiscard]] bool isOpen() const noexcept;

    /// Reconciles the overlay state if Minecraft closes its scene externally.
    /// @param client Active client instance.
    /// @throws Nothing.
    void observeScreen(IClientInstance& client) noexcept;

    /// Processes and consumes a keyboard event while the settings screen is open.
    /// @param event Cancellable LeviLamina keyboard event.
    /// @param client Active client instance.
    /// @throws Nothing.
    void handleKey(ll::event::KeyInputEvent& event, IClientInstance& client) noexcept;

    /// Processes and consumes a mouse event while the settings screen is open.
    /// @param event Cancellable LeviLamina mouse event.
    /// @param client Active client instance used for coordinate conversion.
    /// @throws Nothing.
    void handleMouse(ll::event::MouseInputEvent& event, IClientInstance& client) noexcept;

    /// Draws the BoatHUD-styled settings panel over the gameplay view.
    /// @param context Bedrock UI renderer.
    /// @param client Active client instance used for UI dimensions.
    /// @throws Nothing.
    void render(MinecraftUIRenderContext& context, IClientInstance& client) noexcept;

    /// Tests whether a UI render callback belongs to the gameplay HUD behind this screen.
    /// @param screenName Internal Bedrock screen name.
    /// @return True when the settings overlay should render on this callback.
    [[nodiscard]] bool shouldRenderOn(std::string_view screenName) const noexcept;

private:
    enum class Row : std::size_t {
        HudEnabled,
        Layout,
        SpeedUnit,
        AccelerationUnit,
        SpeedBar,
        InputDisplay,
        ShowPing,
        ShowFps,
        HideVanillaHud,
        SmoothSpeed,
        CameraAssist,
        Telemetry,
        Checkpoints,
        OffsetX,
        OffsetY,
        Done,
        Count,
    };

    /// Changes the selected setting in the requested direction.
    /// @param direction Negative for the previous value and positive for the next value.
    /// @param client Active client used when the Done row closes the screen.
    /// @throws Nothing.
    void adjustSelected(int direction, IClientInstance& client) noexcept;

    /// Converts a raw mouse location into the logical UI coordinate system.
    /// @param client Active client whose screen size data is used.
    /// @param rawX Raw client-space X coordinate.
    /// @param rawY Raw client-space Y coordinate.
    /// @throws Nothing.
    void updatePointer(IClientInstance& client, short rawX, short rawY) noexcept;

    /// Moves the tracked pointer by a raw client-space mouse delta.
    /// @param client Active client whose screen size data is used.
    /// @param deltaX Raw client-space horizontal delta.
    /// @param deltaY Raw client-space vertical delta.
    /// @throws Nothing.
    void movePointer(IClientInstance& client, short deltaX, short deltaY) noexcept;

    /// Selects the settings row currently underneath the native pointer.
    /// @param client Active client whose logical viewport locates the panel.
    /// @throws Nothing.
    void selectPointerRow(IClientInstance& client) noexcept;

    config::ConfigService& mConfigService;
    bool                   mOpen        = false;
    bool                   mScreenSeen  = false;
    std::size_t            mSelectedRow = 0;
    float                  mPointerX    = 0.0f;
    float                  mPointerY    = 0.0f;
    localization::Language mLanguage    = localization::Language::English;
};

} // namespace boat_hud::settings
