#pragma once

#include "_internal/Font.hpp"

__SSS_TR_BEGIN;

void init();
void terminate() noexcept;

void addFontDir(std::string const& font_dir);
void loadFont(std::string const& font_name);
void unloadFont(std::string const& font_name);
void clearFonts() noexcept;

void setDPI(FT_UInt hdpi, FT_UInt vdpi);
void getDPI(FT_UInt& hdpi, FT_UInt& vdpi) noexcept;

__INTERNAL_BEGIN;

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

    static Font::Ptr const& getFont(std::string const& font_name);

};

__INTERNAL_END;

__SSS_TR_END;