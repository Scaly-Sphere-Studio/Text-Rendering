#ifndef SSS_TR_GLOBALS_HPP
#define SSS_TR_GLOBALS_HPP

#include "_includes.hpp"

/** @file
 *  Defines global functions and enums.
 */

SSS_TR_BEGIN;

/** All available inputs to move Area's edit cursor.
 *  @sa Area::cursorMove().
 */
enum class Move {
    None = -1,
    Right,      /**< Move the cursor one character to the right.*/
    Left,       /**< Move the cursor one character to the left.*/
    Down,       /**< Move the cursor one line down.*/
    Up,         /**< Move the cursor one line up.*/
    CtrlRight,  /**< Move the cursor one word to the right.*/
    CtrlLeft,   /**< Move the cursor one word to the left.*/
    Start,      /**< Move the cursor to the start of the line.*/
    End,        /**< Move the cursor to the end of the line.*/
};

/** All available inputs to delete text.
 *  @sa Area::cursorDeleteText().
 */
enum class Delete {
    Invalid = -1,
    Right,      /**< Delete the character at the right of the cursor.*/
    Left,       /**< Delete the character at the left of the cursor.*/
    CtrlRight,  /**< Delete the word at the right of the cursor.*/
    CtrlLeft,   /**< Delete the word at the left of the cursor.*/
};

SSS_TR_API void init();
SSS_TR_API void terminate();

/** Adds user-defined font directory, along with system-defined ones.
 *  @param[in] dir_path The directory path to be added. Can be
 *  relative or absolute.
 *  @sa loadFont(), unloadFont(), clearFonts().
 */
SSS_TR_API void addFontDir(std::string const& dir_path);
/** Loads a font in cache, to be used via Format::font.
 *  @param[in] font_filename Font file name. Must contain
 *  extension, eg: \c "arial.ttf".
 *  @sa addFontDir(), unloadFont(), clearFonts().
 */
SSS_TR_API void loadFont(std::string const& font_filename);
/** Deletes a font that is no longer needed from cache.
 *  @param[in] font_filename Font file name. Must contain
 *  extension, eg: \c "arial.ttf".
 *  @sa addFontDir(), loadFont(), clearFonts().
 */
SSS_TR_API void unloadFont(std::string const& font_filename);
/** Deletes all fonts from cache.
 *  Automatically called from terminate().
 *  @sa addFontDir(), loadFont(), unloadFont().
 */
SSS_TR_API void clearFonts() noexcept;

/** \cond TODO*/
SSS_TR_API void setDPI(FT_UInt hdpi, FT_UInt vdpi);
SSS_TR_API void getDPI(FT_UInt& hdpi, FT_UInt& vdpi) noexcept;
/** \endcond*/

SSS_TR_END;

#endif // SSS_TR_GLOBALS_HPP