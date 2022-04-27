#include "SSS/Text-Rendering/Globals.hpp"
#include "SSS/Text-Rendering/Area.hpp"

SSS_TR_BEGIN;

INTERNAL_BEGIN;

// FreeType library initialization
_internal::FT_Library_Ptr Lib::ptr{ };

// Search for local font directories
std::deque<std::string> Lib::font_dirs{
    // Get the local font directories based on the OS
    []()->std::deque<std::string> {
        std::deque<std::string> ret;
        #if defined(_WIN32)
        // Windows has a single font directory located in WINDIR
        std::string windir(getEnv("WINDIR"));
        if (!windir.empty()) {
            std::string font_dir = windir + "\\Fonts\\";
            if (pathIsDir(font_dir)) {
                ret.push_front(font_dir);
            }
        }
        #elif defined(_APPLE_) && defined(_MACH_)
        LOG_FUNC_WRN("The local font directories of this OS aren't listed yet."));
        #elif defined(linux) || defined(linux)
        LOG_FUNC_WRN("The local font directories of this OS aren't listed yet."));
        #endif
        if (ret.empty()) {
            LOG_FUNC_WRN("No local font directory could be found.");
        }
        return ret;
    }()
};

std::map<std::string, Font::Ptr> Lib::fonts{ };

FT_UInt Lib::hdpi{ 96 };
FT_UInt Lib::vdpi{ 0 };

Font::Ptr const& Lib::getFont(std::string const& font_filename) try
{
    if (fonts.count(font_filename) == 0) {
        throw_exc(CONTEXT_MSG("No loaded font with name", font_filename));
    }
    return fonts.at(font_filename);
}
CATCH_AND_RETHROW_FUNC_EXC;

INTERNAL_END;

void init() try
{
    _internal::Lib::ptr.reset([]()->FT_Library {
        FT_Library lib;
        FT_Error error = FT_Init_FreeType(&lib);
        THROW_IF_FT_ERROR("FT_Init_FreeType()");
        return lib;
        }());
}
CATCH_AND_RETHROW_FUNC_EXC;

void terminate() noexcept
{
    Area::clearMap();
    clearFonts();
    _internal::Lib::ptr.reset();
}

void addFontDir(std::string const& font_dir) try
{
    std::string const rel_path = SSS::PWD + font_dir;
    if (pathIsDir(rel_path)) {
        _internal::Lib::font_dirs.push_front(rel_path);
    }
    else if (pathIsDir(font_dir)) {
        _internal::Lib::font_dirs.push_front(font_dir);
    }
    else {
        LOG_FUNC_CTX_WRN("Could not find a directory given path", font_dir);
    }
}
CATCH_AND_RETHROW_FUNC_EXC;

void loadFont(std::string const& font_filename) try
{
    _internal::Font::Ptr& font = _internal::Lib::fonts[font_filename];
    if (!font) {
        font.reset(new _internal::Font(font_filename));
    }
}
CATCH_AND_RETHROW_FUNC_EXC;

void unloadFont(std::string const& font_filename)
{
    if (_internal::Lib::fonts.count(font_filename) != 0) {
        _internal::Lib::fonts.erase(_internal::Lib::fonts.find(font_filename));
    }
}

void clearFonts() noexcept
{
    _internal::Lib::fonts.clear();
}

void setDPI(FT_UInt hdpi, FT_UInt vdpi)
{
    // TODO: reload all cache if DPIs changed
    _internal::Lib::hdpi = hdpi;
    _internal::Lib::vdpi = vdpi;
}

void getDPI(FT_UInt& hdpi, FT_UInt& vdpi) noexcept
{
    hdpi = _internal::Lib::hdpi;
    vdpi = _internal::Lib::vdpi;
}



SSS_TR_END;
