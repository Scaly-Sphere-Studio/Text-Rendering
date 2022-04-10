#pragma once

#include "Font.hpp"
#include "../Format.hpp"

__SSS_TR_BEGIN;
__INTERNAL_BEGIN;
// Pre-declaration
class Buffer;

    // --- Internal structures ---

// A structure filled with informations of a given glyph
struct GlyphInfo {
    hb_glyph_info_t info{};         // The glyph's informations
    hb_glyph_position_t pos{};      // The glyph's position
    bool is_word_divider{ false };  // Whether the glyph is a word divider
};

struct BufferInfo {
    std::vector<GlyphInfo> glyphs;  // Glyph infos
    Format::Style style;   // Style options
    Format::Color color;   // Color options
    std::string font;   // Font
    std::u32string str; // Original string
    std::locale locale; // Locale
};

class BufferInfoVector : public std::vector<BufferInfo> {
public:
    inline size_t glyphCount() const { return _glyph_count; };
    GlyphInfo const& getGlyph(size_t cursor) const;
    BufferInfo const& getBuffer(size_t cursor) const;
    void update(std::vector<std::unique_ptr<Buffer>> const& buffers);
    void clear() noexcept;
private:
    size_t _glyph_count{ 0 };
};

    // --- Main class ---

// Simplified HarfBuzz buffer
class Buffer {
    friend class BufferInfoVector;
public:
    using Ptr = std::unique_ptr<Buffer>;
// --- Constructor & Destructor ---
    
    // Constructor, creates a HarfBuzz buffer, and shapes it with given parameters.
    Buffer(Format const& opt);
    // Destructor
    ~Buffer();

// --- Basic functions ---

    // Reshapes the buffer with given parameters
    void changeString(std::u32string const& str);
    void changeString(std::string const& str);
    // Insert text at given position
    void insertText(std::u32string const& str, size_t cursor);
    void insertText(std::string const& str, size_t cursor);
    // Reshapes the buffer with given parameters
    void changeFormat(Format const& opt);

    inline size_t glyphCount() const noexcept { return _buffer_info.glyphs.size(); };

    inline std::u32string getString() const noexcept { return _str; };
    inline Format getFormat() const noexcept { return _opt; };

private:

    // Original string converted in uint32 vector
    // This is because HB doesn't handle CPP types,
    // and handles UTF32 with uint32_t arrays
    std::u32string _str;
    // Buffer options
    Format _opt;
    
    HB_Buffer_Ptr _buffer;   // HarfBuzz buffer
    BufferInfo _buffer_info; // Buffer informations

    hb_segment_properties_t _properties;    // HB presets : lng, script, direction
    std::vector<uint32_t> _wd_indexes;      // Word dividers as glyph indexes
    std::locale _locale;


    // Modifies internal options
    void _changeFormat(Format const& opt);

    void _updateBuffer();
    // Shapes the buffer and retrieve its informations
    void _shape();
    // Loads needed glyphs
    void _loadGlyphs();
};

__INTERNAL_END;
__SSS_TR_END;
