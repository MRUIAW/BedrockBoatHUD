// SPDX-License-Identifier: CC0-1.0

#include "mod/BoatHudMod.h"

#include <memory>

#include "client/ClientController.h"
#include "config/BoatHudConfig.h"
#include "ll/api/i18n/I18n.h"
#include "ll/api/mod/RegisterHelper.h"

namespace boat_hud {

BoatHudMod& BoatHudMod::getInstance() {
    static BoatHudMod instance;
    return instance;
}

BoatHudMod::BoatHudMod() : mSelf(*ll::mod::NativeMod::current()) {}

ll::mod::NativeMod& BoatHudMod::getSelf() const noexcept { return mSelf; }

bool BoatHudMod::load() {
    mSelf.getLogger().info("Loading Bedrock BoatHUD");
    if (auto result = ll::i18n::getInstance().load(mSelf.getLangDir()); !result) {
        mSelf.getLogger().error("Bedrock BoatHUD could not load its language files");
        result.error().log(mSelf.getLogger());
    }

    mConfigService = std::make_unique<config::ConfigService>(mSelf);
    if (!mConfigService->load()) {
        mSelf.getLogger().error("Bedrock BoatHUD could not load its configuration");
        return false;
    }
    return true;
}

bool BoatHudMod::enable() {
    mSelf.getLogger().info("Enabling Bedrock BoatHUD");
    if (mConfigService == nullptr) {
        mSelf.getLogger().error("Configuration service is unavailable");
        return false;
    }

    mClientController = std::make_unique<client::ClientController>(mSelf, *mConfigService);
    if (!mClientController->enable()) {
        mClientController.reset();
        return false;
    }
    return true;
}

bool BoatHudMod::disable() {
    mSelf.getLogger().info("Disabling Bedrock BoatHUD");
    if (mClientController != nullptr) {
        mClientController->disable();
        mClientController.reset();
    }
    return true;
}

} // namespace boat_hud

LL_REGISTER_MOD(boat_hud::BoatHudMod, boat_hud::BoatHudMod::getInstance());
