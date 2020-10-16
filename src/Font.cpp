#include "SSS/Text-Rendering/Font.hpp"

__SSS_TR_BEGIN

    // --- Static variables ---

// FreeType library initialization
_internal::FT_Library_Ptr Font::_lib{
    // Init FreeType lib
    []()->FT_Library {
        FT_Library lib;
        FT_Error error = FT_Init_FreeType(&lib);
        __THROW_IF_FT_ERROR("FT_Init_FreeType()");
        return lib;
    }()
};
// Search for local font directories
std::deque<std::string> Font::_font_dirs{
    // Get the local font directories based on the OS
    []()->std::deque<std::string> {
        std::deque<std::string> ret;
        #if defined(_WIN32)
        // Windows has a single font directory located in WINDIR
        std::string windir(copyEnv("WINDIR"));
        if (!windir.empty()) {
            std::string font_dir = windir + "\\Fonts\\";
            if (isDir(font_dir)) {
                ret.push_front(font_dir);
            }
        }
        #elif defined(_APPLE_) && defined(_MACH_)
        __LOG_FUNC_WRN("The local font directories of this OS aren't listed yet."));
        #elif defined(linux) || defined(__linux)
        __LOG_FUNC_WRN("The local font directories of this OS aren't listed yet."));
        #endif
        if (ret.empty()) {
            __LOG_FUNC_WRN("No local font directory could be found.");
        }
        return ret;
    }()
};
std::map<std::string, Font::Weak> Font::_shared{ };
FT_UInt Font::_hdpi{ 96 };
FT_UInt Font::_vdpi{ 0 };

    // --- Static functions ---

// Sets screen DPI for all instances (default: 96x96).
void Font::setDPI(FT_UInt hdpi, FT_UInt vdpi) noexcept
{
    // TODO: reload all cache if DPIs changed
    _hdpi = hdpi;
    _vdpi = vdpi;
}

// Adds a font directory to the system ones. To be called before creating a font.
void Font::addFontDir(std::string const& font_dir) try
{
    if (isDir(font_dir)) {
        _font_dirs.push_front(font_dir);
    }
    else {
        __LOG_FUNC_WRN(context_msg(
            "Given path does not exist or is not a directory", font_dir));
    }
}
__CATCH_AND_RETHROW_FUNC_EXC

// Creates a shared Font instance to be used & re-used everywhere
Font::Shared Font::getShared(std::string const& font_file) try
{
    // Retrieve or create corresponding weak_ptr
    Weak& weak = _shared[font_file];
    // Retrieve linked shared_ptr
    Shared shared = weak.lock();
    // If the shared_ptr is empty, create a new Font
    if (!shared) {
        // Create new Font
        shared = std::make_shared<Font>(font_file);
        // Reference Font in our map
        Weak tmp(shared);
        weak.swap(tmp);
    }
    return shared;
}
__CATCH_AND_RETHROW_FUNC_EXC

    // --- Constructor & Destructor ---

// Constructor, inits FreeType if called for the first time.
// Creates a FreeType font face.
Font::Font(std::string const& font_file) try
{
    // Find the first occurence of the font in _font_dirs
    std::string font_path;
    for (std::string const& dir : _font_dirs) {
        std::string const path = dir + font_file;
        if (isReg(path)) {
            font_path = path;
            break;
        }
    }
    // Ensure that we found a valid path
    if (font_path.empty()) {
        throw_exc("Could not find '" + font_file + "' anywhere.");
    }
    
    FT_Face face;
    FT_Error error = FT_New_Face(_lib.get(), font_path.c_str(), 0, &face);
    __THROW_IF_FT_ERROR("FT_New_Face()");
    _face.reset(face);

    __LOG_CONSTRUCTOR
}
__CATCH_AND_RETHROW_METHOD_EXC

// Destructor, quits FreeType if called from last remaining instance.
// Destroys the FreeType font face.
Font::~Font() noexcept
{
    __LOG_DESTRUCTOR
}

    // --- Glyph functions ---

// Call this function whenever changing charsize
void Font::useCharsize(int charsize) try
{
    // Create font size if needed
    if (_font_sizes.count(charsize) == 0) {
        _font_sizes.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(charsize),        // Key
            std::forward_as_tuple(_face, charsize)  // Value
        );
    }
}
__CATCH_AND_RETHROW_METHOD_EXC

// Loads corresponding glyph.
bool Font::loadGlyph(FT_UInt glyph_index, int charsize, int outline_size) try
{
    return _font_sizes.at(charsize).loadGlyph(glyph_index, outline_size);
}
__CATCH_AND_RETHROW_METHOD_EXC

// Clears out the internal glyph cache.
void Font::unloadGlyphs() noexcept
{
    _font_sizes.clear();
}

    // --- Get functions ---

// Returns the corresponding internal HarfBuzz font.
_internal::HB_Font_Ptr const& Font::getHBFont(int charsize) const try
{
    _throw_if_bad_charsize(charsize);
    return _font_sizes.at(charsize).getHBFont();
}
__CATCH_AND_RETHROW_METHOD_EXC

// Returns corresponding glyph as a bitmap
_internal::FT_BitmapGlyph_Ptr const&
Font::getGlyphBitmap(FT_UInt glyph_index, int charsize) const try
{
    _throw_if_bad_charsize(charsize);
    return _font_sizes.at(charsize).getGlyphBitmap(glyph_index);
}
__CATCH_AND_RETHROW_METHOD_EXC

// Returns corresponding glyph outline as a bitmap
_internal::FT_BitmapGlyph_Ptr const&
Font::getOutlineBitmap(FT_UInt glyph_index, int charsize, int outline_size) const try
{
    _throw_if_bad_charsize(charsize);
    return _font_sizes.at(charsize).getOutlineBitmap(glyph_index, outline_size);
}
__CATCH_AND_RETHROW_METHOD_EXC

    // --- Private functions ---

// Ensures the given charsize has been initialized
void Font::_throw_if_bad_charsize(int charsize) const
{
    if (_font_sizes.count(charsize) == 0) {
        throw_exc(ERR_MSG::NOTHING_FOUND);
    }
}

__SSS_TR_END