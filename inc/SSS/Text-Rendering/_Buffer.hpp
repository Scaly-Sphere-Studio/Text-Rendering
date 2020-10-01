#pragma once

#include "SSS/Text-Rendering/Font.hpp"
#include "SSS/Text-Rendering/TextOpt.hpp"

SSS_TR_BEGIN__
INTERNAL_BEGIN__

    // --- Structures ---

// A structure filled with informations of a given glyph
struct GlyphInfo {
// --- Constructor ---
    GlyphInfo(
        hb_glyph_info_t const& info_,
        hb_glyph_position_t const& pos_,
        TextStyle const& style_,
        TextColors const& color_,
        Font::Shared const& font_
    ) noexcept :
        info(info_),
        pos(pos_),
        style(style_),
        color(color_),
        font(font_)
    {};

// --- Variables ---
    hb_glyph_info_t const& info;    // The glyph's informations
    hb_glyph_position_t const& pos; // The glyph's position
    TextStyle const& style;         // Style options
    TextColors const& color;        // Color options
    Font::Shared const& font;        // Color options
    bool is_word_divider{ false };  // Whether the glyph is a word divider
};

    // --- Class ---

// Simplified HarfBuzz buffer
class Buffer {

public:
// --- Aliases ---

    using Ptr       = std::unique_ptr<Buffer>;         // Unique ptr
    using vector    = std::vector<Buffer::Ptr>;        // Vector
    using it        = Buffer::vector::iterator;        // Iterator
    using cit       = Buffer::vector::const_iterator;  // Const iterator

// --- Constructor & Destructor ---

    // Constructor, creates a HarfBuzz buffer, and shapes it with given parameters.
    Buffer(std::u32string const& str, TextOpt const& opt);
    // Destructor
    ~Buffer() noexcept;

// --- Basic functions ---

    // Reshapes the buffer with given parameters
    void changeContents(std::u32string const& str, TextOpt const& opt);
    // Reshapes the buffer. To be called when the font charsize changes, for example
    void reshape();

// --- Get functions ---

    // Returns the number of glyphs in the buffer
    size_t size() const noexcept;
    // Returns a structure filled with informations of a given glyph.
    // Throws an exception if cursor is out of bound.
    GlyphInfo at(size_t cursor) const;

private:
// --- Private Functions ---

    // Shapes the buffer and retrieve its informations
    void shape_();

// --- Constructor arguments ---
    
    // Original string converted in uint32 vector
    // This is because HB doesn't handle CPP types,
    // and handles UTF32 with uint32_t arrays
    std::vector<uint32_t> indexes_;
    // Buffer options
    TextOpt opt_;
    
// --- HarfBuzz variables ---

    HB_Buffer_Ptr buffer_;      // HarfBuzz buffer
    unsigned int glyph_count_;  // Total number of glyphs

    hb_segment_properties_t properties_;    // HB presets : lng, script, direction
    std::vector<uint32_t> wd_indexes_;      // Word dividers as glyph indexes

    std::vector<hb_glyph_info_t> glyph_info_;       // Glyphs informations
    std::vector<hb_glyph_position_t> glyph_pos_;    // Glpyhs relative position
};

INTERNAL_END__
SSS_TR_END__