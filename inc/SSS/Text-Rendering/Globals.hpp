#ifndef SSS_TR_GLOBALS_HPP
#define SSS_TR_GLOBALS_HPP

#include "_internal/Font.hpp"

/** @file
 *  Defines global functions and enums.
 */

namespace SSS::Log::TR {
    /** Logging properties for SSS::TR globals.*/
    struct Lib : public LogBase<Lib> {
        using LOG_STRUCT_BASICS(TR, Lib);
        /** Logs both SSS::TR::init() and SSS::TR::terminate().*/
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

INTERNAL_BEGIN;

class Lib {
private:
    FT_Library_Ptr _ptr;    // FreeType library pointer

    FT_UInt _hdpi{ 96 };    // Screen's horizontal dpi
    FT_UInt _vdpi{ 0 };     // Screen's vertical dpi

    using FontDirs = std::deque<std::string>;
    FontDirs _font_dirs;    // Font directories
    using FontMap = std::map<std::string, Font::Ptr>;
    FontMap _fonts;         // Fonts

    using Ptr = std::unique_ptr<Lib>;
    static Lib& getInstance();
    
    Lib();
public:
    ~Lib();

    static FT_Library getPtr() noexcept;

    static void addFontDir(std::string const&);
    static FontDirs const& getFontDirs() noexcept;

    static Font& getFont(std::string const& font_filename);
    static void unloadFont(std::string const&);
    static void clearFonts() noexcept;

    static void setDPI(FT_UInt, FT_UInt);
    static void getDPI(FT_UInt&, FT_UInt&) noexcept;
};

INTERNAL_END;

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

SSS_TR_END;

#endif // SSS_TR_GLOBALS_HPP