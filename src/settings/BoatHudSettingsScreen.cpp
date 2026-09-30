// SPDX-License-Identifier: CC0-1.0

#include "settings/BoatHudSettingsScreen.h"

#include "mc/client/gui/screens/ScreenContext.h"
#include "mc/deps/input/RectangleArea.h"

namespace boat_hud::settings {
BoatHudSettingsScreen::BoatHudSettingsScreen() {
    mWidth            = 0;
    mHeight           = 0;
    mShouldSendEvents = false;
    mWantsTextOnly    = false;
    mIsPopped         = false;
}

BoatHudSettingsScreen::~BoatHudSettingsScreen() = default;

void BoatHudSettingsScreen::setupForRendering(ScreenContext&) {}

void BoatHudSettingsScreen::cleanupForRendering(ScreenContext&) {}

void BoatHudSettingsScreen::preRenderUpdate(ScreenContext&) {}

void BoatHudSettingsScreen::prepareFrame(ScreenContext&) {}

void BoatHudSettingsScreen::postRenderUpdate(ScreenContext&) {}

void BoatHudSettingsScreen::render(ScreenContext&, FrameRenderObject const&) {}

bool BoatHudSettingsScreen::renderGameBehind() const { return true; }

bool BoatHudSettingsScreen::absorbsInput() const { return true; }

bool BoatHudSettingsScreen::isModal() const { return true; }

bool BoatHudSettingsScreen::isShowingMenu() const { return true; }

bool BoatHudSettingsScreen::shouldStealMouse() const { return false; }

bool BoatHudSettingsScreen::renderOnlyWhenTopMost() const { return true; }

EyeRenderingModeBit BoatHudSettingsScreen::getEyeRenderingMode() const { return static_cast<EyeRenderingModeBit>(1U); }

ui::SceneType BoatHudSettingsScreen::getSceneType() const { return ui::SceneType::SettingsScene; }

std::string BoatHudSettingsScreen::getScreenName() const { return std::string{Name}; }

bool BoatHudSettingsScreen::equalsScreenName(std::string_view comparison) const { return comparison == Name; }

bool BoatHudSettingsScreen::containsScreenNameSubstring(std::string_view substring) const {
    return Name.find(substring) != std::string_view::npos;
}

RectangleArea BoatHudSettingsScreen::getAreaOfControlByName(std::string const&) const { return {}; }

} // namespace boat_hud::settings
