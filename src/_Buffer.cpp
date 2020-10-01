#include "SSS/Text-Rendering/_Buffer.hpp"

SSS_TR_BEGIN__
INTERNAL_BEGIN__

    // --- Constructor & Destructor ---

// Constructor, creates a HarfBuzz buffer, and shapes it with given parameters.
Buffer::Buffer(std::u32string const& str, TextOpt const& opt) try
    : opt_(opt)
{
    // Create buffer (and reference it to prevent early deletion)
    buffer_.reset(hb_buffer_reference(hb_buffer_create()));
    if (hb_buffer_allocation_successful(buffer_.get()) == 0) {
        throw_exc("HarfBuzz buffer allocation failed.");
    }

    // Fill contents
    changeContents(str, opt);

    LOG_CONSTRUCTOR__
}
CATCH_AND_RETHROW_METHOD_EXC__

// Destructor
Buffer::~Buffer() noexcept
{
    LOG_DESTRUCTOR__
}

    // --- Basic functions ---

// Reshapes the buffer with given parameters
void Buffer::changeContents(std::u32string const& str, TextOpt const& opt) try
{
    opt_ = opt;

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
    wd_indexes_.clear();
    wd_indexes_.reserve(opt.lng.word_dividers.size());
    for (char32_t const& c : opt.lng.word_dividers) {
        FT_UInt const index(FT_Get_Char_Index(opt.font->getFTFace().get(), c));
        wd_indexes_.push_back(index);
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

    // --- Get functions ---

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
        throw_exc(get_error(METHOD__, ERR_MSG::OUT_OF_BOUND));
    }

    // Fill glyph info
    GlyphInfo ret(
        glyph_info_.at(cursor),
        glyph_pos_.at(cursor),
        opt_.style,
        opt_.color,
        opt_.font
    );
    // Check if the glyph is a word divider
    for (hb_codepoint_t index : wd_indexes_) {
        if (ret.info.codepoint == index) {
            ret.is_word_divider = true;
            break;
        }
    }
    // Return infos
    return ret;
}
CATCH_AND_RETHROW_METHOD_EXC__

    // --- Private Functions ---

// Shapes the buffer and retrieve its informations
void Buffer::shape_() try
{
    // Add string to buffer
    hb_buffer_add_utf32(buffer_.get(), &indexes_.at(0),
        static_cast<int>(indexes_.size()), 0U, static_cast<int>(indexes_.size()));
    // Set properties
    hb_buffer_set_segment_properties(buffer_.get(), &properties_);
    // Shape buffer and retrieve informations
    hb_shape(opt_.font->getHBFont(opt_.style.charsize).get(), buffer_.get(), nullptr, 0);

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

INTERNAL_END__
SSS_TR_END__