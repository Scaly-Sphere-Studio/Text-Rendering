#pragma once

#ifdef SSS_LUA
#include <sol/sol.hpp>
#include "SSS/Text-Rendering/Globals.hpp"
#include "SSS/Text-Rendering/Area.hpp"

SSS_TR_BEGIN;

inline void lua_setup_TR(sol::state& lua) try
{
    auto tr = lua["TR"].get_or_create<sol::table>();

    // All Format-related types
    {
        // Format
        tr.new_usertype<Format>("Fmt",
            // Font
            "font", &Format::font,
            // Style
            "charsize", &Format::charsize,
            "has_outline", &Format::has_outline,
            "outline_size", &Format::outline_size,
            "has_shadow", &Format::has_shadow,
            "shadow_offset", &Format::shadow_offset,
            "line_spacing", &Format::line_spacing,
            "alignment", &Format::aligmnent,
            "effect", &Format::effect,
            "effect_offset", &Format::effect_offset,
            // Color
            "text_color", &Format::text_color,
            "outline_color", &Format::outline_color,
            "shadow_color", &Format::shadow_color,
            "alpha", &Format::alpha,
            // Language
            "lng_tag", &Format::lng_tag,
            "lng_script", &Format::lng_script,
            "lng_direction", &Format::lng_direction,
            "word_dividers", &Format::word_dividers
        );
        // Alignment (enum)
        tr.new_enum<Alignment>("Alignment", {
            { "Left", Alignment::Left },
            { "Center", Alignment::Center },
            { "Right", Alignment::Right }
        });
        // Effect (enum)
        tr.new_enum<Effect>("Effect", {
            { "None", Effect::None },
            { "Vibrate", Effect::Vibrate },
            { "Waves", Effect::Waves },
            { "FadingWaves", Effect::FadingWaves }
        });
        // ColorFunc (enum)
        tr.new_enum<ColorFunc>("ColorFunc", {
            { "None", ColorFunc::None },
            { "Rainbow", ColorFunc::Rainbow },
            { "RainbowFixed", ColorFunc::RainbowFixed }
        });
        // Color (struct)
        tr.new_usertype<Color>("Color",
            sol::base_classes, sol::bases<RGB24>(),
            "rgb", sol::property(
                [](RGB24& self) { return self.rgb; },
                [](RGB24& self, uint32_t color) { self.rgb = color; }),
            "r", &Color::r,
            "g", &Color::g,
            "b", &Color::b,
            "func", &Color::func
        );
        // FT_Vector (FreeType struct)
        tr.new_usertype<FT_Vector>("FT_Vector",
            "x", &FT_Vector::x,
            "y", &FT_Vector::y
        );
    }
    // Area
    tr.new_usertype<Area>("Area",
        // Parse & clear
        "string", sol::property(&Area::parseStringU32, &Area::getStringU32),
        "clear_color", sol::property(&Area::getClearColor, &Area::setClearColor),
        "clear", &Area::clear,
        // Format
        "getFmt", &Area::getFormat,
        "setFmt", &Area::setFormat,
        // Margins
        "getMargins", [](Area& area) {
            return std::make_tuple(area.getMarginH(), area.getMarginV());
        },
        "setMargins", &Area::setMargins,
        // Dimensions
        "getDimensions", [](Area& area) {
            int w, h; area.getDimensions(w, h); return std::make_tuple(w, h);
        },
        "setDimensions", &Area::setDimensions,
        // Scroll
        "scroll", &Area::scroll,
        // Focus
        "focus", sol::property(&Area::isFocused, &Area::setFocus),
        // Print mode
        "print_mode", sol::property(&Area::getPrintMode, &Area::setPrintMode),
        "TW_speed", sol::property(&Area::getTypeWriterSpeed, &Area::setTypeWriterSpeed),
        // Static
        "get", [](uint32_t id)->Area* {
            if (Area::getMap().count(id) == 0)
                return nullptr;
            return Area::getMap().at(id).get();
        }
    );
}
CATCH_AND_RETHROW_FUNC_EXC;

SSS_TR_END;

#endif
