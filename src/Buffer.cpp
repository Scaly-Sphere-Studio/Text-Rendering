#include "SSS/Text-Rendering/Buffer.h"

SSS_TR_BEGIN__

// Constructor, creates a HarfBuzz buffer, and shapes it with given parameters.
Buffer::Buffer(Font_Shared font, std::u32string const& str, BufferOpt const& opt) try
    : buffer_(nullptr, hb_buffer_destroy)
{
    // Create buffer (and reference it to prevent early deletion)
    buffer_.reset(hb_buffer_reference(hb_buffer_create()));
    if (hb_buffer_allocation_successful(buffer_.get()) == 0) {
        throw_exc("HarfBuzz buffer allocation failed.");
    }

    // Fill contents
    changeContents(font, str, opt);
    
    LOG_CONSTRUCTOR__
}
CATCH_AND_RETHROW_METHOD_EXC__

// Destructor
Buffer::~Buffer() noexcept
{
    LOG_DESTRUCTOR__
}

// Reshapes the buffer with given parameters
void Buffer::changeContents(Font_Shared font, std::u32string const& str,
    BufferOpt const& opt) try
{
    line_spacing_ = opt.line_spacing * 1.5f;
    style_ = opt.style;
    color_ = opt.color;
    font_ = font;

    // Convert string to uint32 vector, as HarfBuzz only handles that
    indexes_.clear();
    indexes_.reserve(str.length());
    for (char32_t const& c : str) {
        indexes_.push_back(static_cast<uint32_t>(c));
    }

    // Set buffer properties
    properties_.direction = hb_direction_from_string(opt.lng.direction.c_str(), -1);
    properties_.script = hb_script_from_string(opt.lng.script.c_str(), -1);
    properties_.language = hb_language_from_string(opt.lng.language.c_str(), -1);

    // Convert word dividers to glyph indexes
    word_dividers_.clear();
    word_dividers_.reserve(opt.word_dividers.length());
    for (char32_t const& c : opt.word_dividers) {
        FT_UInt const index(FT_Get_Char_Index(font->getFTFace().get(), c));
        word_dividers_.push_back(index);
    }

    // Shape glyphs
    shape_();
}
CATCH_AND_RETHROW_METHOD_EXC__

// Reshapes the buffer. To be called when the font charsize changes, for example
void Buffer::reshape() try
{
    shape_();
}
CATCH_AND_RETHROW_METHOD_EXC__


// Returns the number of glyphs in the buffer
size_t Buffer::size() const noexcept
{
    return static_cast<size_t>(glyph_count_);
}

// Returns a structure filled with informations of a given glyph.
// Throws an exception if cursor is out of bound.
GlyphInfo Buffer::at(size_t cursor) const try
{
    // Ensure the range of given argument is correct
    if (cursor > glyph_count_) {
        throw_exc(get_error(METHOD__, OUT_OF_BOUND));
    }

    // Fill glyph info
    GlyphInfo ret(
        glyph_info_.at(cursor),
        glyph_pos_.at(cursor),
        line_spacing_,
        style_,
        color_,
        font_
    );
    // Check if the glyph is a word divider
    for (hb_codepoint_t index : word_dividers_) {
        if (ret.info.codepoint == index) {
            ret.is_word_divider = true;
            break;
        }
    }
    // Return infos
    return ret;
}
CATCH_AND_RETHROW_METHOD_EXC__


void Buffer::shape_() try
{
    // Add string to buffer
    hb_buffer_add_utf32(buffer_.get(), &indexes_.at(0),
        static_cast<int>(indexes_.size()), 0U, static_cast<int>(indexes_.size()));
    // Set properties
    hb_buffer_set_segment_properties(buffer_.get(), &properties_);
    // Shape buffer and retrieve informations
    hb_shape(font_->getHBFont().get(), buffer_.get(), nullptr, 0);

    // Retrieve glyph informations
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer_.get(), &glyph_count_);
    glyph_info_.clear();
    glyph_info_.insert(glyph_info_.cbegin(), info, &info[glyph_count_]);

    // Retrieve glyph positions
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buffer_.get(), nullptr);
    glyph_pos_.clear();
    glyph_pos_.insert(glyph_pos_.cbegin(), pos, &pos[glyph_count_]);

    // Now that we have all needed informations,
    // reset buffer to free HarfBuzz's internal cache
    // (this does NOT free the buffer itself, only its contents)
    hb_buffer_reset(buffer_.get());
}
CATCH_AND_RETHROW_METHOD_EXC__

SSS_TR_END__