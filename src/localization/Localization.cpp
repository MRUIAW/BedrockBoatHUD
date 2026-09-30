// SPDX-License-Identifier: CC0-1.0

#include "localization/Localization.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

#include "ll/api/i18n/I18n.h"
#include "mc/client/game/IClientInstance.h"
#include "mc/client/options/IOptionRegistry.h"

namespace boat_hud::localization {
namespace {

struct TranslationEntry {
    std::string_view key;
    std::string_view english;
};

using TranslationTable = std::array<TranslationEntry, static_cast<std::size_t>(Text::Count)>;

constexpr TranslationTable Translations{
    {
     {"boathud.settings.title", "BoatHUD Settings"},
     {"boathud.settings.hud_enabled", "HUD Enabled"},
     {"boathud.settings.layout", "Layout"},
     {"boathud.settings.speed_unit", "Speed Unit"},
     {"boathud.settings.acceleration_unit", "Acceleration Unit"},
     {"boathud.settings.speed_bar", "Speed Bar"},
     {"boathud.settings.input_display", "Input Display"},
     {"boathud.settings.numeric_ping", "Numeric Ping"},
     {"boathud.settings.render_fps", "Render FPS"},
     {"boathud.settings.hide_vanilla_hud", "Hide Vanilla HUD"},
     {"boathud.settings.smooth_speed", "Smooth Speed"},
     {"boathud.settings.camera_assist", "Camera Assist"},
     {"boathud.settings.telemetry_csv", "Telemetry CSV"},
     {"boathud.settings.checkpoints", "Checkpoints"},
     {"boathud.settings.horizontal_offset", "Horizontal Offset"},
     {"boathud.settings.bottom_offset", "Bottom Offset"},
     {"boathud.settings.done", "Done"},
     {"boathud.common.enabled", "ON"},
     {"boathud.common.disabled", "OFF"},
     {"boathud.settings.help", "O/Esc close  |  Arrows change  |  Mouse: left next, right previous"},
     }
};

std::string normalizeLanguageCode(std::string code) {
    std::ranges::transform(code, code.begin(), [](unsigned char character) {
        if (character == '_') return '-';
        return static_cast<char>(std::tolower(character));
    });
    return code;
}

std::string_view translate(Language language, std::string_view key, std::string_view english) noexcept {
    std::string_view const locale     = language == Language::SimplifiedChinese ? "zh_CN" : "en";
    std::string_view const translated = ll::i18n::getInstance().get(key, locale);
    if (translated.empty() || translated == key) return english;
    return translated;
}

} // namespace

Language detectLanguage(IClientInstance const& client) noexcept try {
    std::string const code = normalizeLanguageCode(client.getOptions().getLanguage());
    if (code == "zh-cn" || code.starts_with("zh-hans")) return Language::SimplifiedChinese;
    return Language::English;
} catch (...) {
    return Language::English;
}

std::string_view get(Language language, Text text) noexcept {
    std::size_t const index = static_cast<std::size_t>(text);
    if (index >= Translations.size()) return {};
    auto const& entry = Translations[index];
    return translate(language, entry.key, entry.english);
}

std::string_view layoutName(Language language, std::string_view value) noexcept {
    if (value == "race") return translate(language, "boathud.value.layout.race", "Race");
    if (value == "classic") return translate(language, "boathud.value.layout.classic", "Classic");
    if (value == "compact") return translate(language, "boathud.value.layout.compact", "Compact");
    return value;
}

std::string_view speedUnitName(Language language, std::string_view value) noexcept {
    if (value == "ms") return "m/s";
    if (value == "kmh") return "km/h";
    if (value == "mph") return "mph";
    if (value == "knots") return translate(language, "boathud.value.speed_unit.knots", "knots");
    return value;
}

std::string_view accelerationUnitName(Language, std::string_view value) noexcept {
    if (value == "mss") return "m/s2";
    if (value == "g") return "g";
    return value;
}

std::string_view speedBarName(Language language, std::string_view value) noexcept {
    if (value == "packed") return translate(language, "boathud.value.speed_bar.packed", "Water & Packed Ice");
    if (value == "mixed") return translate(language, "boathud.value.speed_bar.mixed", "Mixed Ices");
    if (value == "blue") return translate(language, "boathud.value.speed_bar.blue", "Blue Ice");
    return value;
}

std::string_view inputDisplayName(Language language, std::string_view value) noexcept {
    if (value == "off") return translate(language, "boathud.value.input.off", "Off");
    if (value == "icons") return translate(language, "boathud.value.input.icons", "Icons");
    if (value == "trace") return translate(language, "boathud.value.input.trace", "Trace");
    if (value == "icons_and_trace") {
        return translate(language, "boathud.value.input.icons_and_trace", "Icons + Trace");
    }
    return value;
}

} // namespace boat_hud::localization
