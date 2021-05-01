#include "SSS/Text-Rendering/_FontSize.hpp"

// Including Font in the .cpp to avoid include loop
#include "SSS/Text-Rendering/Font.hpp"

__SSS_TR_BEGIN
__INTERNAL_BEGIN

    // --- Constructor & Destructor ---

// Constructor, throws if invalid charsize
FontSize::FontSize(FT_Face_Ptr const& ft_face, int charsize) try
    : _charsize(charsize), _ft_face(ft_face)
{
    if (charsize <= 0) {
        throw_exc("negative charsize not allowed.");
    }
    // Set charsize
    _setCharsize();
    // Create HarfBuzz font from FreeType font face.
    _hb_font.reset(hb_ft_font_create_referenced(ft_face.get()));
    // Create Stroker
    FT_Stroker stroker;
    FT_Error error = FT_Stroker_New(Font::getFTLib().get(), &stroker);
    __THROW_IF_FT_ERROR("FT_Stroker_New()");
    _stroker.reset(stroker);

    __LOG_CONSTRUCTOR
}
__CATCH_AND_RETHROW_METHOD_EXC

// Destructor. Logs
FontSize::~FontSize()
{
    _originals.clear();
    _outlined.clear();
    _hb_font.release();
    _stroker.release();
    __LOG_DESTRUCTOR
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
    _setCharsize();

    // Load glyph
    FT_Error error = FT_Load_Glyph(_ft_face.get(), glyph_index, FT_LOAD_DEFAULT);
    __LOG_FT_ERROR_AND_RETURN("FT_Load_Glyph()", true);

    // Retrieve glyph
    FT_Glyph original;
    error = FT_Get_Glyph(_ft_face->glyph, &original);
    __LOG_FT_ERROR_AND_RETURN("FT_Get_Glyph()", true);

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
        __LOG_FT_ERROR_AND_RETURN("FT_Glyph_Stroke()", true);
    }

    // Convert the glyph to bitmap, if needed
    if (_originals.count(glyph_index) == 0) {
        error = _convertGlyph(original, _originals[glyph_index]);
        __LOG_FT_ERROR_AND_RETURN("FT_Glyph_To_Bitmap()", true);
    }

    // Store outline bitmap if needed
    if (outline_size > 0) {
        // Convert the glyph to bitmap
        error = _convertGlyph(outlined, _outlined[outline_size][glyph_index]);
        __LOG_FT_ERROR_AND_RETURN("FT_Glyph_To_Bitmap()", true);
    }

    return false;
}
__CATCH_AND_RETHROW_METHOD_EXC

// Returns the corresponding glyph's bitmap. Throws if not found.
Bitmap const& FontSize::getGlyphBitmap(FT_UInt glyph_index) const try
{
    if (_originals.count(glyph_index) == 0) {
        throw_exc(ERR_MSG::NOTHING_FOUND);
    }
    // Retrieve bitmap from cache
    return _originals.at(glyph_index);
}
__CATCH_AND_RETHROW_METHOD_EXC

// Returns the corresponding glyph outline's bitmap. Throws if not found.
Bitmap const& FontSize::getOutlineBitmap(FT_UInt glyph_index, int outline_size) const try
{
    if (_outlined.count(outline_size) == 0
        || _outlined.at(outline_size).count(glyph_index) == 0) {
        throw_exc(ERR_MSG::NOTHING_FOUND);
    }
    // Retrieve bitmap from cache
    return _outlined.at(outline_size).at(glyph_index);
}
__CATCH_AND_RETHROW_METHOD_EXC

void FontSize::_setCharsize()
{
    // Set charsize
    FT_Error error = FT_Set_Char_Size(_ft_face.get(), _charsize << 6, 0,
        Font::getHDPI(), Font::getVDPI());
    __THROW_IF_FT_ERROR("FT_Set_Char_Size()");
}

__INTERNAL_END
__SSS_TR_END