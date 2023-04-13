#ifndef SSS_TR_LUA_HPP
#define SSS_TR_LUA_HPP

#include <sol/sol.hpp>
#include "SSS/Text-Rendering/Globals.hpp"
#include "SSS/Text-Rendering/Area.hpp"

SSS_TR_BEGIN;

SSS_TR_API void lua_setup_TR(sol::state& lua);

SSS_TR_END;

#endif // SSS_TR_LUA_HPP