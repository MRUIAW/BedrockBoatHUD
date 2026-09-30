// SPDX-License-Identifier: CC0-1.0
#pragma once

#include <chrono>
#include <optional>

#include "config/BoatHudConfig.h"
#include "hud/HudSnapshot.h"

class Font;
class IClientInstance;
class MinecraftUIRenderContext;
class RectangleArea;
namespace mce {
class Color;
}
namespace ui {
enum class TextAlignment : int;
}

namespace boat_hud::hud {

/// Draws the configured BoatHUD layout with Bedrock's native UI renderer.
class HudRenderer {
public:
    /// Clears interpolation and frame-rate state when a new driving session begins.
    /// @throws Nothing.
    void reset() noexcept;

    /// Records one HUD frame and publishes a completed one-second frame count when available.
    /// @return New render FPS after a completed window, otherwise no value.
    /// @throws Nothing.
    [[nodiscard]] std::optional<int> recordFrame() noexcept;

    /// Draws one HUD frame.
    /// @param context Bedrock UI context for the current ScreenView.
    /// @param client Client used for viewport and font access.
    /// @param snapshot Most recent Tick-derived driving state.
    /// @param config Active validated configuration.
    /// @throws Nothing.
    void render(
        MinecraftUIRenderContext&    context,
        IClientInstance&             client,
        HudSnapshot const&           snapshot,
        config::BoatHudConfig const& config
    ) noexcept;

private:
    /// Updates the speed displayed by the renderer using frame-rate-independent smoothing.
    /// @param targetSpeed Latest raw speed in metres per second.
    /// @param config Active smoothing settings.
    /// @return Speed value used for this frame.
    [[nodiscard]] double updateDisplayedSpeed(double targetSpeed, config::BoatHudConfig const& config) noexcept;

    std::chrono::steady_clock::time_point mLastRenderTime{};
    std::chrono::steady_clock::time_point mFrameWindowStart{};
    double                                mDisplayedSpeed    = 0.0;
    int                                   mFramesInWindow    = 0;
    bool                                  mHasDisplayedSpeed = false;
};

} // namespace boat_hud::hud
