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
#include <unordered_set>
#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <locale>

// SSS
#include <SSS/Commons.hpp>

    // --- Defines ---

#define __SSS_TR_BEGIN __SSS_BEGIN namespace TR {
#define __SSS_TR_END __SSS_END }

// Logs FT_Error if there is one, then return given value
#define __LOG_FT_ERROR_AND_RETURN(X, Y) if (error) { \
    __LOG_METHOD_ERR(SSS::context_msg(X, FT_Error_String(error))); \
    return Y; \
}
// Throws FT_Error if there is one
#define __THROW_IF_FT_ERROR(X) if (error) { \
    SSS::throw_exc(SSS::context_msg(X, FT_Error_String(error))); \
}

__SSS_TR_BEGIN
__INTERNAL_BEGIN
static inline std::u32string toU32str(std::string str) { return std::u32string(str.cbegin(), str.cend()); };
__INTERNAL_END
__SSS_TR_END
