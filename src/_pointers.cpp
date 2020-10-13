#include "SSS/Text-Rendering/_pointers.hpp"

__SSS_TR_BEGIN
__INTERNAL_BEGIN

void FT_Done_BitmapGlyph(FT_BitmapGlyph glyph)
{
    FT_Done_Glyph((FT_Glyph)glyph);
}

__INTERNAL_END
__SSS_TR_END