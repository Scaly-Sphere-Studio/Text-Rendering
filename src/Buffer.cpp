#include "SSS/Text-Rendering/Buffer.hpp"
#include "SSS/Text-Rendering/TextArea.hpp"

__SSS_TR_BEGIN
__INTERNAL_BEGIN

GlyphInfo const& BufferInfoVector::getGlyph(size_t cursor) const try
{
    for (BufferInfo const& buffer_info : *this) {
        if (buffer_info.glyphs.size() > cursor) {
            return buffer_info.glyphs.at(cursor);
        }
        cursor -= buffer_info.glyphs.size();
    }
    throw_exc(ERR_MSG::OUT_OF_BOUND);
}
__CATCH_AND_RETHROW_METHOD_EXC

_internal::BufferInfo const& BufferInfoVector::getBuffer(size_t cursor) const try
{
    for (BufferInfo const& buffer_info : *this) {
        if (buffer_info.glyphs.size() > cursor) {
            return buffer_info;
        }
        cursor -= buffer_info.glyphs.size();
    }
    throw_exc(ERR_MSG::OUT_OF_BOUND);
}
__CATCH_AND_RETHROW_METHOD_EXC

void BufferInfoVector::update(std::vector<Buffer::Shared> buffers)
{
    _glyph_count = 0;
    clear();
    reserve(buffers.size());
    for (Buffer::Shared const& buffer : buffers) {
        emplace_back(buffer->_buffer_info);
        _glyph_count += buffer->glyphCount();
    }
}

void BufferInfoVector::clear() noexcept
{
    _glyph_count = 0;
    vector::clear();
}

__INTERNAL_END

    // --- Constructor & Destructor ---

// Constructor, creates a HarfBuzz buffer, and shapes it with given parameters.
Buffer::Buffer(std::u32string const& str, TextOpt const& opt) try
    : _opt(opt)
{
    // Create buffer (and reference it to prevent early deletion)
    _buffer.reset(hb_buffer_reference(hb_buffer_create()));
    if (hb_buffer_allocation_successful(_buffer.get()) == 0) {
        throw_exc("HarfBuzz buffer allocation failed.");
    }

    // Fill contents
    _str = str;
    _buffer_info.str = _str;
    _changeOptions(opt);
    _updateBuffer();

    __LOG_CONSTRUCTOR
}
__CATCH_AND_RETHROW_METHOD_EXC

// Destructor
Buffer::~Buffer() noexcept
{
    __LOG_DESTRUCTOR
}

Buffer::Shared Buffer::create(std::u32string const& str, TextOpt const& opt)
{
    return Shared(new Buffer(str, opt));
}

Buffer::Shared Buffer::create(std::string const& str, TextOpt const& opt)
{
    return create(_internal::toU32str(str), opt);
}

// --- Basic functions ---

void Buffer::changeString(std::u32string const& str)
{
    _str = str;
    _buffer_info.str = _str;
    _updateBuffer();
}

void Buffer::changeString(std::string const& str)
{
    changeString(_internal::toU32str(str));
}

void Buffer::insertText(std::u32string const& str, size_t cursor)
{
    uint32_t index;
    if (cursor >= glyphCount()) {
        index = static_cast<uint32_t>(glyphCount());
    }
    else {
        index = _buffer_info.glyphs.at(cursor).info.cluster;
    }
    _str.insert(_str.cbegin() + index, str.cbegin(), str.cend());
    _buffer_info.str = _str;
    _updateBuffer();
}

void Buffer::insertText(std::string const& str, size_t cursor)
{
    insertText(_internal::toU32str(str), cursor);
}

void Buffer::changeOptions(TextOpt const& opt)
{
    _changeOptions(opt);
    _updateBuffer();
}

// Reshapes the buffer with given parameters
void Buffer::_changeOptions(TextOpt const& opt) try
{
    _opt = opt;
    _buffer_info.style = _opt.style;
    _buffer_info.color = _opt.color;
    _buffer_info.font = _opt.font;

    // Set buffer properties
    _properties.direction = hb_direction_from_string(opt.lng.direction.c_str(), -1);
    _properties.script = hb_script_from_string(opt.lng.script.c_str(), -1);
    _properties.language = hb_language_from_string(opt.lng.language.c_str(), -1);
    _locale = std::locale(opt.lng.language);
    _buffer_info.locale = _locale;

    // Convert word dividers to glyph indexes
    _wd_indexes.clear();
    _wd_indexes.reserve(opt.lng.word_dividers.size());
    for (char32_t const& c : opt.lng.word_dividers) {
        FT_UInt const index(FT_Get_Char_Index(opt.font->getFTFace().get(), c));
        _wd_indexes.push_back(index);
    }
}
__CATCH_AND_RETHROW_METHOD_EXC

void Buffer::_updateBuffer()
{
    _shape();
    _loadGlyphs();
    _notifyTextAreas();
}

void Buffer::_notifyTextAreas()
{
    Shared shared_this;
    try {
        shared_this = shared_from_this();
    }
    catch (std::bad_weak_ptr) {
        return;
    }
    for (auto it = TextArea::_instances.begin(); it != TextArea::_instances.cend();) {
        TextArea::Shared textarea = it->lock();
        if (textarea) {
            for (Shared const& buffer : textarea->_buffers) {
                if (shared_this == buffer) {
                    textarea->_draw = true;
                    textarea->_updateLines();
                    break;
                }
            }
            ++it;
        }
        else {
            it = TextArea::_instances.erase(it);
        }
    }
}

// Shapes the buffer and retrieve its informations
void Buffer::_shape() try
{
    _opt.font->setCharsize(_opt.style.charsize);
    // Add string to buffer
    uint32_t const* indexes = reinterpret_cast<uint32_t const*>(&_str[0]);
    int size = static_cast<int>(_str.size());
    hb_buffer_add_utf32(_buffer.get(), indexes, size, 0U, size);
    // Set properties
    hb_buffer_set_segment_properties(_buffer.get(), &_properties);
    // Shape buffer and retrieve informations
    hb_shape(_opt.font->getHBFont(_opt.style.charsize).get(), _buffer.get(), nullptr, 0);

    // Retrieve glyph informations
    unsigned int glyph_count = 0;
    hb_glyph_info_t const* info = hb_buffer_get_glyph_infos(_buffer.get(), &glyph_count);
    // Retrieve glyph positions
    hb_glyph_position_t const* pos = hb_buffer_get_glyph_positions(_buffer.get(), nullptr);

    _buffer_info.glyphs.clear();
    _buffer_info.glyphs.resize(glyph_count);
    for (size_t i = 0; i < glyph_count; ++i) {
        // Add references via constructor
        _internal::GlyphInfo& glyph = _buffer_info.glyphs.at(i);
        glyph.info = info[i];
        glyph.pos = pos[i];
        // Check if the glyph is a word divider
        for (hb_codepoint_t index : _wd_indexes) {
            if (glyph.info.codepoint == index) {
                glyph.is_word_divider = true;
                break;
            }
        }
    }

    // Now that we have all needed informations,
    // reset buffer to free HarfBuzz's internal cache
    // (this does NOT free the buffer itself, only its contents)
    hb_buffer_reset(_buffer.get());
}
__CATCH_AND_RETHROW_METHOD_EXC

void Buffer::_loadGlyphs()
{
    // Load glyphs
    int const outline_size = _opt.style.has_outline ? _opt.style.outline_size : 0;
    std::unordered_set<hb_codepoint_t> glyph_ids;
    for (_internal::GlyphInfo const& glyph : _buffer_info.glyphs) {
        glyph_ids.insert(glyph.info.codepoint);
    }
    for (hb_codepoint_t const& glyph_id: glyph_ids) {
        _opt.font->loadGlyph(glyph_id, _opt.style.charsize, outline_size);
    }
}

__SSS_TR_END
