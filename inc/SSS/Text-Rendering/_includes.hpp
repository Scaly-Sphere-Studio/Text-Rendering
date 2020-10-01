#pragma once

    // --- Includes ---

// FreeType2
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H

// HarfBuzz
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-ot.h>

// Clib
#include <stdlib.h>
#include <sys/stat.h>

// STL
#include <map>
#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <chrono>

// SSS
#include <SSS/Commons.hpp>

    // --- Defines ---

#define SSS_TR_BEGIN__ SSS_BEGIN__ namespace TR {
#define SSS_TR_END__ SSS_END__ }

// Define log and throw macros
#define LOG_FT_ERROR_AND_RETURN__(X, Y) if (error) { \
    LOG_METHOD_ERR__(get_error(X, FT_Error_String(error))); \
    return Y; \
}
#define THROW_IF_FT_ERROR__(X) if (error) { \
    throw_exc(get_error(X, FT_Error_String(error))); \
}
