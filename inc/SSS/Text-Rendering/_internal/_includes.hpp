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

// SSS
#include <SSS/Commons.hpp>

/** \cond INCLUDE */
// STL
#include <string>
#include <vector>
#include <array>
#include <deque>
#include <map>
#include <unordered_set>
#include <memory>
#include <locale>

// Clib
#include <stdlib.h>
#include <sys/stat.h>
/** \endcond */

/** Declares the SSS::TR namespace.
 *  Further code will be nested in the SSS::TR namespace.\n
 *  Should be used in pair with with #__SSS_TR_END.
 */
#define __SSS_TR_BEGIN __SSS_BEGIN; namespace TR {
/** Closes the SSS::TR namespace declaration.
 *  Further code will no longer be nested in the SSS::TR namespace.\n
 *  Should be used in pair with with #__SSS_TR_BEGIN.
 */
#define __SSS_TR_END __SSS_END; }

/** \cond INTERNAL */

/** Logs FT_Error if there is one, then return given value.*/
#define __LOG_FT_ERROR_AND_RETURN(X, Y) if (error) { \
    __LOG_METHOD_CTX_ERR(X, FT_Error_String(error)); \
    return Y; \
}
/** Throws FT_Error if there is one.*/
#define __THROW_IF_FT_ERROR(X) if (error) { \
    SSS::throw_exc(__CONTEXT_MSG(X, FT_Error_String(error))); \
}

/** \endcond */