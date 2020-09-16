#include "SSS/Text-Rendering/Font.h"

SSS_TR_BEGIN__

// Init static variables
FT_Library_Ptr Font::lib_{ nullptr };
size_t Font::instances_{ 0 };
FT_UInt Font::hdpi_{ 96 };
FT_UInt Font::vdpi_{ 0 };

// Constructor, inits FreeType if called for the first time.
// Creates a FreeType font face & a HarfBuzz font.
Font::Font(std::string const& path, int charsize, int outline_size) try
{
    FT_Error error;

    // Init FreeType if needed
    if (instances_ == 0) {
        FT_Library lib;
        error = FT_Init_FreeType(&lib);
        THROW_IF_FT_ERROR__("FT_Init_FreeType()");
        lib_.reset(lib);
        LOG_MSG__("FreeType library initialized");
    }

    // Create FreeType font face.
    FT_Face face;
    error = FT_New_Face(lib_.get(), path.c_str(), 0, &face);
    THROW_IF_FT_ERROR__("FT_New_Face()");
    face_.reset(face);

    // Create FreeType outline stroker.
    FT_Stroker stroker;
    error = FT_Stroker_New(lib_.get(), &stroker);
    THROW_IF_FT_ERROR__("FT_Stroker_New()");
    stroker_.reset(stroker);

    // Set character & outline sizes
    outline_size_ = outline_size;
    setCharsize(charsize);

    // Create HarfBuzz font from FreeType font face.
    hb_font_.reset(hb_ft_font_create_referenced(face_.get()));
    // Increment instances_
    ++instances_;

    LOG_CONSTRUCTOR__
}
CATCH_AND_RETHROW_METHOD_EXC__

// Destructor, quits FreeType if called from last remaining instance.
// Destroys the FreeType font face & the HarfBuzz font.
Font::~Font() noexcept
{
    --instances_;
    if (instances_ == 0) {
        LOG_MSG__("FreeType library quit");
    }
    LOG_DESTRUCTOR__
}

// Loads corresponding glyph.
bool Font::loadGlyph(FT_UInt index) try
{
    Bitmaps& bitmaps = bitmaps_[index];
    // Skip if already fully loaded
    if (bitmaps.original && (outline_size_ <= 0 || bitmaps.stroked)) {
        return false;
    }

    // Load glyph
    FT_Error error(FT_Load_Glyph(face_.get(), index, FT_LOAD_DEFAULT));
    LOG_FT_ERROR_AND_RETURN__("FT_Load_Glyph()", true);
    
    // Retrieve glyph
    FT_Glyph glyph;
    error = FT_Get_Glyph(face_->glyph, &glyph);
    LOG_FT_ERROR_AND_RETURN__("FT_Get_Glyph()", true);
    FT_Glyph_Ptr glyph_ptr(glyph);

    // Convert the glyph to bitmap
    error = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, nullptr, false);
    LOG_FT_ERROR_AND_RETURN__("FT_Glyph_To_Bitmap()", true);

    // Store bitmap
    bitmaps.original.reset((FT_BitmapGlyph)glyph);

    // Store its stroked variant if needed
    if (outline_size_ > 0) {
        // Create a stroked variant of the glyph
        FT_Glyph stroked(glyph_ptr.get());
        error = FT_Glyph_Stroke(&stroked, stroker_.get(), false);
        LOG_FT_ERROR_AND_RETURN__("FT_Glyph_Stroke()", true);

        // Convert the stroked variant to bitmap
        error = FT_Glyph_To_Bitmap(&stroked, FT_RENDER_MODE_NORMAL, nullptr, true);
        LOG_FT_ERROR_AND_RETURN__("FT_Glyph_To_Bitmap()", true);
        
        // Store bitmap
        bitmaps.stroked.reset((FT_BitmapGlyph)stroked);
    }

    return false;
}
CATCH_AND_RETHROW_METHOD_EXC__

// Clears out the internal glyph cache.
void Font::unloadGlyphs() noexcept
{
    bitmaps_.clear();
}

// Sets screen DPI for all instances (default: 96x96).
void Font::setDPI(FT_UInt hdpi, FT_UInt vdpi) noexcept
{
    hdpi_ = hdpi;
    vdpi_ = vdpi;
}

// Changes character size and recreates glyph cache.
void Font::setCharsize(int charsize) try
{
    // Change FT face charsize
    FT_Error error(FT_Set_Char_Size(face_.get(), charsize << 6, 0, hdpi_, vdpi_));
    THROW_IF_FT_ERROR__("FT_Set_Char_Size()");
    charsize_ = charsize;
    // Update HB font (if out of constructor).
    if (hb_font_) {
        hb_ft_font_changed(hb_font_.get());
    }
    // Update stroker & recreate glyph cache
    setOutline(outline_size_);
}
CATCH_AND_RETHROW_METHOD_EXC__

// Changes outline size and recreates glyph cache.
void Font::setOutline(int size) try
{
    // Ensure size stays in the 0 - 100 range
    if (size < 0) {
        size = 0;
    }
    else if (size > 100) {
        size = 100;
    }
    outline_size_ = size;

    // Update stroker if needed
    if (outline_size_ > 0) {
        FT_Stroker_Set(stroker_.get(), ((charsize_ * outline_size_) << 6) / 300,
            FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
    }
    // Recreate glyph cache
    for (auto it(bitmaps_.begin()); it != bitmaps_.end(); ++it) {
        // Free old glyphs
        it->second.original.reset();
        it->second.stroked.reset();
        // Create new glyph
        loadGlyph(it->first);
    }
}
CATCH_AND_RETHROW_METHOD_EXC__

// Returns corresponding loaded glyph.
// Throws exception if none corresponds to passed index.
Bitmaps const& Font::getBitmaps(FT_UInt index) const try
{
    if (bitmaps_.count(index) == 0) {
        throw_exc(NOTHING_FOUND);
    }
    // Retrieve glyph from cache
    return bitmaps_.at(index);
}
CATCH_AND_RETHROW_METHOD_EXC__

// Returns the internal FreeType font face.
FT_Face_Ptr const& Font::getFTFace() const noexcept
{
    return face_;
}

// Returns the internal HarfBuzz font.
HB_Font_Ptr const& Font::getHBFont() const noexcept
{
    return hb_font_;
}

// Returns current character size.
int Font::getCharsize() const noexcept
{
    return charsize_;
}

// Returns current outline size.
int Font::getOutline() const noexcept
{
    return outline_size_;
}

SSS_TR_END__