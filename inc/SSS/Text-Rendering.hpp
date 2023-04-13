#ifndef SSS_TR_HPP
#define SSS_TR_HPP

#ifdef SSS_LUA
#include "Text-Rendering/Lua.hpp"
#endif // SSS_LUA
#include "Text-Rendering/Globals.hpp"
#include "Text-Rendering/Area.hpp"

/** @file
 *  Header of the
 *  [SSS/Text-Rendering](https://github.com/Scaly-Sphere-Studio/Text-Rendering)
 *  library, includes all \c SSS/Text-Rendering/ headers.
 */

/** @dir SSS/Text-Rendering
 *  Holds all \b %SSS/Text-Rendering headers.
 */

/** @namespace SSS::TR
 *  Text rendering logic, using \b FreeType and \b HarfBuzz internally.
 *  Global functions are used to initialize the library and load fonts.\n
 *  The Area class is where most of the logic stands.
 */

#endif // SSS_TR_HPP