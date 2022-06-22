#include "SSS/Text-Rendering/Globals.hpp"
#include "SSS/Text-Rendering/Area.hpp"

SSS_TR_BEGIN;

INTERNAL_BEGIN;

Lib::Lib()
{
    // Init _font_dirs
    {
#if defined(_WIN32)
        // Windows has a single font directory located in WINDIR
        std::string windir(getEnv("WINDIR"));
        if (!windir.empty()) {
            std::string font_dir = windir + "\\Fonts\\";
            if (pathIsDir(font_dir)) {
                _font_dirs.push_front(font_dir);
            }
        }
#elif defined(_APPLE_) && defined(_MACH_)
        LOG_FUNC_WRN("The local font directories of this OS aren't listed yet."));
#elif defined(linux) || defined(linux)
        LOG_FUNC_WRN("The local font directories of this OS aren't listed yet."));
#endif
        if (_font_dirs.empty()) {
            LOG_FUNC_WRN("No local font directory could be found.");
        }
    }

    // Init _ptr
    _internal::Lib::_ptr.reset(
        []()->FT_Library {
            FT_Library lib;
            FT_Error error = FT_Init_FreeType(&lib);
            THROW_IF_FT_ERROR("FT_Init_FreeType()");
            return lib;
        }
    ());

    // Log
    if (Log::TR::Lib::query(Log::TR::Lib::get().init)) {
        LOG_TR_MSG("Initialized library");
    }
}

Lib::~Lib()
{
    // Clean
    Area::clearMap();
    clearFonts();
    _ptr.reset();

    // Log
    if (Log::TR::Lib::query(Log::TR::Lib::get().init)) {
        LOG_TR_MSG("Terminated library");
    }
}

Lib::Ptr const& Lib::getInstance()
{
    static Ptr singleton(new Lib);
    return singleton;
}

FT_Library_Ptr const& Lib::getPtr() noexcept
{
    Ptr const& instance = getInstance();
    return instance->_ptr;
}

Lib::FontDirs const& Lib::getFontDirs() noexcept
{
    Ptr const& instance = getInstance();
    return instance->_font_dirs;
}

Font::Ptr const& Lib::getFont(std::string const& font_filename) try
{
    Ptr const& instance = getInstance();
    Font::Ptr& font = instance->_fonts[font_filename];
    if (!font) {
        font.reset(new Font(font_filename));
    }
    return font;
}
CATCH_AND_RETHROW_FUNC_EXC;

INTERNAL_END;

void addFontDir(std::string const& font_dir) try
{
    _internal::Lib::Ptr const& instance = _internal::Lib::getInstance();

    std::string const rel_path = SSS::PWD + font_dir;
    if (pathIsDir(rel_path)) {
        instance->_font_dirs.push_front(rel_path);
    }
    else if (pathIsDir(font_dir)) {
        instance->_font_dirs.push_front(font_dir);
    }
    else {
        LOG_FUNC_CTX_WRN("Could not find a directory for given path", font_dir);
    }
}
CATCH_AND_RETHROW_FUNC_EXC;

void loadFont(std::string const& font_filename) try
{
    _internal::Lib::getFont(font_filename);
}
CATCH_AND_RETHROW_FUNC_EXC;

void unloadFont(std::string const& font_filename)
{
    _internal::Lib::Ptr const& instance = _internal::Lib::getInstance();
    if (instance->_fonts.count(font_filename) != 0) {
        instance->_fonts.erase(instance->_fonts.find(font_filename));
    }
}

void clearFonts() noexcept
{
    _internal::Lib::Ptr const& instance = _internal::Lib::getInstance();
    instance->_fonts.clear();
}

void setDPI(FT_UInt hdpi, FT_UInt vdpi)
{
    // TODO: reload all cache if DPIs changed
    _internal::Lib::Ptr const& instance = _internal::Lib::getInstance();
    instance->_hdpi = hdpi;
    instance->_vdpi = vdpi;
}

void getDPI(FT_UInt& hdpi, FT_UInt& vdpi) noexcept
{
    _internal::Lib::Ptr const& instance = _internal::Lib::getInstance();
    hdpi = instance->_hdpi;
    vdpi = instance->_vdpi;
}



SSS_TR_END;
