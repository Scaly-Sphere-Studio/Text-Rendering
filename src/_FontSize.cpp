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
    __LOG_DESTRUCTOR
}

    // --- Glyph functions ---

// Loads the given glyph, and its ouline if outline_size > 0
bool FontSize::loadGlyph(FT_UInt glyph_index, int outline_size) try
{
    _setCharsize();
    // Load glyph
    if (_loadOriginal(glyph_index)) {
        return true;
    }
    // Load its outline
    if (_loadOutlined(glyph_index, outline_size)) {
        return true;
    }
    // Store its/their bitmap(s)
    return _storeBitmaps(glyph_index, outline_size);
}
__CATCH_AND_RETHROW_METHOD_EXC

// Returns the corresponding glyph's bitmap. Throws if not found.
FT_BitmapGlyph_Ptr const&
FontSize::getGlyphBitmap(FT_UInt glyph_index) const try
{
    if (_originals.count(glyph_index) == 0) {
        throw_exc(ERR_MSG::NOTHING_FOUND);
    }
    // Retrieve bitmap from cache
    return _originals.at(glyph_index).bitmap;
}
__CATCH_AND_RETHROW_METHOD_EXC

// Returns the corresponding glyph outline's bitmap. Throws if not found.
FT_BitmapGlyph_Ptr const&
FontSize::getOutlineBitmap(FT_UInt glyph_index, int outline_size) const try
{
    if (_outlined.count(outline_size) == 0
        || _outlined.at(outline_size).count(glyph_index) == 0) {
        throw_exc(ERR_MSG::NOTHING_FOUND);
    }
    // Retrieve bitmap from cache
    return _outlined.at(outline_size).at(glyph_index).bitmap;
}
__CATCH_AND_RETHROW_METHOD_EXC

    // --- Private functions ---

// Change FT face charsize
void FontSize::_setCharsize()
{
    FT_Error error = FT_Set_Char_Size(_ft_face.get(), _charsize << 6, 0,
        Font::getHDPI(), Font::getVDPI());
    __THROW_IF_FT_ERROR("FT_Set_Char_Size()");
}

// Loads the original glyph. Returns true on error
bool FontSize::_loadOriginal(FT_UInt glyph_index) try
{
    FT_Glyph_Ptr& glyph = _originals[glyph_index].glyph;
    // Skip if already loaded
    if (glyph) {
        return false;
    }

    // Load glyph
    FT_Error error = FT_Load_Glyph(_ft_face.get(), glyph_index, FT_LOAD_DEFAULT);
    __LOG_FT_ERROR_AND_RETURN("FT_Load_Glyph()", true);

    // Retrieve glyph
    FT_Glyph ft_glyph;
    error = FT_Get_Glyph(_ft_face->glyph, &ft_glyph);
    __LOG_FT_ERROR_AND_RETURN("FT_Get_Glyph()", true);
    glyph.reset(ft_glyph);

    return false;
}
__CATCH_AND_RETHROW_METHOD_EXC

// Loads the glyph's outline. Returns true on error
bool FontSize::_loadOutlined(FT_UInt glyph_index, int outline_size) try
{
    // Skip if no outline is needed
    if (outline_size <= 0) {
        return false;
    }

    FT_Glyph_Ptr& glyph = _outlined[outline_size][glyph_index].glyph;
    // Skip if already loaded
    if (glyph) {
        return false;
    }

    // Update stroker if needed
    if (outline_size != _last_outline_size) {
        _last_outline_size = outline_size;
        FT_Stroker_Set(_stroker.get(), ((_charsize * outline_size) << 6) / 300,
            FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
    }

    // Create a stroked variant of the original glyph
    FT_Glyph ft_glyph(_originals.at(glyph_index).glyph.get());
    FT_Error error = FT_Glyph_Stroke(&ft_glyph, _stroker.get(), false);
    __LOG_FT_ERROR_AND_RETURN("FT_Glyph_Stroke()", true);
    glyph.reset(ft_glyph);

    return false;
}
__CATCH_AND_RETHROW_METHOD_EXC

// Stores the glyph and its ouline's bitmaps. Returns true on error
bool FontSize::_storeBitmaps(FT_UInt glyph_index, int outline_size) try
{
    // Retrieve the loaded glyph
    _Glyph& original = _originals.at(glyph_index);
    FT_Glyph ft_glyph = original.glyph.get();
    // Convert the glyph to bitmap
    FT_Error error = FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, nullptr, false);
    __LOG_FT_ERROR_AND_RETURN("FT_Glyph_To_Bitmap()", true);
    // Store bitmap
    original.bitmap.reset((FT_BitmapGlyph)ft_glyph);

    // Store outline bitmap if needed
    if (outline_size > 0) {
        // Retrieve the loaded glyph
        _Glyph& outlined = _outlined.at(outline_size).at(glyph_index);
        ft_glyph = outlined.glyph.get();
        // Convert the glyph to bitmap
        error = FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, nullptr, false);
        __LOG_FT_ERROR_AND_RETURN("FT_Glyph_To_Bitmap()", true);
        // Store bitmap
        outlined.bitmap.reset((FT_BitmapGlyph)ft_glyph);
    }

    return false;
}
__CATCH_AND_RETHROW_METHOD_EXC

__INTERNAL_END
__SSS_TR_END