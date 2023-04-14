#ifndef SSS_TR_LIB_HPP
#define SSS_TR_LIB_HPP

#include "Text-Rendering/_includes.hpp"

namespace SSS::Log::TR {
    /** Logging properties for SSS::TR globals.*/
    struct Lib : public LogBase<Lib> {
        using LOG_STRUCT_BASICS(TR, Lib);
        /** Logs both SSS::TR::init() and SSS::TR::terminate().*/
        bool init = false;
    };
}

SSS_TR_BEGIN;
INTERNAL_BEGIN;

using FT_Library_Ptr = C_Ptr
    <FT_LibraryRec_, FT_Error(*)(FT_Library), FT_Done_FreeType>;

using FT_Face_Ptr = C_Ptr
    <FT_FaceRec_, FT_Error(*)(FT_Face), FT_Done_Face>;

using FT_Stroker_Ptr = C_Ptr
    <FT_StrokerRec_, void(*)(FT_Stroker), FT_Stroker_Done>;

using HB_Font_Ptr = C_Ptr
    <hb_font_t, void(*)(hb_font_t*), hb_font_destroy>;

using HB_Buffer_Ptr = C_Ptr
    <hb_buffer_t, void(*)(hb_buffer_t*), hb_buffer_destroy>;

// Pre-declaration
class Font;

class Lib {
private:
    FT_Library_Ptr _ptr;    // FreeType library pointer

    FT_UInt _hdpi{ 96 };    // Screen's horizontal dpi
    FT_UInt _vdpi{ 0 };     // Screen's vertical dpi

    using FontDirs = std::deque<std::string>;
    FontDirs _font_dirs;    // Font directories
    using FontMap = std::map<std::string, std::unique_ptr<Font>>;
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
SSS_TR_END;

#endif // SSS_TR_LIB_HPP
