// SPDX-License-Identifier: CC0-1.0

#include "settings/SettingsOverlay.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "config/BoatHudConfig.h"
#include "fmt/format.h"
#include "ll/api/event/input/KeyInputEvent.h"
#include "ll/api/event/input/MouseInputEvent.h"
#include "localization/Localization.h"
#include "mc/client/game/IClientInstance.h"
#include "mc/client/gui/CaretMeasureData.h"
#include "mc/client/gui/FontHandle.h"
#include "mc/client/gui/GuiData.h"
#include "mc/client/gui/TextAlignment.h"
#include "mc/client/gui/TextMeasureData.h"
#include "mc/client/gui/screens/interfaces/ISceneStack.h"
#include "mc/client/renderer/screen/MinecraftUIRenderContext.h"
#include "mc/deps/core/math/Color.h"
#include "mc/deps/input/MouseAction.h"
#include "mc/deps/input/RectangleArea.h"
#include "settings/BoatHudSettingsScreen.h"

namespace boat_hud::settings {
namespace {

constexpr int MenuKey   = 0x4F;
constexpr int EscapeKey = 0x1B;
constexpr int EnterKey  = 0x0D;
constexpr int SpaceKey  = 0x20;
constexpr int LeftKey   = 0x25;
constexpr int UpKey     = 0x26;
constexpr int RightKey  = 0x27;
constexpr int DownKey   = 0x28;

constexpr float PanelWidth  = 300.0f;
constexpr float PanelHeight = 236.0f;
constexpr float RowHeight   = 12.0f;
constexpr float RowsTop     = 25.0f;
constexpr float TextHeight  = 10.0f;

mce::Color const BackdropColor{0, 0, 0};
mce::Color const PanelColor{24, 24, 24};
mce::Color const BorderColor{160, 160, 160};
mce::Color const RowColor{45, 45, 45};
mce::Color const SelectedColor{49, 137, 211};
mce::Color const TextColor{255, 255, 255};
mce::Color const MutedTextColor{175, 175, 175};
mce::Color const EnabledColor{85, 255, 85};
mce::Color const DisabledColor{255, 110, 110};

RectangleArea makeRectangle(float x, float y, float width, float height) noexcept {
    return {x, x + width, y, y + height};
}

void fill(
    MinecraftUIRenderContext& context,
    float                     x,
    float                     y,
    float                     width,
    float                     height,
    mce::Color const&         color,
    float                     alpha = 1.0f
) noexcept {
    context.fillRectangle(makeRectangle(x, y, width, height), color, alpha);
}

void drawLabel(
    MinecraftUIRenderContext& context,
    Font&                     font,
    std::string               text,
    RectangleArea const&      area,
    ui::TextAlignment         alignment,
    mce::Color const&         color = TextColor
) noexcept {
    TextMeasureData const  textData{1.0f, 0.0f, true, false, false, alignment};
    CaretMeasureData const caretData{-1, false};
    context.drawText(font, area, std::move(text), color, 1.0f, alignment, textData, caretData);
}

std::string_view enabledText(bool enabled, localization::Language language) noexcept {
    return localization::get(language, enabled ? localization::Text::Enabled : localization::Text::Disabled);
}

mce::Color const& enabledColor(bool enabled) noexcept { return enabled ? EnabledColor : DisabledColor; }

template <std::size_t Size>
void cycleValue(std::string& value, std::array<std::string_view, Size> const& values, int direction) {
    auto const  found = std::ranges::find(values, value);
    std::size_t index = found == values.end() ? 0 : static_cast<std::size_t>(std::distance(values.begin(), found));
    if (direction < 0) {
        index = index == 0 ? Size - 1 : index - 1;
    } else {
        index = (index + 1) % Size;
    }
    value = values[index];
}

} // namespace

SettingsOverlay::SettingsOverlay(config::ConfigService& configService) : mConfigService(configService) {}

bool SettingsOverlay::open(IClientInstance& client) noexcept {
    if (mOpen || !client.isInWorldAndNotShowingAnyMenuScreens()) return false;

    try {
        auto const  viewport = *client.getGuiData()->mScreenSizeData->clientUIScreenSize;
        float const panelY   = std::max((viewport.y - PanelHeight) / 2.0f, 2.0f);
        auto        screen   = std::make_shared<BoatHudSettingsScreen>();

        mOpen        = true;
        mScreenSeen  = false;
        mSelectedRow = 0;
        mPointerX    = viewport.x / 2.0f;
        mPointerY    = panelY + RowsTop + RowHeight / 2.0f;
        mLanguage    = localization::detectLanguage(client);
        client.getCurrentSceneStack()->pushScreen(std::move(screen), false);
        return true;
    } catch (...) {
        mOpen       = false;
        mScreenSeen = false;
        return false;
    }
}

void SettingsOverlay::close(IClientInstance& client) noexcept {
    if (!mOpen) return;
    mOpen       = false;
    mScreenSeen = false;
    mConfigService.save();

    auto stack = client.getCurrentSceneStack();
    if (stack->isOnSceneStack(std::string{BoatHudSettingsScreen::Name})) stack->schedulePopScreen(1);
}

bool SettingsOverlay::isOpen() const noexcept { return mOpen; }

void SettingsOverlay::observeScreen(IClientInstance& client) noexcept {
    if (!mOpen) return;

    bool const onStack = client.getCurrentSceneStack()->isOnSceneStack(std::string{BoatHudSettingsScreen::Name});
    if (onStack) {
        mScreenSeen = true;
    } else if (mScreenSeen) {
        mOpen       = false;
        mScreenSeen = false;
        mConfigService.save();
    }
}

void SettingsOverlay::handleKey(ll::event::KeyInputEvent& event, IClientInstance& client) noexcept {
    if (!mOpen) return;
    event.cancel();
    if (!event.isDown()) return;

    switch (event.keyCode()) {
    case EscapeKey:
    case MenuKey:
        close(client);
        break;
    case UpKey:
        mSelectedRow = mSelectedRow == 0 ? static_cast<std::size_t>(Row::Count) - 1 : mSelectedRow - 1;
        break;
    case DownKey:
        mSelectedRow = (mSelectedRow + 1) % static_cast<std::size_t>(Row::Count);
        break;
    case LeftKey:
        adjustSelected(-1, client);
        break;
    case RightKey:
    case EnterKey:
    case SpaceKey:
        adjustSelected(1, client);
        break;
    default:
        break;
    }
}

void SettingsOverlay::handleMouse(ll::event::MouseInputEvent& event, IClientInstance& client) noexcept {
    if (!mOpen) return;
    event.cancel();

    if (event.actionButtonId() == MouseAction::ActionMove) {
        updatePointer(client, event.x(), event.y());
        selectPointerRow(client);
        return;
    }
    if (event.actionButtonId() == MouseAction::ActionMoveRelative) {
        movePointer(client, event.dx(), event.dy());
        selectPointerRow(client);
        return;
    }

    if (event.buttonData() != MouseAction::DataDown) return;
    bool const leftClick  = event.actionButtonId() == MouseAction::ActionLeft;
    bool const rightClick = event.actionButtonId() == MouseAction::ActionRight;
    if (!leftClick && !rightClick) return;

    auto const  viewport = *client.getGuiData()->mScreenSizeData->clientUIScreenSize;
    float const panelX   = (viewport.x - PanelWidth) / 2.0f;
    float const panelY   = std::max((viewport.y - PanelHeight) / 2.0f, 2.0f);
    float const rowsY    = panelY + RowsTop;
    if (mPointerX < panelX + 8.0f || mPointerX > panelX + PanelWidth - 8.0f || mPointerY < rowsY) return;

    std::size_t const row = static_cast<std::size_t>((mPointerY - rowsY) / RowHeight);
    if (row >= static_cast<std::size_t>(Row::Count)) return;
    mSelectedRow = row;
    adjustSelected(rightClick ? -1 : 1, client);
}

void SettingsOverlay::render(MinecraftUIRenderContext& context, IClientInstance& client) noexcept {
    if (!mOpen) return;

    auto const  viewport = *client.getGuiData()->mScreenSizeData->clientUIScreenSize;
    float const panelX   = (viewport.x - PanelWidth) / 2.0f;
    float const panelY   = std::max((viewport.y - PanelHeight) / 2.0f, 2.0f);
    auto const& config   = mConfigService.get();

    auto  fontHandle = client.getFontHandle();
    auto& font       = fontHandle.getFont();

    fill(context, 0.0f, 0.0f, viewport.x, viewport.y, BackdropColor, 0.58f);
    fill(context, panelX, panelY, PanelWidth, PanelHeight, PanelColor, 0.97f);
    context.drawRectangle(makeRectangle(panelX, panelY, PanelWidth, PanelHeight), BorderColor, 1.0f, 1);
    drawLabel(
        context,
        font,
        std::string{localization::get(mLanguage, localization::Text::SettingsTitle)},
        makeRectangle(panelX + 8.0f, panelY + 7.0f, PanelWidth - 16.0f, TextHeight),
        ui::TextAlignment::Center
    );

    std::array<std::string, static_cast<std::size_t>(Row::Count)> const labels{
        std::string{localization::get(mLanguage, localization::Text::HudEnabled)},
        std::string{localization::get(mLanguage, localization::Text::Layout)},
        std::string{localization::get(mLanguage, localization::Text::SpeedUnit)},
        std::string{localization::get(mLanguage, localization::Text::AccelerationUnit)},
        std::string{localization::get(mLanguage, localization::Text::SpeedBar)},
        std::string{localization::get(mLanguage, localization::Text::InputDisplay)},
        std::string{localization::get(mLanguage, localization::Text::NumericPing)},
        std::string{localization::get(mLanguage, localization::Text::RenderFps)},
        std::string{localization::get(mLanguage, localization::Text::HideVanillaHud)},
        std::string{localization::get(mLanguage, localization::Text::SmoothSpeed)},
        std::string{localization::get(mLanguage, localization::Text::CameraAssist)},
        std::string{localization::get(mLanguage, localization::Text::TelemetryCsv)},
        std::string{localization::get(mLanguage, localization::Text::Checkpoints)},
        std::string{localization::get(mLanguage, localization::Text::HorizontalOffset)},
        std::string{localization::get(mLanguage, localization::Text::BottomOffset)},
        std::string{localization::get(mLanguage, localization::Text::Done)},
    };
    std::array<std::string, static_cast<std::size_t>(Row::Count)> const values{
        std::string{enabledText(config.enabled, mLanguage)},
        std::string{localization::layoutName(mLanguage, config.layout)},
        std::string{localization::speedUnitName(mLanguage, config.speedUnit)},
        std::string{localization::accelerationUnitName(mLanguage, config.accelerationUnit)},
        std::string{localization::speedBarName(mLanguage, config.speedBar)},
        std::string{localization::inputDisplayName(mLanguage, config.inputDisplayMode)},
        std::string{enabledText(config.showPingNumber, mLanguage)},
        std::string{enabledText(config.showFps, mLanguage)},
        std::string{enabledText(config.hideVanillaHud, mLanguage)},
        std::string{enabledText(config.smoothDisplayedSpeed, mLanguage)},
        std::string{enabledText(config.cameraAssistEnabled, mLanguage)},
        std::string{enabledText(config.telemetryEnabled, mLanguage)},
        std::string{enabledText(config.checkpointsEnabled, mLanguage)},
        fmt::format("{} px", config.offsetX),
        fmt::format("{} px", config.offsetY),
        "",
    };

    for (std::size_t row = 0; row < static_cast<std::size_t>(Row::Count); ++row) {
        float const rowY     = panelY + RowsTop + static_cast<float>(row) * RowHeight;
        bool const  selected = row == mSelectedRow;
        fill(
            context,
            panelX + 8.0f,
            rowY,
            PanelWidth - 16.0f,
            RowHeight - 1.0f,
            selected ? SelectedColor : RowColor,
            selected ? 0.75f : 0.55f
        );
        drawLabel(
            context,
            font,
            labels[row],
            makeRectangle(panelX + 13.0f, rowY + 1.0f, PanelWidth - 26.0f, TextHeight),
            row == static_cast<std::size_t>(Row::Done) ? ui::TextAlignment::Center : ui::TextAlignment::Left,
            row == static_cast<std::size_t>(Row::Done) ? TextColor : MutedTextColor
        );
        if (row == static_cast<std::size_t>(Row::Done)) continue;

        bool const booleanRow =
            row == static_cast<std::size_t>(Row::HudEnabled) || row == static_cast<std::size_t>(Row::ShowPing)
            || row == static_cast<std::size_t>(Row::ShowFps) || row == static_cast<std::size_t>(Row::HideVanillaHud)
            || row == static_cast<std::size_t>(Row::SmoothSpeed) || row == static_cast<std::size_t>(Row::CameraAssist)
            || row == static_cast<std::size_t>(Row::Telemetry) || row == static_cast<std::size_t>(Row::Checkpoints);
        bool enabled = false;
        switch (static_cast<Row>(row)) {
        case Row::HudEnabled:
            enabled = config.enabled;
            break;
        case Row::ShowPing:
            enabled = config.showPingNumber;
            break;
        case Row::ShowFps:
            enabled = config.showFps;
            break;
        case Row::HideVanillaHud:
            enabled = config.hideVanillaHud;
            break;
        case Row::SmoothSpeed:
            enabled = config.smoothDisplayedSpeed;
            break;
        case Row::CameraAssist:
            enabled = config.cameraAssistEnabled;
            break;
        case Row::Telemetry:
            enabled = config.telemetryEnabled;
            break;
        case Row::Checkpoints:
            enabled = config.checkpointsEnabled;
            break;
        default:
            break;
        }
        drawLabel(
            context,
            font,
            values[row],
            makeRectangle(panelX + 13.0f, rowY + 1.0f, PanelWidth - 26.0f, TextHeight),
            ui::TextAlignment::Right,
            booleanRow ? enabledColor(enabled) : TextColor
        );
    }

    drawLabel(
        context,
        font,
        std::string{localization::get(mLanguage, localization::Text::SettingsHelp)},
        makeRectangle(panelX + 8.0f, panelY + PanelHeight - 12.0f, PanelWidth - 16.0f, TextHeight),
        ui::TextAlignment::Center,
        MutedTextColor
    );
    context.flushText(0.0f, std::nullopt);
}

bool SettingsOverlay::shouldRenderOn(std::string_view screenName) const noexcept {
    return screenName.find("hud") != std::string_view::npos;
}

void SettingsOverlay::adjustSelected(int direction, IClientInstance& client) noexcept {
    auto& config = mConfigService.get();
    switch (static_cast<Row>(mSelectedRow)) {
    case Row::HudEnabled:
        config.enabled = !config.enabled;
        break;
    case Row::Layout:
        cycleValue(config.layout, std::array<std::string_view, 3>{"race", "classic", "compact"}, direction);
        break;
    case Row::SpeedUnit:
        cycleValue(config.speedUnit, std::array<std::string_view, 4>{"ms", "kmh", "mph", "knots"}, direction);
        break;
    case Row::AccelerationUnit:
        cycleValue(config.accelerationUnit, std::array<std::string_view, 2>{"mss", "g"}, direction);
        break;
    case Row::SpeedBar:
        cycleValue(config.speedBar, std::array<std::string_view, 3>{"packed", "mixed", "blue"}, direction);
        break;
    case Row::InputDisplay:
        cycleValue(
            config.inputDisplayMode,
            std::array<std::string_view, 4>{"off", "icons", "trace", "icons_and_trace"},
            direction
        );
        break;
    case Row::ShowPing:
        config.showPingNumber = !config.showPingNumber;
        break;
    case Row::ShowFps:
        config.showFps = !config.showFps;
        break;
    case Row::HideVanillaHud:
        config.hideVanillaHud = !config.hideVanillaHud;
        break;
    case Row::SmoothSpeed:
        config.smoothDisplayedSpeed = !config.smoothDisplayedSpeed;
        break;
    case Row::CameraAssist:
        config.cameraAssistEnabled = !config.cameraAssistEnabled;
        break;
    case Row::Telemetry:
        config.telemetryEnabled = !config.telemetryEnabled;
        break;
    case Row::Checkpoints:
        config.checkpointsEnabled = !config.checkpointsEnabled;
        break;
    case Row::OffsetX:
        config.offsetX = std::clamp(config.offsetX + (direction < 0 ? -4 : 4), -1000, 1000);
        break;
    case Row::OffsetY:
        config.offsetY = std::clamp(config.offsetY + (direction < 0 ? -4 : 4), 0, 1000);
        break;
    case Row::Done:
        close(client);
        break;
    case Row::Count:
        break;
    }
}

void SettingsOverlay::updatePointer(IClientInstance& client, short rawX, short rawY) noexcept {
    auto const  screenSize = *client.getGuiData()->mScreenSizeData;
    float const width      = std::max(screenSize.clientScreenSize->x, 1.0f);
    float const height     = std::max(screenSize.clientScreenSize->y, 1.0f);
    auto const  viewport   = *screenSize.clientUIScreenSize;
    mPointerX = std::clamp(static_cast<float>(rawX) * viewport.x / width, 0.0f, std::max(viewport.x - 1.0f, 0.0f));
    mPointerY = std::clamp(static_cast<float>(rawY) * viewport.y / height, 0.0f, std::max(viewport.y - 1.0f, 0.0f));
}

void SettingsOverlay::movePointer(IClientInstance& client, short deltaX, short deltaY) noexcept {
    auto const  screenSize = *client.getGuiData()->mScreenSizeData;
    float const width      = std::max(screenSize.clientScreenSize->x, 1.0f);
    float const height     = std::max(screenSize.clientScreenSize->y, 1.0f);
    auto const  viewport   = *screenSize.clientUIScreenSize;
    mPointerX              = std::clamp(
        mPointerX + static_cast<float>(deltaX) * viewport.x / width,
        0.0f,
        std::max(viewport.x - 1.0f, 0.0f)
    );
    mPointerY = std::clamp(
        mPointerY + static_cast<float>(deltaY) * viewport.y / height,
        0.0f,
        std::max(viewport.y - 1.0f, 0.0f)
    );
}

void SettingsOverlay::selectPointerRow(IClientInstance& client) noexcept {
    auto const  viewport = *client.getGuiData()->mScreenSizeData->clientUIScreenSize;
    float const panelX   = (viewport.x - PanelWidth) / 2.0f;
    float const panelY   = std::max((viewport.y - PanelHeight) / 2.0f, 2.0f);
    float const rowsY    = panelY + RowsTop;
    if (mPointerX < panelX + 8.0f || mPointerX > panelX + PanelWidth - 8.0f || mPointerY < rowsY) return;

    std::size_t const row = static_cast<std::size_t>((mPointerY - rowsY) / RowHeight);
    if (row < static_cast<std::size_t>(Row::Count)) mSelectedRow = row;
}

} // namespace boat_hud::settings
