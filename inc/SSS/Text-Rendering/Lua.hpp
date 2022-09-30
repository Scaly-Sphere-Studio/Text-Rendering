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
        auto fmt = tr.new_usertype<Format>("Fmt");
        // Font
        fmt["font"] = &Format::font;
        // Style
        fmt["charsize"] = &Format::charsize;
        fmt["has_outline"] = &Format::has_outline;
        fmt["outline_size"] = &Format::outline_size;
        fmt["has_shadow"] = &Format::has_shadow;
        fmt["shadow_offset"] = &Format::shadow_offset;
        fmt["line_spacing"] = &Format::line_spacing;
        fmt["alignment"] = &Format::aligmnent;
        fmt["effect"] = &Format::effect;
        fmt["effect_offset"] = &Format::effect_offset;
        // Color
        fmt["text_color"] = &Format::text_color;
        fmt["outline_color"] = &Format::outline_color;
        fmt["shadow_color"] = &Format::shadow_color;
        fmt["alpha"] = &Format::alpha;
        // Language
        fmt["lng_tag"] = &Format::lng_tag;
        fmt["lng_script"] = &Format::lng_script;
        fmt["lng_direction"] = &Format::lng_direction;
        fmt["word_dividers"] = &Format::word_dividers;

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
        auto color = tr.new_usertype<Color>("Color", sol::base_classes, sol::bases<RGB24>());
        color["rgb"] = sol::property(
            [](RGB24& self) { return self.rgb; },
            [](RGB24& self, uint32_t color) { self.rgb = color; }
        );
        color["r"] = &Color::r;
        color["g"] = &Color::g;
        color["b"] = &Color::b;
        color["func"] = &Color::func;
        
        // FT_Vector (FreeType struct)
        auto vec = tr.new_usertype<FT_Vector>("FT_Vector");
        vec["x"] = &FT_Vector::x;
        vec["y"] = &FT_Vector::y;
    }
    // Area
    auto area = tr.new_usertype<Area>("Area");
    // Parse & clear
    area["string"] = sol::property(&Area::parseStringU32, &Area::getStringU32);
    area["clear_color"] = sol::property(&Area::getClearColor, &Area::setClearColor);
    area["clear"] = &Area::clear;
    // Format
    area["getFmt"] = &Area::getFormat;
    area["setFmt"] = &Area::setFormat;
    area["wrapping"] = sol::property(&Area::getWrapping, &Area::setWrapping);
    // Margins
    area["getMargins"] = [](Area& area) {
        return std::make_tuple(area.getMarginV(), area.getMarginH());
    };
    area["setMargins"] = &Area::setMargins;
    area["margin_v"] = sol::property(&Area::getMarginV, &Area::setMarginV);
    area["margin_h"] = sol::property(&Area::getMarginH, &Area::setMarginH);
    // Dimensions
    area["getDimensions"] = [](Area& area) {
        int w, h; area.getDimensions(w, h); return std::make_tuple(w, h);
    };
    area["setDimensions"] = &Area::setDimensions;
    area["w"] = sol::property(&Area::getWidth, &Area::setWidth);
    area["h"] = sol::property(&Area::getHeight, &Area::setHeight);
    // Scroll
    area["scroll"] = &Area::scroll;
    // Focus
    area["focus"] = sol::property(&Area::isFocused, &Area::setFocus);
    // Print mode
    area["print_mode"] = sol::property(&Area::getPrintMode, &Area::setPrintMode);
    area["TW_speed"] = sol::property(&Area::getTypeWriterSpeed, &Area::setTypeWriterSpeed);
    // Static
    area["get"] = [](uint32_t id)->Area* {
        if (Area::getMap().count(id) == 0)
            return Area::create(id).get();
        return Area::getMap().at(id).get();
    };
    area["getFocused"] = []()->Area* {
        return Area::getFocused().get();
    };
    area["resetFocus"] = &Area::resetFocus;

    // PrintMode
    tr.new_enum<PrintMode>("PrintMode", {
        { "Instant", PrintMode::Instant},
        { "Typewriter", PrintMode::Typewriter}
    });

    tr["addFontDir"] = &addFontDir;
}
CATCH_AND_RETHROW_FUNC_EXC;

SSS_TR_END;

#endif
