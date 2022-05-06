#include "SSS/Text-Rendering/_internal/FontSize.hpp"
#include "SSS/Text-Rendering/Globals.hpp"

SSS_TR_BEGIN;
INTERNAL_BEGIN;

// Constructor, throws if invalid charsize
FontSize::FontSize(FT_Face_Ptr const& ft_face, int charsize) try
    : _charsize(charsize), _ft_face(ft_face)
{
    if (charsize <= 0) {
        throw_exc("negative charsize not allowed.");
    }
    // Set charsize
    setCharsize();
    // Create HarfBuzz font from FreeType font face.
    _hb_font.reset(hb_ft_font_create_referenced(ft_face.get()));
    // Create Stroker
    FT_Stroker stroker;
    FT_Error error = FT_Stroker_New(Lib::getPtr().get(), &stroker);
    THROW_IF_FT_ERROR("FT_Stroker_New()");
    _stroker.reset(stroker);

    if (Log::TR::Fonts::query(Log::TR::Fonts::get().life_state)) {
        char buff[256];
        sprintf_s(buff, "Loaded '%s' -> size %03d", _ft_face->family_name, _charsize);
        LOG_TR_MSG(buff);
    }
}
CATCH_AND_RETHROW_METHOD_EXC;

// Destructor. Logs
FontSize::~FontSize()
{
    _originals.clear();
    _outlined.clear();
    _hb_font.release();
    _stroker.release();
    
    if (Log::TR::Fonts::query(Log::TR::Fonts::get().life_state)) {
        char buff[256];
        sprintf_s(buff, "Unloaded '%s' -> size %03d", _ft_face->family_name, _charsize);
        LOG_TR_MSG(buff);
    }
}

    // --- Glyph functions ---

static FT_Error _convertGlyph(FT_Glyph ft_glyph, Bitmap& bitmap)
{
    // Convert glyph to bitmap.
    // This frees the glyph (when last parameter is set to true) and allocates a bitmap
    FT_Error error = FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, nullptr, true);
    if (error != 0) {
        return error;
    }

    FT_BitmapGlyph ft_bitmap = (FT_BitmapGlyph)ft_glyph;

    bitmap.pen_left = ft_bitmap->left;
    bitmap.pen_top = ft_bitmap->top;

    bitmap.width = abs(ft_bitmap->bitmap.pitch);
    bitmap.height = ft_bitmap->bitmap.rows;
    bitmap.bpp = ft_bitmap->bitmap.width == 0 ? 0 : bitmap.width / ft_bitmap->bitmap.width;

    bitmap.buffer = std::vector<unsigned char>(
        ft_bitmap->bitmap.buffer,
        ft_bitmap->bitmap.buffer + (bitmap.width * bitmap.height)
    );
    bitmap.pixel_mode = ft_bitmap->bitmap.pixel_mode;

    // Free FT allocated bitmap
    FT_Done_Glyph(ft_glyph);

    return false;
}

void FontSize::setCharsize()
{
    // Get DPI
    FT_UInt hdpi, vdpi;
    getDPI(hdpi, vdpi);
    // Set charsize
    FT_Error error = FT_Set_Char_Size(_ft_face.get(), _charsize << 6, 0, hdpi, vdpi);
    THROW_IF_FT_ERROR("FT_Set_Char_Size()");
}

// Loads the given glyph, and its ouline if outline_size > 0
bool FontSize::loadGlyph(FT_UInt glyph_index, int outline_size) try
{
    // Check if glyph is already loaded
    if (_originals.count(glyph_index) != 0 && (outline_size == 0
         || (_outlined.count(outline_size) != 0
            && _outlined.at(outline_size).count(glyph_index) != 0)))
    {
        return false;
    }
    // Set charsize
    setCharsize();

    // Load glyph
    FT_Error error = FT_Load_Glyph(_ft_face.get(), glyph_index, FT_LOAD_DEFAULT);
    LOG_FT_ERROR_AND_RETURN("FT_Load_Glyph()", true);

    // Retrieve glyph
    FT_Glyph original;
    error = FT_Get_Glyph(_ft_face->glyph, &original);
    LOG_FT_ERROR_AND_RETURN("FT_Get_Glyph()", true);

    // Load its outline if needed
    FT_Glyph outlined = original;
    if (outline_size > 0) {
        // Update stroker if needed
        if (outline_size != _last_outline_size) {
            _last_outline_size = outline_size;
            FT_Stroker_Set(_stroker.get(), ((_charsize * outline_size) << 6) / 300,
                FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
        }

        // Create a stroked variant of the original glyph
        FT_Error error = FT_Glyph_Stroke(&outlined, _stroker.get(), false);
        LOG_FT_ERROR_AND_RETURN("FT_Glyph_Stroke()", true);
    }

    // Convert the glyph to bitmap, if needed
    if (_originals.count(glyph_index) == 0) {
        error = _convertGlyph(original, _originals[glyph_index]);
        LOG_FT_ERROR_AND_RETURN("FT_Glyph_To_Bitmap()", true);
    }

    // Store outline bitmap if needed
    if (outline_size > 0) {
        // Convert the glyph to bitmap
        error = _convertGlyph(outlined, _outlined[outline_size][glyph_index]);
        LOG_FT_ERROR_AND_RETURN("FT_Glyph_To_Bitmap()", true);
    }

    if (Log::TR::Fonts::query(Log::TR::Fonts::get().glyph_load)) {
        char buff[256];
        sprintf_s(buff, "Loaded '%s' -> size %03d -> glyph id '%u'",
            _ft_face->family_name, _charsize, glyph_index);
        LOG_TR_MSG(buff);
    }

    return false;
}
CATCH_AND_RETHROW_METHOD_EXC;

// Returns the corresponding glyph's bitmap. Throws if not found.
Bitmap const& FontSize::getGlyphBitmap(FT_UInt glyph_index) const try
{
    if (_originals.count(glyph_index) == 0) {
        throw_exc("No glyph found for given index.");
    }
    // Retrieve bitmap from cache
    return _originals.at(glyph_index);
}
CATCH_AND_RETHROW_METHOD_EXC;

// Returns the corresponding glyph outline's bitmap. Throws if not found.
Bitmap const& FontSize::getOutlineBitmap(FT_UInt glyph_index, int outline_size) const try
{
    if (_outlined.count(outline_size) == 0) {
        throw_exc("No map found for given outline size.");
    }
    if (_outlined.at(outline_size).count(glyph_index) == 0) {
        throw_exc("No glyph found for given index & outline size.");
    }
    // Retrieve bitmap from cache
    return _outlined.at(outline_size).at(glyph_index);
}
CATCH_AND_RETHROW_METHOD_EXC;

INTERNAL_END;
SSS_TR_END