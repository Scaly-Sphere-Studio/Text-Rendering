#pragma once

#include "SSS/Text-Rendering/Font.hpp"
#include "SSS/Text-Rendering/TextOpt.hpp"

__SSS_TR_BEGIN

    // --- Internal structures ---

__INTERNAL_BEGIN
// A structure filled with informations of a given glyph
struct GlyphInfo {
// --- Constructor ---
    GlyphInfo(
        hb_glyph_info_t const& info_,
        hb_glyph_position_t const& pos_,
        TextStyle const& style_,
        TextColors const& color_,
        Font::Shared const& font_,
        std::u32string const& str_,
        std::locale const& locale_
    ) noexcept :
        info(info_),
        pos(pos_),
        style(style_),
        color(color_),
        font(font_),
        str(str_),
        locale(locale_)
    {};

// --- Variables ---
    hb_glyph_info_t const& info;    // The glyph's informations
    hb_glyph_position_t const& pos; // The glyph's position
    TextStyle const& style;         // Style options
    TextColors const& color;        // Color options
    Font::Shared const& font;       // Font
    std::u32string const& str;      // Original string
    std::locale const& locale;      // Locale
    bool is_word_divider{ false };  // Whether the glyph is a word divider
};
__INTERNAL_END

    // --- Class ---

// Simplified HarfBuzz buffer
class Buffer : public std::enable_shared_from_this<Buffer> {
    friend class TextArea;
public:
// --- Aliases ---

    using Shared = std::shared_ptr<Buffer>;

// --- Constructor & Destructor ---
private:
    // Constructor, creates a HarfBuzz buffer, and shapes it with given parameters.
    Buffer(std::u32string const& str, TextOpt const& opt);
    
public:
    // Destructor
    ~Buffer() noexcept;

    static Shared create(std::u32string const& str, TextOpt const& opt);
    static Shared create(std::string const& str, TextOpt const& opt);

// --- Basic functions ---

    // Reshapes the buffer with given parameters
    void changeString(std::u32string const& str);
    void changeString(std::string const& str);
    // Insert text at given position
    void insertText(std::u32string const& str, size_t cursor);
    void insertText(std::string const& str, size_t cursor);
    // Reshapes the buffer with given parameters
    void changeOptions(TextOpt const& opt);

private:

    // Original string converted in uint32 vector
    // This is because HB doesn't handle CPP types,
    // and handles UTF32 with uint32_t arrays
    std::u32string _str;
    // Buffer options
    TextOpt _opt;
    
    _internal::HB_Buffer_Ptr _buffer;   // HarfBuzz buffer
    unsigned int _glyph_count;          // Total number of glyphs

    hb_segment_properties_t _properties;    // HB presets : lng, script, direction
    std::vector<uint32_t> _wd_indexes;      // Word dividers as glyph indexes
    std::locale _locale;

    std::vector<hb_glyph_info_t> _glyph_info;       // Glyphs informations
    std::vector<hb_glyph_position_t> _glyph_pos;    // Glpyhs relative position

    // Returns the number of glyphs in the buffer
    inline size_t _size() const noexcept { return static_cast<size_t>(_glyph_count); }
    // Returns a structure filled with informations of a given glyph.
    // Throws an exception if cursor is out of bound.
    _internal::GlyphInfo _at(size_t cursor) const;

    // Modifies internal options
    void _changeOptions(TextOpt const& opt);

    void _updateBuffer();
    void _notifyTextAreas();
    // Shapes the buffer and retrieve its informations
    void _shape();
    // Loads needed glyphs
    void _loadGlyphs();
};

__SSS_TR_END