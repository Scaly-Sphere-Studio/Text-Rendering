#include "SSS/Text-Rendering/pointers.h"

SSS_TR_BEGIN__

FT_Library_Ptr::FT_Library_Ptr(FT_Library ptr) noexcept
    : std::unique_ptr<FT_LibraryRec_, FT_Error(*)(FT_Library)>(ptr, FT_Done_FreeType)
{
    LOG_CONSTRUCTOR__
}

FT_Library_Ptr::~FT_Library_Ptr() noexcept
{
    LOG_DESTRUCTOR__
}

FT_Face_Ptr::FT_Face_Ptr(FT_Face ptr) noexcept
    : std::unique_ptr<FT_FaceRec_, FT_Error(*)(FT_Face)>(ptr, FT_Done_Face)
{
    LOG_CONSTRUCTOR__
}

FT_Face_Ptr::~FT_Face_Ptr() noexcept
{
    LOG_DESTRUCTOR__
}

FT_Glyph_Ptr::FT_Glyph_Ptr(FT_Glyph ptr) noexcept
    : std::unique_ptr<FT_GlyphRec_, void(*)(FT_Glyph)>(ptr, FT_Done_Glyph)
{
    LOG_CONSTRUCTOR__
}

FT_Glyph_Ptr::~FT_Glyph_Ptr() noexcept
{
    LOG_DESTRUCTOR__
}

// Necesary for FT_BitmapGlyph_Ptr destructor
void FT_Done_BitmapGlyph(FT_BitmapGlyph glyph) noexcept
{
    FT_Done_Glyph((FT_Glyph)glyph);
}

FT_BitmapGlyph_Ptr::FT_BitmapGlyph_Ptr(FT_BitmapGlyph ptr) noexcept
    : std::unique_ptr<FT_BitmapGlyphRec_, void(*)(FT_BitmapGlyph)>(ptr, FT_Done_BitmapGlyph)
{
    LOG_CONSTRUCTOR__
}

FT_BitmapGlyph_Ptr::~FT_BitmapGlyph_Ptr() noexcept
{
    LOG_DESTRUCTOR__
}

FT_Stroker_Ptr::FT_Stroker_Ptr(FT_Stroker ptr) noexcept
    : std::unique_ptr<FT_StrokerRec_, void(*)(FT_Stroker)>(ptr, FT_Stroker_Done)
{
    LOG_CONSTRUCTOR__
}

FT_Stroker_Ptr::~FT_Stroker_Ptr() noexcept
{
    LOG_DESTRUCTOR__
}

HB_Font_Ptr::HB_Font_Ptr(hb_font_t *ptr) noexcept
    : std::unique_ptr<hb_font_t, void(*)(hb_font_t*)>(ptr, hb_font_destroy)
{
    LOG_CONSTRUCTOR__
}

HB_Font_Ptr::~HB_Font_Ptr() noexcept
{
    LOG_DESTRUCTOR__
}

SSS_TR_END__