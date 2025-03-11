#ifndef SSS_TR_BUFFER_HPP
#define SSS_TR_BUFFER_HPP

#include "Font.hpp"
#include "Text-Rendering/Area.hpp"

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

struct TextPart;

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


struct BufferInfo : public TextPart {
    std::vector<GlyphInfo> glyphs;  // Glyph infos
    std::locale locale; // Locale
};

class BufferInfoVector : public std::vector<BufferInfo> {
public:
    inline size_t glyphCount() const noexcept { return _glyph_count; };
    inline std::string getDirection() const noexcept { return _direction; };
    inline bool isLTR() const noexcept { return _direction == "ltr"; };
    GlyphInfo const& getGlyph(size_t cursor) const;
    BufferInfo const& getBuffer(size_t cursor) const;
    char32_t const& getChar(size_t cursor) const;
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
    
    // From TextPart
    Buffer(TextPart const& part);
    // Destructor
    ~Buffer();

// --- Basic functions ---

    void set(TextPart const& part);
    void changeString(std::u32string const& str);
    void changeFormat(Format const& fmt);

    uint32_t getClusterIndex(size_t cursor) const;

    // Insert text at given position
    void insertText(std::u32string const& str, size_t cursor);
    void insertText(std::string const& str, size_t cursor);
    // Deletes text, from first (included) to last (excluded)
    void deleteText(size_t first, size_t last);

    inline size_t glyphCount() const noexcept { return _info.glyphs.size(); };

    inline std::u32string const& getString() const noexcept { return _info.str; };
    inline Format const& getFormat() const noexcept { return _info.fmt; };

    inline BufferInfo const& getInfo() const noexcept { return _info; };

private:

    HB_Buffer_Ptr _buffer;  // HarfBuzz buffer
    BufferInfo _info;       // Buffer informations

    hb_segment_properties_t _properties;    // HB presets : lng, script, direction
    std::vector<uint32_t> _wd_indexes;      // Word dividers as glyph indexes

    // Modifies internal options
    void _formatChanged();

    void _updateBuffer();
    // Shapes the buffer and retrieve its informations
    void _shape();
    // Loads needed glyphs
    void _loadGlyphs();
};

INTERNAL_END;
SSS_TR_END;

#endif // SSS_TR_BUFFER_HPP