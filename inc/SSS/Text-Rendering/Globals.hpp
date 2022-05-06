#pragma once

#include "_internal/Font.hpp"

/** @file
 *  Defines global functions and enums.
 */

namespace SSS::Log::TR {
    struct Lib : public LogBase<Lib> {
        using LOG_STRUCT_BASICS(TR, Lib);
        bool init = false;
    };
}

SSS_TR_BEGIN;

/** All available inputs to move Area's edit cursor.
 *  @sa Area::cursorMove().
 */
enum class Move {
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
    Right,      /**< Delete the character at the right of the cursor.*/
    Left,       /**< Delete the character at the left of the cursor.*/
    CtrlRight,  /**< Delete the word at the right of the cursor.*/
    CtrlLeft,   /**< Delete the word at the left of the cursor.*/
};

/** Inits internal libraries.
 *  @usage To be called before any processing operation, and
 *  met with terminate() before exiting the program.
 */
void init();
/** Clears cached data & terminates internal libraries.
 *  @usage To be called before exiting the program, to meet
 *  previous init() call.
 */
void terminate() noexcept;

/** Adds user-defined font directory, along with system-defined ones.
 *  @param[in] dir_path The directory path to be added. Can be
 *  relative or absolute.
 *  @sa loadFont(), unloadFont(), clearFonts().
 */
void addFontDir(std::string const& dir_path);
/** Loads a font in cache, to be used via Format::font.
 *  @param[in] font_filename Font file name. Must contain
 *  extension, eg: \c "arial.ttf".
 *  @sa addFontDir(), unloadFont(), clearFonts().
 */
void loadFont(std::string const& font_filename);
/** Deletes a font that is no longer needed from cache.
 *  @param[in] font_filename Font file name. Must contain
 *  extension, eg: \c "arial.ttf".
 *  @sa addFontDir(), loadFont(), clearFonts().
 */
void unloadFont(std::string const& font_filename);
/** Deletes all fonts from cache.
 *  Automatically called from terminate().
 *  @sa addFontDir(), loadFont(), unloadFont().
 */
void clearFonts() noexcept;

/** \cond TODO*/
void setDPI(FT_UInt hdpi, FT_UInt vdpi);
void getDPI(FT_UInt& hdpi, FT_UInt& vdpi) noexcept;
/** \endcond*/

INTERNAL_BEGIN;

class Lib {
    friend void ::SSS::TR::init();
    friend void ::SSS::TR::terminate() noexcept;

    friend void ::SSS::TR::addFontDir(std::string const&);
    friend void ::SSS::TR::loadFont(std::string const&);
    friend void ::SSS::TR::unloadFont(std::string const&);
    friend void ::SSS::TR::clearFonts() noexcept;

    friend void ::SSS::TR::setDPI(FT_UInt, FT_UInt);
    friend void ::SSS::TR::getDPI(FT_UInt&, FT_UInt&) noexcept;

private:
    static FT_Library_Ptr ptr; // FreeType library pointer

    static FT_UInt hdpi;   // Screen's horizontal dpi
    static FT_UInt vdpi;   // Screen's vertical dpi

    using FontDirs = std::deque<std::string>;
    static FontDirs font_dirs;  // Font directories
    using FontMap = std::map<std::string, Font::Ptr>;
    static FontMap fonts;       // Fonts

public:
    static inline FT_Library_Ptr const& getPtr() noexcept { return ptr; };

    static inline FontDirs const& getFontDirs() noexcept { return font_dirs; };

    static Font::Ptr const& getFont(std::string const& font_filename);

};

INTERNAL_END;

SSS_TR_END;