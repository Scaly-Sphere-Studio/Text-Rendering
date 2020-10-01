#include "SSS/Text-Rendering/_pointers.hpp"

SSS_TR_BEGIN__
INTERNAL_BEGIN__

void FT_Done_BitmapGlyph(FT_BitmapGlyph glyph)
{
    FT_Done_Glyph((FT_Glyph)glyph);
}

INTERNAL_END__
SSS_TR_END__