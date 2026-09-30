// SPDX-License-Identifier: CC0-1.0
#pragma once

class IClientInstance;

namespace boat_hud::hud {

/// Applies BoatHUD's vanilla-HUD policy and guarantees a matching reset on every exit path.
class HudVisibilityGuard {
public:
    HudVisibilityGuard()                                     = default;
    HudVisibilityGuard(HudVisibilityGuard const&)            = delete;
    HudVisibilityGuard(HudVisibilityGuard&&)                 = delete;
    HudVisibilityGuard& operator=(HudVisibilityGuard const&) = delete;
    HudVisibilityGuard& operator=(HudVisibilityGuard&&)      = delete;

    /// Hides or restores vanilla HUD elements when the desired state changes.
    /// @param client Client whose GuiData owns the visibility state.
    /// @param hidden True to hide elements that overlap BoatHUD.
    /// @throws Nothing.
    void setHidden(IClientInstance& client, bool hidden) noexcept;

    /// Restores every element previously hidden by this guard.
    /// @param client Client whose GuiData owns the visibility state.
    /// @throws Nothing.
    void restore(IClientInstance& client) noexcept;

    /// Reports whether this guard currently owns a hidden state.
    /// @return True after a successful transition to hidden.
    [[nodiscard]] bool isHidden() const noexcept;

private:
    bool mHidden = false;
};

} // namespace boat_hud::hud
