#pragma once

#include "Font.hpp"
#include "../Format.hpp"

/** @file
 *  Defines internal glyph and kerning info classes.
 */

namespace SSS::Log::TR {
    /** Logging properties for SSS::TR internal buffers.*/
    struct Buffers : public LogBase<Buffers> {
        using LOG_STRUCT_BASICS(TR, Buffers);
        /** Logs both constructor and destructor.*/
        bool life_state = false;
    };
}

SSS_TR_BEGIN;
INTERNAL_BEGIN;
// Pre-declaration
class Buffer;

    // --- Internal structures ---

// A structure filled with informations of a given glyph
struct GlyphInfo {
    hb_glyph_info_t info{};         // The glyph's informations
    hb_glyph_position_t pos{};      // The glyph's position
    bool is_word_divider{ false };  // Whether the glyph is a word divider OR a \n
    bool is_new_line{ false };      // Whether the glyph is a \n (new line)
};

struct BufferInfo {
    std::vector<GlyphInfo> glyphs;  // Glyph infos
    std::u32string str; // Original string
    std::locale locale; // Locale
    Format fmt; // Format
};

class BufferInfoVector : public std::vector<BufferInfo> {
public:
    inline size_t glyphCount() const noexcept { return _glyph_count; };
    inline std::string getDirection() const noexcept { return _direction; };
    inline bool isLTR() const noexcept { return _direction == "ltr"; };
    GlyphInfo const& getGlyph(size_t cursor) const;
    BufferInfo const& getBuffer(size_t cursor) const;
    std::u32string getString() const;
    void update(std::vector<std::unique_ptr<Buffer>> const& buffers);
    void clear() noexcept;
private:
    size_t _glyph_count{ 0 };
    std::string _direction;
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

private:
    uint32_t _getClusterIndex(size_t cursor);

public:
    // Insert text at given position
    void insertText(std::u32string const& str, size_t cursor);
    void insertText(std::string const& str, size_t cursor);
    // Deletes text, from first (included) to last (excluded)
    void deleteText(size_t first, size_t last);
    // Reshapes the buffer with given parameters
    void changeFormat(Format const& opt);

    inline size_t glyphCount() const noexcept { return _buffer_info.glyphs.size(); };

    inline std::u32string getString() const noexcept { return _str; };
    inline Format getFormat() const noexcept { return _opt; };

    inline BufferInfo const& getInfo() const noexcept { return _buffer_info; };
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

INTERNAL_END;
SSS_TR_END;
