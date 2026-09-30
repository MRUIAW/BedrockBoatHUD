// SPDX-License-Identifier: CC0-1.0
#pragma once

#include <string_view>

class IClientInstance;

namespace boat_hud::localization {

/// Languages currently provided by BoatHUD's in-game interface.
enum class Language {
    English,
    SimplifiedChinese,
};

/// Stable identifiers for player-facing text.
enum class Text {
    SettingsTitle,
    HudEnabled,
    Layout,
    SpeedUnit,
    AccelerationUnit,
    SpeedBar,
    InputDisplay,
    NumericPing,
    RenderFps,
    HideVanillaHud,
    SmoothSpeed,
    CameraAssist,
    TelemetryCsv,
    Checkpoints,
    HorizontalOffset,
    BottomOffset,
    Done,
    Enabled,
    Disabled,
    SettingsHelp,
    Count,
};

/// Maps Minecraft's current language code to a supported BoatHUD language.
/// @param client Active client whose option registry owns the language selection.
/// @return Simplified Chinese for Minecraft's zh_CN or the equivalent zh-Hans tag, otherwise English.
/// @throws Nothing.
[[nodiscard]] Language detectLanguage(IClientInstance const& client) noexcept;

/// Looks up a localized player-facing string.
/// @param language Selected supported language.
/// @param text Stable text identifier.
/// @return UTF-8 text loaded through LeviLamina i18n, or the built-in English fallback.
/// @throws Nothing.
[[nodiscard]] std::string_view get(Language language, Text text) noexcept;

/// Localizes a persisted layout identifier without changing the stored value.
[[nodiscard]] std::string_view layoutName(Language language, std::string_view value) noexcept;

/// Localizes a persisted speed-unit identifier without changing the stored value.
[[nodiscard]] std::string_view speedUnitName(Language language, std::string_view value) noexcept;

/// Localizes a persisted acceleration-unit identifier without changing the stored value.
[[nodiscard]] std::string_view accelerationUnitName(Language language, std::string_view value) noexcept;

/// Localizes a persisted speed-bar identifier without changing the stored value.
[[nodiscard]] std::string_view speedBarName(Language language, std::string_view value) noexcept;

/// Localizes a persisted input-display identifier without changing the stored value.
[[nodiscard]] std::string_view inputDisplayName(Language language, std::string_view value) noexcept;

} // namespace boat_hud::localization
