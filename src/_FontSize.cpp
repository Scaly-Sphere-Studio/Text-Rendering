#include "SSS/Text-Rendering/_FontSize.hpp"

SSS_TR_BEGIN__
INTERNAL_BEGIN__

    // --- Constructor & Destructor ---

// Constructor, throws if invalid charsize
FontSize::FontSize(FT_Face_Ptr const& ft_face, int charsize) try
    : charsize_(charsize), ft_face_(ft_face)
{
    setCharsize_();
    // Create HarfBuzz font from FreeType font face.
    hb_font_.reset(hb_ft_font_create_referenced(ft_face.get()));
    // Create Stroker
    FT_Stroker stroker;
    FT_Error error = FT_Stroker_New(Font::lib_.get(), &stroker);
    THROW_IF_FT_ERROR__("FT_Stroker_New()");
    stroker_.reset(stroker);

    LOG_CONSTRUCTOR__
}
CATCH_AND_RETHROW_METHOD_EXC__

// Destructor. Logs
FontSize::~FontSize()
{
    LOG_DESTRUCTOR__
}

    // --- Glyph functions ---

// Loads the given glyph, and its ouline if outline_size > 0
bool FontSize::loadGlyph(FT_UInt glyph_index, int outline_size) try
{
    setCharsize_();
    // Load glyph
    if (loadOriginal_(glyph_index)) {
        return true;
    }
    // Load its outline
    if (loadOutlined_(glyph_index, outline_size)) {
        return true;
    }
    // Store its/their bitmap(s)
    return storeBitmaps_(glyph_index, outline_size);
}
CATCH_AND_RETHROW_METHOD_EXC__

// Returns the corresponding glyph's bitmap. Throws if not found.
FT_BitmapGlyph_Ptr const&
FontSize::getGlyphBitmap(FT_UInt glyph_index) const try
{
    if (originals_.count(glyph_index) == 0) {
        throw_exc(ERR_MSG::NOTHING_FOUND);
    }
    // Retrieve bitmap from cache
    return originals_.at(glyph_index).bitmap;
}
CATCH_AND_RETHROW_METHOD_EXC__

// Returns the corresponding glyph outline's bitmap. Throws if not found.
FT_BitmapGlyph_Ptr const&
FontSize::getOutlineBitmap(FT_UInt glyph_index, int outline_size) const try
{
    if (outlined_.count(outline_size) == 0
        || outlined_.at(outline_size).count(glyph_index) == 0) {
        throw_exc(ERR_MSG::NOTHING_FOUND);
    }
    // Retrieve bitmap from cache
    return outlined_.at(outline_size).at(glyph_index).bitmap;
}
CATCH_AND_RETHROW_METHOD_EXC__

    // --- Get functions ---

// Returns the corresponding HarfBuzz font
HB_Font_Ptr const& FontSize::getHBFont() const noexcept
{
    return hb_font_;
}

    // --- Static functions ---

// Change FT face charsize
void FontSize::setCharsize_()
{
    FT_Error error = FT_Set_Char_Size(ft_face_.get(), charsize_ << 6, 0,
        Font::hdpi_, Font::vdpi_);
    THROW_IF_FT_ERROR__("FT_Set_Char_Size()");
}

// Loads the original glyph. Returns true on error
bool FontSize::loadOriginal_(FT_UInt glyph_index) try
{
    FT_Glyph_Ptr& glyph = originals_[glyph_index].glyph;
    // Skip if already loaded
    if (glyph) {
        return false;
    }

    // Load glyph
    FT_Error error = FT_Load_Glyph(ft_face_.get(), glyph_index, FT_LOAD_DEFAULT);
    LOG_FT_ERROR_AND_RETURN__("FT_Load_Glyph()", true);

    // Retrieve glyph
    FT_Glyph ft_glyph;
    error = FT_Get_Glyph(ft_face_->glyph, &ft_glyph);
    LOG_FT_ERROR_AND_RETURN__("FT_Get_Glyph()", true);
    glyph.reset(ft_glyph);

    return false;
}
CATCH_AND_RETHROW_METHOD_EXC__

// Loads the glyph's outline. Returns true on error
bool FontSize::loadOutlined_(FT_UInt glyph_index, int outline_size) try
{
    // Skip if no outline is needed
    if (outline_size <= 0) {
        return false;
    }

    FT_Glyph_Ptr& glyph = outlined_[outline_size][glyph_index].glyph;
    // Skip if already loaded
    if (glyph) {
        return false;
    }

    // Update stroker if needed
    if (outline_size != last_outline_size_) {
        last_outline_size_ = outline_size;
        FT_Stroker_Set(stroker_.get(), ((charsize_ * outline_size) << 6) / 300,
            FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
    }

    // Create a stroked variant of the original glyph
    FT_Glyph ft_glyph(originals_.at(glyph_index).glyph.get());
    FT_Error error = FT_Glyph_Stroke(&ft_glyph, stroker_.get(), false);
    LOG_FT_ERROR_AND_RETURN__("FT_Glyph_Stroke()", true);
    glyph.reset(ft_glyph);

    return false;
}
CATCH_AND_RETHROW_METHOD_EXC__

// Stores the glyph and its ouline's bitmaps. Returns true on error
bool FontSize::storeBitmaps_(FT_UInt glyph_index, int outline_size) try
{
    // Retrieve the loaded glyph
    _Glyph& original = originals_.at(glyph_index);
    FT_Glyph ft_glyph = original.glyph.get();
    // Convert the glyph to bitmap
    FT_Error error = FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, nullptr, false);
    LOG_FT_ERROR_AND_RETURN__("FT_Glyph_To_Bitmap()", true);
    // Store bitmap
    original.bitmap.reset((FT_BitmapGlyph)ft_glyph);

    // Store outline bitmap if needed
    if (outline_size > 0) {
        // Retrieve the loaded glyph
        _Glyph& outlined = outlined_.at(outline_size).at(glyph_index);
        ft_glyph = outlined.glyph.get();
        // Convert the glyph to bitmap
        error = FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, nullptr, false);
        LOG_FT_ERROR_AND_RETURN__("FT_Glyph_To_Bitmap()", true);
        // Store bitmap
        outlined.bitmap.reset((FT_BitmapGlyph)ft_glyph);
    }

    return false;
}
CATCH_AND_RETHROW_METHOD_EXC__

INTERNAL_END__
SSS_TR_END__