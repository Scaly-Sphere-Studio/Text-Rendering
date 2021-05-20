#pragma once

#include "SSS/Text-Rendering/Font.hpp"
#include "SSS/Text-Rendering/TextOpt.hpp"

__SSS_TR_BEGIN

    // --- Internal structures ---

__INTERNAL_BEGIN
// A structure filled with informations of a given glyph
struct GlyphInfo {
    hb_glyph_info_t info;       // The glyph's informations
    hb_glyph_position_t pos;    // The glyph's position
    bool is_word_divider{ false };  // Whether the glyph is a word divider
};

struct BufferInfo {
    std::vector<GlyphInfo> glyphs;  // Glyph infos
    TextStyle style;    // Style options
    TextColors color;   // Color options
    Font::Shared font;  // Font
    std::u32string str; // Original string
    std::locale locale; // Locale
};

class BufferInfoVector : public std::vector<BufferInfo> {
public:
    GlyphInfo const& getGlyph(size_t cursor);
    BufferInfo const& getBuffer(size_t cursor);
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

    _internal::BufferInfo _buffer_info;

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