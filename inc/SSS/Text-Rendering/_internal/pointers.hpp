#pragma once

#include "_includes.hpp"

/** @file
 *  Defines internal SSS::C_Ptr alias declarations.
 */

__SSS_TR_BEGIN;
__INTERNAL_BEGIN;

using FT_Library_Ptr = C_Ptr
    <FT_LibraryRec_, FT_Error(*)(FT_Library), FT_Done_FreeType>;

using FT_Face_Ptr = C_Ptr
    <FT_FaceRec_, FT_Error(*)(FT_Face), FT_Done_Face>;

using FT_Stroker_Ptr = C_Ptr
    <FT_StrokerRec_, void(*)(FT_Stroker), FT_Stroker_Done>;

using HB_Font_Ptr = C_Ptr
    <hb_font_t, void(*)(hb_font_t*), hb_font_destroy>;

using HB_Buffer_Ptr = C_Ptr
    <hb_buffer_t, void(*)(hb_buffer_t*), hb_buffer_destroy>;

__INTERNAL_END;
__SSS_TR_END;
