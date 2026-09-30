// SPDX-License-Identifier: CC0-1.0
#pragma once

#include <string>
#include <string_view>

#include "mc/client/gui/screens/AbstractScreenSetupCleanupStrategy.h"
#include "mc/client/gui/screens/BaseScreen.h"

class RectangleArea;
class ScreenContext;
struct FrameRenderObject;

namespace boat_hud::settings {

/// Minimal native scene-stack entry that gives the custom BoatHUD menu proper mouse ownership.
class BoatHudSettingsScreen final : public BaseScreen {
public:
    static constexpr std::string_view Name = "boat_hud_settings_screen";

    BoatHudSettingsScreen();
    ~BoatHudSettingsScreen() override;

    void setupForRendering(ScreenContext& screenContext) override;
    void cleanupForRendering(ScreenContext& screenContext) override;
    void preRenderUpdate(ScreenContext& screenContext) override;
    void prepareFrame(ScreenContext& screenContext) override;
    void postRenderUpdate(ScreenContext& screenContext) override;
    void render(ScreenContext& screenContext, FrameRenderObject const& renderObject) override;

    [[nodiscard]] bool                renderGameBehind() const override;
    [[nodiscard]] bool                absorbsInput() const override;
    [[nodiscard]] bool                isModal() const override;
    [[nodiscard]] bool                isShowingMenu() const override;
    [[nodiscard]] bool                shouldStealMouse() const override;
    [[nodiscard]] bool                renderOnlyWhenTopMost() const override;
    [[nodiscard]] EyeRenderingModeBit getEyeRenderingMode() const override;
    [[nodiscard]] ui::SceneType       getSceneType() const override;
    [[nodiscard]] std::string         getScreenName() const override;
    [[nodiscard]] bool                equalsScreenName(std::string_view comparison) const override;
    [[nodiscard]] bool                containsScreenNameSubstring(std::string_view substring) const override;
    [[nodiscard]] RectangleArea       getAreaOfControlByName(std::string const& controlName) const override;
};

} // namespace boat_hud::settings
