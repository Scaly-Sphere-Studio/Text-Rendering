#include "SSS/Text-Rendering/Buffer.hpp"
#include "SSS/Text-Rendering/TextArea.hpp"

__SSS_TR_BEGIN

    // --- Local function ---

static std::u32string toU32str(std::string str)
{
    return std::u32string(str.cbegin(), str.cend());
}

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
    _changeString(str);
    _changeOptions(opt);
    _shape();
    _loadGlyphs();

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
    return create(toU32str(str), opt);
}

    // --- Basic functions ---

void Buffer::changeString(std::u32string const& str)
{
    _changeString(str);
    _updateBuffer();
}

void Buffer::changeString(std::string const& str)
{
    changeString(toU32str(str));
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
        _opt.font
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
void Buffer::_changeString(std::u32string const& str) try
{
    // Convert string to uint32 vector, as HarfBuzz only handles that
    _indexes.clear();
    _indexes.reserve(str.length());
    for (char32_t const& c : str) {
        _indexes.push_back(static_cast<uint32_t>(c));
    }
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
    Shared shared_this = shared_from_this();
    for (auto it = TextArea::_instances.begin(); it != TextArea::_instances.cend();) {
        TextArea::Shared textarea = it->lock();
        if (textarea) {
            for (Shared const& buffer : textarea->_buffers) {
                if (shared_this == buffer) {
                    textarea->_update_format = true;
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
    hb_buffer_add_utf32(_buffer.get(), &_indexes.at(0),
        static_cast<int>(_indexes.size()), 0U, static_cast<int>(_indexes.size()));
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
    for (size_t i(0); i < _glyph_count; ++i) {
        _opt.font->loadGlyph(_at(i).info.codepoint, _opt.style.charsize, outline_size);
    }
}

__SSS_TR_END