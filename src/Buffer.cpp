#include "SSS/Text-Rendering/Buffer.hpp"
#include "SSS/Text-Rendering/TextArea.hpp"

__SSS_TR_BEGIN

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
    _updateBuffer();
}

void Buffer::changeString(std::string const& str)
{
    changeString(_internal::toU32str(str));
}

void Buffer::insertText(std::u32string const& str, size_t cursor)
{
    uint32_t index;
    if (cursor >= _glyph_count) {
        index = _glyph_count;
    }
    else {
        index = _at(cursor).info.cluster;
    }
    _str.insert(_str.cbegin() + index, str.cbegin(), str.cend());
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

// Returns a structure filled with informations of a given glyph.
// Throws an exception if cursor is out of bound.
_internal::GlyphInfo Buffer::_at(size_t cursor) const try
{
    // Ensure the range of given argument is correct
    if (cursor > _glyph_count) {
        throw_exc(ERR_MSG::OUT_OF_BOUND);
    }

    // Fill glyph info
    _internal::GlyphInfo ret(
        _glyph_info.at(cursor),
        _glyph_pos.at(cursor),
        _opt.style,
        _opt.color,
        _opt.font,
        _str,
        _locale
    );
    // Check if the glyph is a word divider
    for (hb_codepoint_t index : _wd_indexes) {
        if (ret.info.codepoint == index) {
            ret.is_word_divider = true;
            break;
        }
    }
    // Return infos
    return ret;
}
__CATCH_AND_RETHROW_METHOD_EXC

// Reshapes the buffer with given parameters
void Buffer::_changeOptions(TextOpt const& opt) try
{
    _opt = opt;

    // Set buffer properties
    _properties.direction = hb_direction_from_string(opt.lng.direction.c_str(), -1);
    _properties.script = hb_script_from_string(opt.lng.script.c_str(), -1);
    _properties.language = hb_language_from_string(opt.lng.language.c_str(), -1);
    _locale = std::locale(opt.lng.language);
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
                    textarea->_update_lines = true;
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
    _opt.font->useCharsize(_opt.style.charsize);
    // Add string to buffer
    uint32_t const* indexes = reinterpret_cast<uint32_t const*>(&_str[0]);
    int size = static_cast<int>(_str.size());
    hb_buffer_add_utf32(_buffer.get(), indexes, size, 0U, size);
    // Set properties
    hb_buffer_set_segment_properties(_buffer.get(), &_properties);
    // Shape buffer and retrieve informations
    hb_shape(_opt.font->getHBFont(_opt.style.charsize).get(), _buffer.get(), nullptr, 0);

    // Retrieve glyph informations
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(_buffer.get(), &_glyph_count);
    _glyph_info.clear();
    _glyph_info.insert(_glyph_info.cbegin(), info, &info[_glyph_count]);

    // Retrieve glyph positions
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(_buffer.get(), nullptr);
    _glyph_pos.clear();
    _glyph_pos.insert(_glyph_pos.cbegin(), pos, &pos[_glyph_count]);

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
    for (hb_glyph_info_t const& info : _glyph_info) {
        glyph_ids.insert(info.codepoint);
    }
    for (hb_codepoint_t const& glyph_id: glyph_ids) {
        _opt.font->loadGlyph(glyph_id, _opt.style.charsize, outline_size);
    }
}

__SSS_TR_END