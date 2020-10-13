#pragma once

#include "SSS/Text-Rendering/_includes.hpp"

__SSS_TR_BEGIN

// --- Style options ---
struct TextStyle {
public:
    TextStyle(
        int charsize_ = 12,
        int outline_size_ = 0,
        bool has_outline_ = false,
        bool has_shadow_ = false,
        float line_spacing_ = 1.5f
    ) noexcept :
        charsize(charsize_),
        outline_size(outline_size_),
        has_outline(has_outline_),
        has_shadow(has_shadow_),
        line_spacing(line_spacing_)
    {};
    int charsize;       // Charsize
    int outline_size;   // Size of outline
    bool has_outline;   // Whether this text has an outline
    bool has_shadow;    // Whether this text has shadows
    float line_spacing; // Spacing between lines
};

// --- Color options ---
struct TextColors {
    TextColors(
        BGR24::s text_ = BGR24(0xFFFFFFU),      // White
        BGR24::s outline_ = BGR24(0x000000U),   // Black
        BGR24::s shadow_ = BGR24(0x444444U),    // Grey
        uint8_t alpha_ = 0xFFU                  // No transparency
    ) noexcept :
        text(text_),
        outline(outline_),
        shadow(shadow_),
        alpha(alpha_)
    {};
    BGR24::s text;      // Text color
    BGR24::s outline;   // Outline color
    BGR24::s shadow;    // Shadow color
    uint8_t alpha;      // Overall transparency
};

// --- Language options ---
struct TextLanguage {
private:
    using _c32Vector = std::vector<char32_t>;

public:
    TextLanguage(
        std::string const& language_ = "en",            // English
        std::string const& script_ = "Latn",            // Latin script
        std::string const& direction_ = "ltr",          // Left to right
        _c32Vector const& word_dividers_ = { U' ' }   // Single word divider: space
    ) noexcept :
        language(language_),
        script(script_),
        direction(direction_),
        word_dividers(word_dividers_)
    {};
    std::string language;           // Language ("en", "fr", "ar", ...)
    std::string script;             // Script ("Latn", "Arab", ...)
    std::string direction;          // Text direction ("ltr", "rtl")
    _c32Vector word_dividers;   // Word dividers (spaces, most of the time)
};

// --- All text options put together ---
struct TextOpt {
    // --- Constructor ---
    TextOpt(
        Font::Shared font_,  // Used font
        TextStyle const& style_ = TextStyle(),      // See upward
        TextColors const& color_ = TextColors(),    // See upward
        TextLanguage const& lng_ = TextLanguage()   // See upward
    ) noexcept :
        font(font_),
        style(style_),
        color(color_),
        lng(lng_)
    {};

    // --- Variables ---
    Font::Shared font;   // Font
    TextStyle style;    // Style
    TextColors color;   // Colors
    TextLanguage lng;   // Language
};

__SSS_TR_END