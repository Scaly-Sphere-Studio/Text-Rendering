#pragma once

#include <SSS/Text-Rendering/includes.hpp>
#include <SSS/Text-Rendering/Font.hpp>

SSS_TR_BEGIN__

// Style options
struct BufferStyle {
    BufferStyle(
        bool outline_ = false,
        bool shadow_ = false
    ) noexcept :
        outline(outline_),
        shadow(shadow_)
    {};
    bool outline;   // Whether this text has an outline
    bool shadow;    // Whether this text has shadows
};

// Color32 options
struct BufferColors {
    BufferColors(
        BGR24_s text_ = BGR24(0xFFFFFFU),       // White
        BGR24_s outline_ = BGR24(0x000000U),    // Black
        BGR24_s shadow_ = BGR24(0x444444U),     // Grey
        uint8_t alpha_ = 0xFFU                  // No transparency
    ) noexcept :
        text(text_),
        outline(outline_),
        shadow(shadow_),
        alpha(alpha_)
    {};
    BGR24_s text;       // Text color
    BGR24_s outline;    // Outline color
    BGR24_s shadow;     // Shadow color
    uint8_t alpha;      // Overall transparency
};

// Language options
struct BufferLanguage {
    BufferLanguage(
        std::string const& language_ = "en",    // English
        std::string const& script_ = "Latn",    // Latin script
        std::string const& direction_ = "ltr"   // Left to right
    ) noexcept :
        language(language_),
        script(script_),
        direction(direction_)
    {};
    std::string language;   // Language
    std::string script;     // Script
    std::string direction;  // Text direction
};

// All buffer options put together
struct BufferOpt {
private:
    // Default line_spacing value
    static constexpr float def_ls_ = 1.f;

public:
    // --- Constructors ---

    // Global constructor
    BufferOpt(
        float line_spacing_ = def_ls_,
        BufferStyle const& style_ = BufferStyle(),      // See upward
        BufferColors const& color_ = BufferColors(),    // See upward
        BufferLanguage const& lng_ = BufferLanguage(),  // See upward
        std::u32string const& word_dividers_ = { U' ' } // Unique word divider: space
    ) noexcept :
        style(style_),
        color(color_),
        lng(lng_),
        line_spacing(line_spacing_),
        word_dividers(word_dividers_)
    {};
    // Style constructor
    BufferOpt(BufferStyle const& style_) noexcept
        : BufferOpt(def_ls_, style_) {};
    // Colors constructor
    BufferOpt(BufferColors const& color_) noexcept
        : BufferOpt(def_ls_, BufferStyle(), color_) {};
    // Language constructor
    BufferOpt(BufferLanguage const& lng_) noexcept
        : BufferOpt(def_ls_, BufferStyle(), BufferColors(), lng_) {};
    // Word dividers constructor
    BufferOpt(std::u32string word_dividers_) noexcept
        : BufferOpt(def_ls_, BufferStyle(), BufferColors(), BufferLanguage(), word_dividers_) {};
    // Variables
    BufferStyle style;      // Style
    BufferColors color;     // Colors
    BufferLanguage lng;     // Language
    float line_spacing;     // Line spacing
    std::u32string word_dividers;   // Word dividers
};

// A structure filled with informations of a given glyph
struct GlyphInfo {
    // Unique constructor
    GlyphInfo(
        hb_glyph_info_t info_,
        hb_glyph_position_t pos_,
        float line_spacing_,
        BufferStyle const& style_,
        BufferColors const& color_,
        Font_Shared const& font_
    ) noexcept :
        info(info_),
        pos(pos_),
        line_spacing(line_spacing_),
        style(style_),
        color(color_),
        font(font_),
        is_word_divider(false)
    {};
    // Variables
    hb_glyph_info_t info;       // The glyph's informations
    hb_glyph_position_t pos;    // The glyph's position
    float line_spacing;         // Line spacing
    BufferStyle const& style;   // Style
    BufferColors const& color;  // Color32
    Font_Shared const& font;    // The Font object it was created with
    bool is_word_divider;       // Whether the glyph is a word divider
};

// unique_ptr alias
using hb_buffer_Ptr = std::unique_ptr<hb_buffer_t, void(*)(hb_buffer_t*)>;

class Buffer {
#ifndef NDEBUG
private:
    static constexpr bool log_constructor_ = true;
    static constexpr bool log_destructor_ = true;
#endif // NDEBUG

public:
    // Constructor, creates a HarfBuzz buffer, and shapes it with given parameters.
    Buffer(Font_Shared font, std::u32string const& str, BufferOpt const& opt = BufferOpt());
    // Destructor
    ~Buffer() noexcept;

    // Reshapes the buffer with given parameters
    void changeContents(Font_Shared font, std::u32string const& str, BufferOpt const& opt = BufferOpt());
    // Reshapes the buffer. To be called when the font charsize changes, for example
    void reshape();

    // Returns the number of glyphs in the buffer
    size_t size() const noexcept;
    // Returns a structure filled with informations of a given glyph.
    // Throws an exception if cursor is out of bound.
    GlyphInfo at(size_t cursor) const;

private:
    // Shapes the buffer and retrieve its informations
    void shape_();

    // Given parameters
    Font_Shared font_;      // Font used to draw this buffer
    std::vector<uint32_t> indexes_;     // Original string converted in uint32 vector

    // Buffer options
    float line_spacing_;    // Line spacing coeff
    BufferStyle style_;     // Style
    BufferColors color_;    // Colors
    hb_segment_properties_t properties_;    // Language, script, direction
    std::vector<hb_codepoint_t> word_dividers_; // Word dividers (glyph indexes)

    // Internal work
    hb_buffer_Ptr buffer_;      // HarfBuzz buffer
    unsigned int glyph_count_;  // Total number of glyphs in buffer_
    std::vector<hb_glyph_info_t> glyph_info_;       // Each glyph's informations
    std::vector<hb_glyph_position_t> glyph_pos_;    // Each glpyh's relative position
};
using Buffer_Ptr    = std::unique_ptr<Buffer>;
using Buffer_Shared = std::shared_ptr<Buffer>;
using Buffers       = std::vector<Buffer_Ptr>;
using Buffer_it     = Buffers::iterator;
using Buffer_cit    = Buffers::const_iterator;

SSS_TR_END__