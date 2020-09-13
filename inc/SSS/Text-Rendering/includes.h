#pragma once

#define SSS_TR_BEGIN__ namespace SSS { namespace TR {
#define SSS_TR_END__ }}

// FreeType2
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H

// HarfBuzz
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-ot.h>

// STL
#include <map>
#include <vector>
#include <string>
#include <memory>

// SSS libs
#include <SSS/log.h>
#include <SSS/color.h>
