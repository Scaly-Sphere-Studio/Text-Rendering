#pragma once

#include "SSS/Text-Rendering/_includes.hpp"

SSS_TR_BEGIN__
INTERNAL_BEGIN__

using FT_Library_Ptr = C_Ptr
    <FT_LibraryRec_, FT_Error(*)(FT_Library), FT_Done_FreeType>;

using FT_Face_Ptr = C_Ptr
    <FT_FaceRec_, FT_Error(*)(FT_Face), FT_Done_Face>;

using FT_Glyph_Ptr = C_Ptr
    <FT_GlyphRec_, void(*)(FT_Glyph), FT_Done_Glyph>;

auto lambda = [](FT_BitmapGlyph bmp) noexcept { FT_Done_Glyph((FT_Glyph)bmp); };

void FT_Done_BitmapGlyph(FT_BitmapGlyph glyph);

using FT_BitmapGlyph_Ptr = C_Ptr
    <FT_BitmapGlyphRec_, void(*)(FT_BitmapGlyph), FT_Done_BitmapGlyph>;

using FT_Stroker_Ptr = C_Ptr
    <FT_StrokerRec_, void(*)(FT_Stroker), FT_Stroker_Done>;

using HB_Font_Ptr = C_Ptr
    <hb_font_t, void(*)(hb_font_t*), hb_font_destroy>;

using HB_Buffer_Ptr = C_Ptr
    <hb_buffer_t, void(*)(hb_buffer_t*), hb_buffer_destroy>;

INTERNAL_END__
SSS_TR_END__