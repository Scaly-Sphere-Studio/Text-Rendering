#ifndef SSS_TR_LUA_HPP
#define SSS_TR_LUA_HPP

#define SOL_ALL_SAFETIES_ON 1
#define SOL_SAFE_NUMERICS 1
#define SOL_STRINGS_ARE_NUMBERS 1
#include <sol/sol.hpp>
#include "_includes.hpp"

SSS_TR_BEGIN;

SSS_TR_API void lua_setup_TR(sol::state& lua);

SSS_TR_END;

#endif // SSS_TR_LUA_HPP