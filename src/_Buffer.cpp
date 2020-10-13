#include "SSS/Text-Rendering/_Buffer.hpp"

__SSS_TR_BEGIN
__INTERNAL_BEGIN

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
    changeContents(str, opt);

    __LOG_CONSTRUCTOR
}
__CATCH_AND_RETHROW_METHOD_EXC

// Destructor
Buffer::~Buffer() noexcept
{
    __LOG_DESTRUCTOR
}

    // --- Basic functions ---

// Reshapes the buffer with given parameters
void Buffer::changeContents(std::u32string const& str, TextOpt const& opt) try
{
    _opt = opt;

    // Convert string to uint32 vector, as HarfBuzz only handles that
    _indexes.clear();
    _indexes.reserve(str.length());
    for (char32_t const& c : str) {
        _indexes.push_back(static_cast<uint32_t>(c));
    }

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

    // Shape glyphs
    _shape();
}
__CATCH_AND_RETHROW_METHOD_EXC

// Reshapes the buffer. To be called when the font charsize changes, for example
void Buffer::reshape() try
{
    _shape();
}
__CATCH_AND_RETHROW_METHOD_EXC

    // --- Get functions ---

// Returns the number of glyphs in the buffer
size_t Buffer::size() const noexcept
{
    return static_cast<size_t>(_glyph_count);
}

// Returns a structure filled with informations of a given glyph.
// Throws an exception if cursor is out of bound.
GlyphInfo Buffer::at(size_t cursor) const try
{
    // Ensure the range of given argument is correct
    if (cursor > _glyph_count) {
        throw_exc(ERR_MSG::OUT_OF_BOUND);
    }

    // Fill glyph info
    GlyphInfo ret(
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

    // --- Private Functions ---

// Shapes the buffer and retrieve its informations
void Buffer::_shape() try
{
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

__INTERNAL_END
__SSS_TR_END