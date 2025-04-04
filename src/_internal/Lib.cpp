#include "Lib.hpp"
#include "Font.hpp"
#include "Text-Rendering/Area.hpp"
#include "Text-Rendering\Globals.hpp"

SSS_TR_BEGIN;
INTERNAL_BEGIN;

Lib::Ptr Lib::_singleton;

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
    // Ensure all areas stopped drawing
    Area::cancelAll();
    clearFonts();
    _ptr.reset();

    // Log
    if (Log::TR::Lib::query(Log::TR::Lib::get().init)) {
        LOG_TR_MSG("Terminated library");
    }
}

Lib& Lib::getInstance()
{
    if (!_singleton) {
        _singleton.reset(new Lib());
    }
    return *_singleton;
}

void Lib::terminate()
{
    _singleton.reset();
}

FT_Library Lib::getPtr() noexcept
{
    Lib& instance = getInstance();
    return instance._ptr.get();
}


void Lib::addFontDir(std::string const& font_dir) try
{
    Lib& instance = getInstance();

    std::string const rel_path = SSS::PWD + font_dir;
    if (pathIsDir(rel_path)) {
        instance._font_dirs.push_front(rel_path);
    }
    else if (pathIsDir(font_dir)) {
        instance._font_dirs.push_front(font_dir);
    }
    else {
        LOG_FUNC_CTX_WRN("Could not find a directory for given path", font_dir);
    }
}
CATCH_AND_RETHROW_FUNC_EXC;

Lib::FontDirs const& Lib::getFontDirs() noexcept
{
    Lib& instance = getInstance();
    return instance._font_dirs;
}

Font& Lib::getFont(std::string const& font_filename) try
{
    Lib& instance = getInstance();
    Font::Ptr& font = instance._fonts[font_filename];
    if (!font) {
        font.reset(new Font(font_filename));
    }
    return *font;
}
CATCH_AND_RETHROW_FUNC_EXC;

void Lib::unloadFont(std::string const& font_filename)
{
    Lib& instance = getInstance();
    if (instance._fonts.count(font_filename) != 0) {
        instance._fonts.erase(instance._fonts.find(font_filename));
    }
}

void Lib::clearFonts() noexcept
{
    Lib& instance = getInstance();
    instance._fonts.clear();
}


void Lib::setDPI(FT_UInt hdpi, FT_UInt vdpi)
{
    // TODO: reload all cache if DPIs changed
    Lib& instance = getInstance();
    instance._hdpi = hdpi;
    instance._vdpi = vdpi;
}

void Lib::getDPI(FT_UInt& hdpi, FT_UInt& vdpi) noexcept
{
    Lib& instance = getInstance();
    hdpi = instance._hdpi;
    vdpi = instance._vdpi;
}

INTERNAL_END;

SSS_TR_API void init()
{
    _internal::Lib::getPtr();
}

SSS_TR_API void terminate()
{
    _internal::Lib::terminate();
}

void addFontDir(std::string const& font_dir) try
{
    _internal::Lib::addFontDir(font_dir);
}
CATCH_AND_RETHROW_FUNC_EXC;

void loadFont(std::string const& font_filename) try
{
    _internal::Lib::getFont(font_filename);
}
CATCH_AND_RETHROW_FUNC_EXC;

void unloadFont(std::string const& font_filename)
{
    _internal::Lib::unloadFont(font_filename);
}

void clearFonts() noexcept
{
    _internal::Lib::clearFonts();
}

void setDPI(FT_UInt hdpi, FT_UInt vdpi)
{
    _internal::Lib::setDPI(hdpi, vdpi);
}

void getDPI(FT_UInt& hdpi, FT_UInt& vdpi) noexcept
{
    _internal::Lib::getDPI(hdpi, vdpi);
}

SSS_TR_END;