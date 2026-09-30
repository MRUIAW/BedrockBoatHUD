// SPDX-License-Identifier: CC0-1.0

#include "hud/HudRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "fmt/format.h"

#include "mc/client/game/IClientInstance.h"
#include "mc/client/gui/CaretMeasureData.h"
#include "mc/client/gui/FontHandle.h"
#include "mc/client/gui/GuiData.h"
#include "mc/client/gui/TextAlignment.h"
#include "mc/client/gui/TextMeasureData.h"
#include "mc/client/renderer/screen/MinecraftUIRenderContext.h"
#include "mc/deps/core/math/Color.h"
#include "mc/deps/input/RectangleArea.h"

namespace boat_hud::hud {
namespace {

constexpr double Gravity    = 9.80665;
constexpr float  TextHeight = 10.0f;

mce::Color const PanelColor{33, 33, 33};
mce::Color const BorderColor{4, 4, 4};
mce::Color const TextColor{255, 255, 255};
mce::Color const MutedTextColor{170, 170, 170};
mce::Color const AccentColor{49, 137, 211};
mce::Color const PositiveColor{85, 255, 85};
mce::Color const NegativeColor{255, 85, 85};
mce::Color const GMeterNormalColor{255, 191, 0};
mce::Color const TraceColor{230, 230, 230};
mce::Color const BarDarkColor{38, 38, 38};
mce::Color const BarUnlitColor{73, 73, 73};
mce::Color const BarUnlitMidColor{57, 57, 57};
mce::Color const BarUnlitBlueColor{13, 49, 82};
mce::Color const BarUnlitBlueMidColor{13, 40, 67};
mce::Color const BarUnlitBlueEdgeColor{13, 32, 49};
mce::Color const BarWhiteEdgeColor{74, 74, 74};
mce::Color const BarWhiteTopColor{236, 236, 236};
mce::Color const BarWhiteMidColor{185, 185, 185};
mce::Color const BarWhiteBottomColor{140, 140, 140};
mce::Color const BarBlueEdgeColor{13, 49, 83};
mce::Color const BarBlueTopColor{13, 127, 237};
mce::Color const BarBlueMidColor{13, 102, 188};
mce::Color const BarBlueBottomColor{13, 81, 146};

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

double speedRate(std::string_view unit) noexcept {
    if (unit == "kmh") return 3.6;
    if (unit == "mph") return 2.236936;
    if (unit == "knots") return 1.943844;
    return 1.0;
}

std::string_view speedSuffix(std::string_view unit) noexcept {
    if (unit == "kmh") return "km/h";
    if (unit == "mph") return "mph";
    if (unit == "knots") return "kt";
    return "m/s";
}

double displayedAcceleration(double acceleration, std::string_view unit) noexcept {
    return unit == "g" ? acceleration / Gravity : acceleration;
}

std::string_view accelerationSuffix(std::string_view unit) noexcept { return unit == "g" ? "g" : "m/s2"; }

struct SpeedBarState {
    double progress = 0.0;
    bool   overflow = false;
};

SpeedBarState speedBarState(double speed, std::string_view profile) noexcept {
    double minimum = 0.0;
    double maximum = 40.0;

    if (profile == "mixed") maximum = 72.0;
    if (profile == "blue") {
        minimum = 40.0;
        maximum = 72.7;
    }

    if (speed < minimum) return {};
    if (speed > maximum) return {1.0, true};
    return {std::clamp((speed - minimum) / (maximum - minimum), 0.0, 1.0), false};
}

void drawUnlitBarSection(MinecraftUIRenderContext& context, float x, float y, float width, bool blue) noexcept {
    if (width <= 0.0f) return;
    fill(context, x, y, width, 5.0f, blue ? BarUnlitBlueEdgeColor : BarDarkColor);
    fill(context, x, y + 1.0f, width, 1.0f, blue ? BarUnlitBlueColor : BarUnlitColor);
    fill(context, x, y + 2.0f, width, 1.0f, blue ? BarUnlitBlueMidColor : BarUnlitMidColor);
    fill(context, x, y + 3.0f, width, 1.0f, blue ? BarUnlitBlueColor : BarUnlitColor);
}

void drawLitBarSection(MinecraftUIRenderContext& context, float x, float y, float width, bool blue) noexcept {
    if (width <= 0.0f) return;
    fill(context, x, y, width, 5.0f, blue ? BarBlueEdgeColor : BarWhiteEdgeColor);
    fill(context, x, y + 1.0f, width, 1.0f, blue ? BarBlueTopColor : BarWhiteTopColor);
    fill(context, x, y + 2.0f, width, 1.0f, blue ? BarBlueMidColor : BarWhiteMidColor);
    fill(context, x, y + 3.0f, width, 1.0f, blue ? BarBlueBottomColor : BarWhiteBottomColor);
}

void drawSpeedBarSections(
    MinecraftUIRenderContext& context,
    float                     x,
    float                     y,
    float                     width,
    float                     visibleWidth,
    std::string_view          profile,
    bool                      lit
) noexcept {
    auto const drawSection = lit ? drawLitBarSection : drawUnlitBarSection;
    if (profile == "blue") {
        drawSection(context, x, y, visibleWidth, true);
        return;
    }

    // The original texture changes colour at 8 m/s (packed) or 40 m/s (mixed).
    float const split     = std::floor(width * (profile == "mixed" ? 40.0f / 72.0f : 8.0f / 40.0f));
    bool const  firstBlue = profile != "mixed";
    drawSection(context, x, y, std::min(visibleWidth, split), firstBlue);
    drawSection(context, x + split, y, std::max(visibleWidth - split, 0.0f), !firstBlue);
}

void drawSpeedBar(
    MinecraftUIRenderContext& context,
    float                     x,
    float                     y,
    float                     width,
    double                    speed,
    std::string_view          profile,
    std::uint64_t             elapsedTicks
) noexcept {
    drawSpeedBarSections(context, x, y, width, width, profile, false);

    auto const state = speedBarState(speed, profile);
    if (speed < (profile == "blue" ? 40.0 : 0.0) || (state.overflow && elapsedTicks % 2 == 0)) return;

    float const litWidth = std::min(std::floor(width * static_cast<float>(state.progress)) + 1.0f, width);
    drawSpeedBarSections(context, x, y, width, litWidth, profile, true);
}

bool showsIcons(std::string_view mode) noexcept { return mode == "icons" || mode == "icons_and_trace"; }

bool showsTrace(std::string_view mode) noexcept { return mode == "trace" || mode == "icons_and_trace"; }

void drawInputIcon(
    MinecraftUIRenderContext& context,
    Font&                     font,
    float                     x,
    float                     y,
    std::string               label,
    bool                      active
) noexcept {
    auto const& color = active ? PositiveColor : MutedTextColor;
    fill(context, x, y, 11.0f, 9.0f, color, active ? 0.35f : 0.16f);
    context.drawRectangle(makeRectangle(x, y, 11.0f, 9.0f), color, 0.9f, 1);
    if (label == "L" || label == "R") {
        bool const pointsLeft = label == "L";
        for (int arrow = 0; arrow < 2; ++arrow) {
            float const baseX     = x + 2.0f + static_cast<float>(arrow * 4);
            float const direction = pointsLeft ? 1.0f : -1.0f;
            float const tipX      = pointsLeft ? baseX : baseX + 2.0f;
            fill(context, tipX, y + 4.0f, 1.0f, 1.0f, color);
            fill(context, tipX + direction, y + 3.0f, 1.0f, 3.0f, color);
            fill(context, tipX + direction * 2.0f, y + 2.0f, 1.0f, 5.0f, color);
        }
        return;
    }
    drawLabel(
        context,
        font,
        std::move(label),
        makeRectangle(x, y - 1.0f, 11.0f, TextHeight),
        ui::TextAlignment::Center,
        color
    );
}

void drawSteeringIcon(MinecraftUIRenderContext& context, float x, float y, bool pointsLeft, bool active) noexcept {
    auto const& color = active ? mce::Color{85, 255, 255} : MutedTextColor;
    for (int arrow = 0; arrow < 3; ++arrow) {
        float const left = x + 1.0f + static_cast<float>(arrow * 5);
        for (int row = 0; row < 3; ++row) {
            float const offset = static_cast<float>(row);
            float const pixelX = pointsLeft ? left + 2.0f - offset : left + offset;
            fill(context, pixelX, y + 1.0f + offset, 1.0f, 1.0f, color);
            fill(context, pixelX, y + 6.0f - offset, 1.0f, 1.0f, color);
        }
    }
}

void drawThrottleBar(
    MinecraftUIRenderContext& context,
    float                     x,
    float                     y,
    float                     width,
    bool                      forward,
    bool                      active
) noexcept {
    mce::Color const darkColor   = forward ? mce::Color{0, 65, 0} : mce::Color{70, 0, 0};
    mce::Color const brightColor = forward ? mce::Color{0, 255, 0} : mce::Color{255, 55, 55};
    auto const&      color       = active ? brightColor : darkColor;
    fill(context, x, y, width, 5.0f, color, active ? 0.95f : 0.8f);
    fill(context, x + 1.0f, y + 1.0f, std::max(width - 2.0f, 0.0f), 1.0f, color);
}

void drawPingIcon(MinecraftUIRenderContext& context, float x, float y, std::int64_t pingMilliseconds) noexcept {
    int activeBars = 0;
    if (pingMilliseconds >= 0 && pingMilliseconds < 1000) activeBars = 1;
    if (pingMilliseconds >= 0 && pingMilliseconds < 600) activeBars = 2;
    if (pingMilliseconds >= 0 && pingMilliseconds < 300) activeBars = 3;
    if (pingMilliseconds >= 0 && pingMilliseconds < 150) activeBars = 4;

    for (int bar = 0; bar < 4; ++bar) {
        float const height = 2.0f + static_cast<float>(bar * 2);
        fill(
            context,
            x + static_cast<float>(bar * 2),
            y + 8.0f - height,
            1.0f,
            height,
            bar < activeBars ? TextColor : MutedTextColor,
            bar < activeBars ? 1.0f : 0.45f
        );
    }
}

void drawPanelBackground(MinecraftUIRenderContext& context, float x, float y, float width, float height) noexcept {
    fill(context, x, y, width, height, PanelColor, 0.64f);
    context.drawRectangle(makeRectangle(x, y, width, height), BorderColor, 1.0f, 1);
}

void drawTrace(
    MinecraftUIRenderContext& context,
    std::deque<float> const&  trace,
    float                     x,
    float                     centerY,
    float                     width,
    mce::Color const&         color
) noexcept {
    if (trace.empty()) return;

    float const step  = width / static_cast<float>(trace.size());
    std::size_t index = 0;
    for (float const value : trace) {
        float const offset = std::clamp(value, -1.0f, 1.0f) * 4.0f;
        fill(context, x + static_cast<float>(index) * step, centerY - offset, std::max(step, 1.0f), 1.0f, color);
        ++index;
    }
}

void drawGMeter(MinecraftUIRenderContext& context, float centerX, float centerY, HudSnapshot const& snapshot) noexcept {
    constexpr float Size = 18.0f;
    float const     x    = centerX - Size / 2.0f;
    float const     y    = centerY - Size / 2.0f;
    context.drawRectangle(makeRectangle(x, y, Size, Size), TextColor, 1.0f, 1);
    context.drawRectangle(makeRectangle(centerX - 5.0f, centerY - 5.0f, 10.0f, 10.0f), TextColor, 1.0f, 1);

    int const  markerX = std::clamp(static_cast<int>(snapshot.lateralAcceleration / 2.5), -8, 8);
    int const  markerY = std::clamp(static_cast<int>(snapshot.longitudinalAcceleration / 2.5), -8, 8);
    bool const warning =
        std::abs(snapshot.lateralAcceleration) > 22.5 || std::abs(snapshot.longitudinalAcceleration) > 22.5;
    fill(
        context,
        centerX - 1.0f + static_cast<float>(markerX),
        centerY - 1.0f - static_cast<float>(markerY),
        2.0f,
        2.0f,
        warning ? mce::Color{255, 0, 0} : GMeterNormalColor
    );
}

} // namespace

void HudRenderer::reset() noexcept {
    mLastRenderTime    = {};
    mFrameWindowStart  = {};
    mDisplayedSpeed    = 0.0;
    mFramesInWindow    = 0;
    mHasDisplayedSpeed = false;
}

std::optional<int> HudRenderer::recordFrame() noexcept {
    auto const now = std::chrono::steady_clock::now();
    if (mFrameWindowStart.time_since_epoch().count() == 0) mFrameWindowStart = now;
    ++mFramesInWindow;

    auto const elapsed = std::chrono::duration<double>(now - mFrameWindowStart).count();
    if (elapsed < 1.0) return std::nullopt;

    int const framesPerSecond = static_cast<int>(std::lround(static_cast<double>(mFramesInWindow) / elapsed));
    mFrameWindowStart         = now;
    mFramesInWindow           = 0;
    return framesPerSecond;
}

void HudRenderer::render(
    MinecraftUIRenderContext&    context,
    IClientInstance&             client,
    HudSnapshot const&           snapshot,
    config::BoatHudConfig const& config
) noexcept {
    auto const   viewport       = *client.getGuiData()->mScreenSizeData->clientUIScreenSize;
    float const  panelWidth     = config.layout == "compact" ? 146.0f : (config.layout == "classic" ? 182.0f : 218.0f);
    float const  panelHeight    = config.layout == "compact" ? 26.0f : (config.layout == "classic" ? 33.0f : 42.0f);
    float const  panelX         = (viewport.x - panelWidth) / 2.0f + static_cast<float>(config.offsetX);
    float const  panelY         = viewport.y - static_cast<float>(config.offsetY) - panelHeight;
    float const  centerX        = panelX + panelWidth / 2.0f;
    double const speed          = updateDisplayedSpeed(snapshot.speed, config);
    double const convertedSpeed = speed * speedRate(config.speedUnit);

    auto  fontHandle = client.getFontHandle();
    auto& font       = fontHandle.getFont();

    drawPanelBackground(context, panelX, panelY, panelWidth, panelHeight);
    drawSpeedBar(
        context,
        panelX + 1.0f,
        panelY + 1.0f,
        panelWidth - 2.0f,
        speed,
        config.speedBar,
        snapshot.elapsedTicks
    );

    std::string const speedText    = fmt::format("{:03.0f} {}", convertedSpeed, speedSuffix(config.speedUnit));
    std::string const slipText     = fmt::format("{:03.0f} deg", std::abs(snapshot.slipAngle));
    double const      longitudinal = displayedAcceleration(snapshot.longitudinalAcceleration, config.accelerationUnit);
    double const      lateral      = displayedAcceleration(snapshot.lateralAcceleration, config.accelerationUnit);
    std::string const accelerationText =
        fmt::format("{:+.1f}/{:.1f} {}", longitudinal, std::abs(lateral), accelerationSuffix(config.accelerationUnit));
    std::string const raceAccelerationText = fmt::format(
        "{:+.{}f} / {:.{}f} {}",
        longitudinal,
        std::abs(longitudinal) < 10.0 ? 2 : 1,
        std::abs(lateral),
        std::abs(lateral) < 10.0 ? 2 : 1,
        accelerationSuffix(config.accelerationUnit)
    );

    if (config.layout == "compact") {
        drawLabel(
            context,
            font,
            speedText,
            makeRectangle(panelX + 4.0f, panelY + 7.0f, 62.0f, TextHeight),
            ui::TextAlignment::Left
        );
        drawLabel(
            context,
            font,
            slipText,
            makeRectangle(panelX + panelWidth - 66.0f, panelY + 7.0f, 62.0f, TextHeight),
            ui::TextAlignment::Right
        );
        if (showsIcons(config.inputDisplayMode)) {
            drawInputIcon(context, font, centerX - 24.0f, panelY + 16.0f, "L", snapshot.input.left);
            drawInputIcon(context, font, centerX - 11.0f, panelY + 16.0f, "B", snapshot.input.backward);
            drawInputIcon(context, font, centerX + 2.0f, panelY + 16.0f, "F", snapshot.input.forward);
            drawInputIcon(context, font, centerX + 15.0f, panelY + 16.0f, "R", snapshot.input.right);
        }
        drawPingIcon(context, panelX + 5.0f, panelY + 17.0f, snapshot.pingMilliseconds);
        if (config.showFps) {
            drawLabel(
                context,
                font,
                fmt::format("{}", snapshot.framesPerSecond),
                makeRectangle(panelX + panelWidth - 30.0f, panelY + 17.0f, 25.0f, TextHeight),
                ui::TextAlignment::Right,
                MutedTextColor
            );
        }
        context.flushText(0.0f, std::nullopt);
        return;
    }

    if (config.layout == "classic") {
        drawLabel(
            context,
            font,
            speedText,
            makeRectangle(centerX - 88.0f, panelY + 7.0f, 60.0f, TextHeight),
            ui::TextAlignment::Center
        );
        drawLabel(
            context,
            font,
            slipText,
            makeRectangle(centerX - 30.0f, panelY + 7.0f, 60.0f, TextHeight),
            ui::TextAlignment::Center
        );
        drawLabel(
            context,
            font,
            accelerationText,
            makeRectangle(centerX + 28.0f, panelY + 7.0f, 60.0f, TextHeight),
            ui::TextAlignment::Center
        );
        if (showsIcons(config.inputDisplayMode)) {
            drawSteeringIcon(context, panelX + 5.0f, panelY + 18.0f, true, snapshot.input.left);
            drawSteeringIcon(context, panelX + 28.0f, panelY + 18.0f, false, snapshot.input.right);
            drawThrottleBar(context, centerX - 61.0f, panelY + 28.0f, 61.0f, false, snapshot.input.backward);
            drawThrottleBar(context, centerX, panelY + 28.0f, 61.0f, true, snapshot.input.forward);
        }
        if (showsTrace(config.inputDisplayMode)) {
            drawTrace(context, snapshot.throttleTrace, centerX - 61.0f, panelY + 23.0f, 61.0f, NegativeColor);
            drawTrace(context, snapshot.steeringTrace, centerX, panelY + 23.0f, 61.0f, AccentColor);
        }
        drawPingIcon(context, panelX + panelWidth - 58.0f, panelY + 18.0f, snapshot.pingMilliseconds);
        if (config.showPingNumber) {
            drawLabel(
                context,
                font,
                fmt::format("{} ms", snapshot.pingMilliseconds),
                makeRectangle(panelX + panelWidth - 48.0f, panelY + 18.0f, 44.0f, TextHeight),
                ui::TextAlignment::Right
            );
        }
        context.flushText(0.0f, std::nullopt);
        return;
    }

    drawLabel(
        context,
        font,
        speedText,
        makeRectangle(panelX + 5.0f, panelY + 7.0f, 58.0f, TextHeight),
        ui::TextAlignment::Left
    );
    drawLabel(
        context,
        font,
        slipText,
        makeRectangle(panelX + panelWidth - 63.0f, panelY + 7.0f, 58.0f, TextHeight),
        ui::TextAlignment::Right
    );
    drawGMeter(context, centerX, panelY + 15.0f, snapshot);
    if (showsTrace(config.inputDisplayMode)) {
        drawTrace(context, snapshot.throttleTrace, centerX - 57.0f, panelY + 13.0f, 42.0f, TraceColor);
        drawTrace(context, snapshot.steeringTrace, centerX + 15.0f, panelY + 13.0f, 42.0f, AccentColor);
    }
    drawPingIcon(context, centerX + 16.0f, panelY + 20.0f, snapshot.pingMilliseconds);
    if (config.showPingNumber) {
        drawLabel(
            context,
            font,
            fmt::format("{} ms", snapshot.pingMilliseconds),
            makeRectangle(centerX + 26.0f, panelY + 20.0f, 42.0f, TextHeight),
            ui::TextAlignment::Left,
            snapshot.pingMilliseconds > 500 ? NegativeColor : TextColor
        );
    }
    if (config.showFps) {
        drawLabel(
            context,
            font,
            fmt::format("{} fps", snapshot.framesPerSecond),
            makeRectangle(panelX + panelWidth - 57.0f, panelY + 20.0f, 52.0f, TextHeight),
            ui::TextAlignment::Right
        );
    }
    if (snapshot.checkpointResultAvailable) {
        auto const& timeColor  = snapshot.checkpointTimeDelta > 0.025 ? NegativeColor : PositiveColor;
        auto const& speedColor = snapshot.checkpointSpeedDelta < -0.4 ? NegativeColor : PositiveColor;
        drawLabel(
            context,
            font,
            fmt::format(
                "CP {}/{} {:+.3f} s",
                snapshot.checkpointIndex,
                snapshot.checkpointCount,
                snapshot.checkpointTimeDelta
            ),
            makeRectangle(panelX + 5.0f, panelY + 20.0f, 76.0f, TextHeight),
            ui::TextAlignment::Left,
            timeColor
        );
        drawLabel(
            context,
            font,
            fmt::format(
                "{:+.1f} {}",
                snapshot.checkpointSpeedDelta * speedRate(config.speedUnit),
                speedSuffix(config.speedUnit)
            ),
            makeRectangle(panelX + 81.0f, panelY + 20.0f, 48.0f, TextHeight),
            ui::TextAlignment::Right,
            speedColor
        );
    } else {
        drawLabel(
            context,
            font,
            raceAccelerationText,
            makeRectangle(panelX + 5.0f, panelY + 20.0f, 92.0f, TextHeight),
            ui::TextAlignment::Left
        );
    }
    if (showsIcons(config.inputDisplayMode)) {
        drawSteeringIcon(context, centerX - 39.0f, panelY + 31.0f, true, snapshot.input.left);
        drawSteeringIcon(context, centerX + 22.0f, panelY + 31.0f, false, snapshot.input.right);
        drawInputIcon(context, font, centerX - 12.0f, panelY + 31.0f, "B", snapshot.input.backward);
        drawInputIcon(context, font, centerX + 1.0f, panelY + 31.0f, "F", snapshot.input.forward);
    }
    context.flushText(0.0f, std::nullopt);
}

double HudRenderer::updateDisplayedSpeed(double targetSpeed, config::BoatHudConfig const& config) noexcept {
    auto const now = std::chrono::steady_clock::now();
    if (!mHasDisplayedSpeed || !config.smoothDisplayedSpeed) {
        mDisplayedSpeed    = targetSpeed;
        mHasDisplayedSpeed = true;
        mLastRenderTime    = now;
        return mDisplayedSpeed;
    }

    double const frameSeconds  = std::clamp(std::chrono::duration<double>(now - mLastRenderTime).count(), 0.0, 0.25);
    double const alpha         = 1.0 - std::exp(-config.speedSmoothingResponse * frameSeconds);
    mDisplayedSpeed           += (targetSpeed - mDisplayedSpeed) * alpha;
    mLastRenderTime            = now;
    return mDisplayedSpeed;
}

} // namespace boat_hud::hud
