#include "Buffer.hpp"

SSS_TR_BEGIN;
INTERNAL_BEGIN;

GlyphInfo const& BufferInfoVector::getGlyph(size_t cursor) const try
{
    if (_glyph_count == 0)
        throw_exc("Empty buffer");
    for (BufferInfo const& buffer_info : *this) {
        if (buffer_info.glyphs.size() > cursor) {
            return buffer_info.glyphs.at(cursor);
        }
        cursor -= buffer_info.glyphs.size();
    }
    return back().glyphs.back();
}
CATCH_AND_RETHROW_METHOD_EXC;

_internal::BufferInfo const& BufferInfoVector::getBuffer(size_t cursor) const try
{
    if (empty())
        throw_exc("Empty buffer list");
    for (BufferInfo const& buffer_info : *this) {
        if (buffer_info.glyphs.size() > cursor) {
            return buffer_info;
        }
        cursor -= buffer_info.glyphs.size();
    }
    return back();
}
CATCH_AND_RETHROW_METHOD_EXC;

char32_t const& BufferInfoVector::getChar(size_t cursor) const try
{
    BufferInfo const& buff = getBuffer(cursor);
    GlyphInfo const& glyph = getGlyph(cursor);
    return buff.str.at(glyph.info.cluster);
}
CATCH_AND_RETHROW_METHOD_EXC;

std::u32string BufferInfoVector::getString() const
{
    std::u32string str;
    for (BufferInfo const& buffer_info : *this) {
        str += buffer_info.str;
    }
    return str;
}

void BufferInfoVector::update(std::vector<Buffer::Ptr> const& buffers)
{
    _glyph_count = 0;
    clear();
    reserve(buffers.size());
    for (Buffer::Ptr const& ptr : buffers) {
        _internal::Buffer& buffer = *ptr;
        emplace_back(buffer._info);
        _glyph_count += buffer.glyphCount();
    }
    if (!empty())
        _direction = front().fmt.lng_direction;
}

void BufferInfoVector::clear() noexcept
{
    _glyph_count = 0;
    vector::clear();
}


    // --- Constructor & Destructor ---

// Constructor, creates a HarfBuzz buffer, and shapes it with given parameters.
Buffer::Buffer(Format const& opt) try
{
    // Create buffer (and reference it to prevent early deletion)
    _buffer.reset(hb_buffer_reference(hb_buffer_create()));
    if (hb_buffer_allocation_successful(_buffer.get()) == 0) {
        throw_exc("HarfBuzz buffer allocation failed.");
    }

    _changeFormat(opt);

    if (Log::TR::Buffers::query(Log::TR::Buffers::get().life_state)) {
        char buff[256];
        sprintf_s(buff, "Created an internal Buffer.");
        LOG_TR_MSG(buff);
    }
}
CATCH_AND_RETHROW_METHOD_EXC;

// Destructor
Buffer::~Buffer()
{
    if (Log::TR::Buffers::query(Log::TR::Buffers::get().life_state)) {
        char buff[256];
        sprintf_s(buff, "Deleted an internal Buffer.");
        LOG_TR_MSG(buff);
    }
}

// --- Basic functions ---

void Buffer::changeString(std::u32string const& str)
{
    _info.str = str;
    _updateBuffer();
}

void Buffer::changeString(std::string const& str)
{
    changeString(strToStr32(str));
}

uint32_t Buffer::_getClusterIndex(size_t cursor)
{
    if (cursor >= glyphCount()) {
        if (glyphCount())
            return _info.glyphs.back().info.cluster + 1;
        return 0;
    }
    return _info.glyphs.at(cursor).info.cluster;
}

void Buffer::insertText(std::u32string const& str, size_t cursor)
{
    uint32_t const index = _getClusterIndex(cursor);
    _info.str.insert(_info.str.cbegin() + index, str.cbegin(), str.cend());
    _updateBuffer();
}

void Buffer::insertText(std::string const& str, size_t cursor)
{
    insertText(strToStr32(str), cursor);
}

void Buffer::deleteText(size_t cursor, size_t count)
{
    if (count == 0)
        return;
    size_t const first = _getClusterIndex(cursor);
    size_t const last = first + count < _info.str.size() ? first + count : _info.str.size();
    _info.str.erase(_info.str.cbegin() + first, _info.str.cbegin() + last);
    _updateBuffer();
}

void Buffer::changeFormat(Format const& opt)
{
    _changeFormat(opt);
    _updateBuffer();
}

// Reshapes the buffer with given parameters
void Buffer::_changeFormat(Format const& opt) try
{
    // Retrieve Font (must be loaded)
    Font& font = Lib::getFont(opt.font);

    _info.fmt = opt;
    for (char& c : _info.fmt.lng_direction)
        c = std::tolower(c);

    // Set buffer properties
    _properties.direction = hb_direction_from_string(opt.lng_direction.c_str(), -1);
    _properties.script = hb_script_from_string(opt.lng_script.c_str(), -1);
    _properties.language = hb_language_from_string(opt.lng_tag.c_str(), -1);
    _info.locale = std::locale(opt.lng_tag);

    // Convert word dividers to glyph indexes
    _wd_indexes.clear();
    _wd_indexes.reserve(opt.word_dividers.size());
    for (char32_t const& c : opt.word_dividers) {
        FT_UInt const index(FT_Get_Char_Index(font.getFTFace(), c));
        _wd_indexes.push_back(index);
    }
}
CATCH_AND_RETHROW_METHOD_EXC;

void Buffer::_updateBuffer()
{
    _shape();
    _loadGlyphs();
}

// Shapes the buffer and retrieve its informations
void Buffer::_shape() try
{
    // Retrieve Font (must be loaded)
    Font& font = Lib::getFont(_info.fmt.font);

    font.setCharsize(_info.fmt.charsize);
    // Add string to buffer
    uint32_t const* indexes = reinterpret_cast<uint32_t const*>(&_info.str[0]);
    int size = static_cast<int>(_info.str.size());
    hb_buffer_add_utf32(_buffer.get(), indexes, size, 0U, size);
    // Set properties
    hb_buffer_set_segment_properties(_buffer.get(), &_properties);
    hb_buffer_set_cluster_level(_buffer.get(), HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
    // Shape buffer and retrieve informations
    hb_shape(font.getHBFont(_info.fmt.charsize), _buffer.get(), nullptr, 0);

    // Retrieve glyph informations
    unsigned int glyph_count = 0;
    hb_glyph_info_t const* info = hb_buffer_get_glyph_infos(_buffer.get(), &glyph_count);
    // Retrieve glyph positions
    hb_glyph_position_t const* pos = hb_buffer_get_glyph_positions(_buffer.get(), nullptr);

    _info.glyphs.clear();
    _info.glyphs.resize(glyph_count);
    for (size_t i = 0; i < glyph_count; ++i) {
        // Reverse if RTL
        size_t const index = _info.fmt.lng_direction == "ltr" ? i : (glyph_count - (i + 1));
        _internal::GlyphInfo& glyph = _info.glyphs.at(index);
        glyph.info = info[i];
        glyph.pos = pos[i];
        // Check if the glyph is a word divider
        for (hb_codepoint_t index : _wd_indexes) {
            if (glyph.info.codepoint == index) {
                glyph.is_word_divider = true;
                break;
            }
        }
        // Check if the glyph is a new line
        if (_info.str.at(glyph.info.cluster) == '\n') {
            glyph.is_new_line = true;
            glyph.is_word_divider = true;
        }
    }
    // Now that we have all needed informations,
    // reset buffer to free HarfBuzz's internal cache
    // (this does NOT free the buffer itself, only its contents)
    hb_buffer_reset(_buffer.get());
}
CATCH_AND_LOG_METHOD_EXC;

void Buffer::_loadGlyphs()
{
    // Retrieve Font (must be loaded)
    Font& font = Lib::getFont(_info.fmt.font);

    // Load glyphs
    int const outline_size = _info.fmt.has_outline ? _info.fmt.outline_size : 0;
    std::unordered_set<hb_codepoint_t> glyph_ids;
    for (_internal::GlyphInfo const& glyph : _info.glyphs) {
        glyph_ids.insert(glyph.info.codepoint);
    }
    for (hb_codepoint_t const& glyph_id: glyph_ids) {
        font.loadGlyph(glyph_id, _info.fmt.charsize, outline_size);
    }
}

INTERNAL_END;
SSS_TR_END;
