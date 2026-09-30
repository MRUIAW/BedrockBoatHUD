// SPDX-License-Identifier: CC0-1.0

#include "hud/HudVisibilityGuard.h"

#include <vector>

#include "mc/client/game/IClientInstance.h"
#include "mc/client/gui/GuiData.h"
#include "mc/util/HudElement.h"
#include "mc/util/HudVisibility.h"

namespace boat_hud::hud {
namespace {

std::vector<HudElement> const HiddenElements{
    HudElement::Armor,
    HudElement::HotBar,
    HudElement::Health,
    HudElement::ProgressBar,
    HudElement::Hunger,
    HudElement::AirBubbles,
};

} // namespace

void HudVisibilityGuard::setHidden(IClientInstance& client, bool hidden) noexcept {
    if (hidden == mHidden) return;

    auto guiData = client.getGuiData();
    guiData->setHudVisibilityState(HiddenElements, hidden ? HudVisibility::Hide : HudVisibility::Reset);
    mHidden = hidden;
}

void HudVisibilityGuard::restore(IClientInstance& client) noexcept { setHidden(client, false); }

bool HudVisibilityGuard::isHidden() const noexcept { return mHidden; }

} // namespace boat_hud::hud
