// SPDX-License-Identifier: CC0-1.0

#include "client/ClientController.h"

#include <chrono>
#include <exception>
#include <string>
#include <string_view>

#include "config/BoatHudConfig.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/client/ClientExitLevelEvent.h"
#include "ll/api/event/client/ClientJoinLevelEvent.h"
#include "ll/api/event/input/KeyInputEvent.h"
#include "ll/api/event/input/MouseInputEvent.h"
#include "ll/api/event/render/UIRenderEvent.h"
#include "ll/api/event/world/ClientLevelTickEvent.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/service/TargetedBedrock.h"
#include "mc/client/game/IClientInstance.h"
#include "mc/client/gui/screens/ScreenView.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorCategory.h"
#include "mc/world/actor/ActorType.h"

namespace boat_hud::client {
namespace {

constexpr int DefaultToggleKey   = 0x42;
constexpr int DefaultLayoutKey   = 0x4E;
constexpr int DefaultCameraKey   = 0x43;
constexpr int DefaultSettingsKey = 0x4F;

bool isBoat(Actor const& actor) noexcept {
    if (actor.hasCategory(ActorCategory::Boat)) return true;
    if (actor.isType(ActorType::BoatRideable) || actor.isType(ActorType::ChestBoatRideable)) return true;

    std::string const& typeName = actor.getTypeName();
    return typeName.find("boat") != std::string::npos;
}

bool isHudScreen(std::string_view screenName) noexcept { return screenName.find("hud") != std::string_view::npos; }

} // namespace

ClientController::ClientController(ll::mod::NativeMod& mod, config::ConfigService& configService)
: mMod(mod),
  mConfigService(configService),
  mCheckpointManager(mod),
  mSettingsOverlay(configService),
  mTelemetryWriter(mod) {}

bool ClientController::enable() noexcept {
    try {
        auto& eventBus = ll::event::EventBus::getInstance();

        mListeners.emplace_back(eventBus.emplaceListener<ll::event::ClientJoinLevelEvent>([this](auto&) {
            endSession(nullptr);
        }));
        mListeners.emplace_back(eventBus.emplaceListener<ll::event::ClientExitLevelEvent>([this](auto& event) {
            mSettingsOverlay.close(event.self());
            endSession(&event.self());
        }));
        mListeners.emplace_back(eventBus.emplaceListener<ll::event::ClientLevelTickEvent>([this](auto&) { onTick(); }));
        mListeners.emplace_back(eventBus.emplaceListener<ll::event::AfterUIRenderEvent>([this](auto& event) {
            onUiRender(event.screenView().getScreenName(), event.uiRenderContext(), event.uiRenderContext().mClient);
        }));
        mListeners.emplace_back(eventBus.emplaceListener<ll::event::KeyInputEvent>([this](auto& event) {
            if (auto client = ll::service::getClientInstance()) onKeyInput(event, *client);
        }));
        mListeners.emplace_back(eventBus.emplaceListener<ll::event::MouseInputEvent>([this](auto& event) {
            if (auto client = ll::service::getClientInstance()) mSettingsOverlay.handleMouse(event, *client);
        }));
        for (auto const& listener : mListeners) {
            if (listener == nullptr) {
                mMod.getLogger().error("Failed to register a required client event listener");
                disable();
                return false;
            }
        }

        return true;
    } catch (std::exception const& exception) {
        mMod.getLogger().error("Failed to enable client controller: {}", exception.what());
    } catch (...) {
        mMod.getLogger().error("Failed to enable client controller because of an unknown error");
    }

    disable();
    return false;
}

bool ClientController::disable() noexcept {
    if (auto client = ll::service::getClientInstance()) {
        mSettingsOverlay.close(*client);
        endSession(client.as_ptr());
    }

    auto& eventBus = ll::event::EventBus::getInstance();
    for (auto const& listener : mListeners) {
        if (listener != nullptr) eventBus.removeListener(listener);
    }
    mListeners.clear();
    return true;
}

void ClientController::onTick() noexcept {
    auto client = ll::service::getClientInstance();
    if (!client) {
        endSession(nullptr);
        return;
    }

    Actor* boat = findDrivenBoat(*client);
    if (boat == nullptr) {
        endSession(client.as_ptr());
        return;
    }

    if (!mSession) {
        mSession.emplace(mConfigService.get().traceLengthTicks);
        mRenderer.reset();
        mTelemetryStartAttempted = false;
        if (mConfigService.get().checkpointsEnabled) {
            mCheckpointManager.load(mConfigService.get().checkpointFile);
        }
    }

    auto const& config = mConfigService.get();
    if (config.telemetryEnabled) {
        if (!mTelemetryWriter.isActive() && !mTelemetryStartAttempted) {
            mTelemetryStartAttempted = true;
            mTelemetryWriter.start();
        }
    } else {
        mTelemetryWriter.stop();
        mTelemetryStartAttempted = false;
    }

    auto const input = mInputSampler.sample(*client);
    auto const ping  = std::chrono::duration_cast<std::chrono::milliseconds>(client->getServerPingTime()).count();
    mSession->update(*boat, input, ping);
    if (config.checkpointsEnabled) {
        auto const result = mCheckpointManager.update(mSession->snapshot(), config.circularTrack);
        if (result) {
            mSession->setCheckpointResult(
                result->checkpointIndex,
                result->checkpointCount,
                result->timeDelta,
                result->speedDelta
            );
        }
    }
    if (mTelemetryWriter.isActive()) mTelemetryWriter.write(mSession->snapshot());
}

void ClientController::onKeyInput(ll::event::KeyInputEvent& event, IClientInstance& client) noexcept {
    int const  keyCode        = event.keyCode();
    bool const trackedKeyCode = keyCode >= 0 && keyCode < static_cast<int>(mPressedKeys.size());

    if (!event.isDown()) {
        if (trackedKeyCode) mPressedKeys[static_cast<std::size_t>(keyCode)] = false;
        if (mSettingsOverlay.isOpen()) mSettingsOverlay.handleKey(event, client);
        return;
    }

    if (mSettingsOverlay.isOpen()) {
        mSettingsOverlay.handleKey(event, client);
        return;
    }
    if (!client.isInWorldAndNotShowingAnyMenuScreens() || !trackedKeyCode) return;

    auto& pressed = mPressedKeys[static_cast<std::size_t>(keyCode)];
    if (pressed) return;
    pressed = true;

    switch (keyCode) {
    case DefaultToggleKey: {
        event.cancel();
        auto& config   = mConfigService.get();
        config.enabled = !config.enabled;
        if (!config.enabled) mVisibilityGuard.restore(client);
        mConfigService.save();
        break;
    }
    case DefaultLayoutKey: {
        event.cancel();
        auto& layout = mConfigService.get().layout;
        layout       = layout == "race" ? "classic" : (layout == "classic" ? "compact" : "race");
        mConfigService.save();
        mRenderer.reset();
        break;
    }
    case DefaultCameraKey: {
        event.cancel();
        auto& enabled = mConfigService.get().cameraAssistEnabled;
        enabled       = !enabled;
        if (!enabled) mCameraAssist.reset();
        mConfigService.save();
        break;
    }
    case DefaultSettingsKey:
        event.cancel();
        mVisibilityGuard.restore(client);
        mCameraAssist.reset();
        if (!mSettingsOverlay.open(client)) {
            mMod.getLogger().error("BoatHUD could not open its native settings screen");
        }
        break;
    default:
        break;
    }
}

void ClientController::onUiRender(
    std::string const&        screenName,
    MinecraftUIRenderContext& context,
    IClientInstance&          client
) noexcept {
    std::string const activeScreenName = client.getScreenName();

    bool const isGameplayView = screenName == activeScreenName || (activeScreenName.empty() && isHudScreen(screenName));
    if (mSettingsOverlay.isOpen()) {
        mSettingsOverlay.observeScreen(client);
        mVisibilityGuard.restore(client);
        mCameraAssist.reset();
        if (mSettingsOverlay.isOpen() && mSettingsOverlay.shouldRenderOn(screenName)) {
            mSettingsOverlay.render(context, client);
        }
        return;
    }

    auto const& config = mConfigService.get();
    if (!mSession.has_value() || !client.isInWorldAndNotShowingAnyMenuScreens()) {
        mVisibilityGuard.restore(client);
        mCameraAssist.reset();
        return;
    }

    if (!isGameplayView) return;

    if (config.cameraAssistEnabled) {
        if (auto* boat = findDrivenBoat(client)) {
            mCameraAssist.update(client, *boat, config);
        } else {
            mCameraAssist.reset();
        }
    } else {
        mCameraAssist.reset();
    }

    if (!config.enabled) {
        mVisibilityGuard.restore(client);
        return;
    }

    mVisibilityGuard.setHidden(client, config.hideVanillaHud);

    if (auto framesPerSecond = mRenderer.recordFrame()) {
        mSession->setFramesPerSecond(*framesPerSecond);
    }
    mRenderer.render(context, client, mSession->snapshot(), config);
}

Actor* ClientController::findDrivenBoat(IClientInstance& client) noexcept {
    auto* player = client.getLocalPlayer();
    if (player == nullptr) return nullptr;

    auto* vehicle = player->getVehicle();
    if (vehicle == nullptr || !isBoat(*vehicle) || vehicle->getFirstPassenger() != player) return nullptr;

    return vehicle;
}

void ClientController::endSession(IClientInstance* client) noexcept {
    if (client != nullptr) mVisibilityGuard.restore(*client);
    mCameraAssist.reset();
    if (!mSession) {
        mTelemetryStartAttempted = false;
        return;
    }

    mSession.reset();
    mCheckpointManager.reset();
    mTelemetryWriter.stop();
    mTelemetryStartAttempted = false;
    mRenderer.reset();
}

} // namespace boat_hud::client
