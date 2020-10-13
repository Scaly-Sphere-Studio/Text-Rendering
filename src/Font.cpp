#include "SSS/Text-Rendering/Font.hpp"

SSS_TR_BEGIN__

    // --- Static variables ---

// FreeType init
_internal::FT_Library_Ptr Font::lib_{
    // Init FreeType
    []()->FT_Library {
        FT_Library lib;
        FT_Error error = FT_Init_FreeType(&lib);
        THROW_IF_FT_ERROR__("FT_Init_FreeType()");
        LOG_MSG__("FreeType library initialized");
        return lib;
    }()
};
// Search for local font directories
_internal::StringDeque Font::font_dirs_{
    // Get the local font directories based on the OS
    []()->_internal::StringDeque {
        _internal::StringDeque ret;
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
        LOG_ERR__("WARNING: The local font directories of this OS aren't listed yet."));
        #elif defined(linux) || defined(__linux)
        LOG_ERR__("WARNING: The local font directories of this OS aren't listed yet."));
        #endif
        if (ret.empty()) {
            LOG_ERR__("WARNING: No local font directory could be found.");
        }
        return ret;
    }()
};
std::map<std::string, Font::Weak> Font::shared_{ };
size_t Font::instances_{ 0 };
FT_UInt Font::hdpi_{ 96 };
FT_UInt Font::vdpi_{ 0 };

    // --- Static functions ---

// Sets screen DPI for all instances (default: 96x96).
void Font::setDPI(FT_UInt hdpi, FT_UInt vdpi) noexcept
{
    hdpi_ = hdpi;
    vdpi_ = vdpi;
}

// Adds a font directory to the system ones. To be called before creating a font.
void Font::addFontDir(std::string const& font_dir) try
{
    if (isDir(font_dir)) {
        font_dirs_.push_front(font_dir);
    }
    else {
        LOG_FUNC_ERR__(get_error(
            "WARNING: Given path does not exist or is not a directory.", font_dir));
    }
}
CATCH_AND_RETHROW_FUNC_EXC__

// Creates a shared Font instance to be used & re-used everywhere
Font::Shared Font::getShared(std::string const& font_file) try
{
    // Retrieve or create corresponding weak_ptr
    Weak& weak = shared_[font_file];
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
CATCH_AND_RETHROW_FUNC_EXC__

    // --- Constructor & Destructor ---

// Constructor, inits FreeType if called for the first time.
// Creates a FreeType font face.
Font::Font(std::string const& font_file) try
{
    // Find the first occurence of the font in font_dirs_
    std::string font_path;
    for (std::string const& dir : font_dirs_) {
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
    FT_Error error = FT_New_Face(lib_.get(), font_path.c_str(), 0, &face);
    THROW_IF_FT_ERROR__("FT_New_Face()");
    face_.reset(face);

    ++instances_;
    LOG_CONSTRUCTOR__
}
CATCH_AND_RETHROW_METHOD_EXC__

// Destructor, quits FreeType if called from last remaining instance.
// Destroys the FreeType font face.
Font::~Font() noexcept
{
    --instances_;
    if (instances_ == 0) {
        LOG_MSG__("FreeType library quit");
    }
    LOG_DESTRUCTOR__
}

    // --- Glyph functions ---

// Call this function whenever changing charsize
void Font::useCharsize(int charsize) try
{
    // Create font size if needed
    if (font_sizes_.count(charsize) == 0) {
        font_sizes_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(charsize),        // Key
            std::forward_as_tuple(face_, charsize)  // Value
        );
    }
}
CATCH_AND_RETHROW_METHOD_EXC__

// Loads corresponding glyph.
bool Font::loadGlyph(FT_UInt glyph_index, int charsize, int outline_size) try
{
    return font_sizes_.at(charsize).loadGlyph(glyph_index, outline_size);
}
CATCH_AND_RETHROW_METHOD_EXC__

// Clears out the internal glyph cache.
void Font::unloadGlyphs() noexcept
{
    font_sizes_.clear();
}

    // --- Get functions ---

// Returns corresponding glyph as a bitmap
_internal::FT_BitmapGlyph_Ptr const&
Font::getGlyphBitmap(FT_UInt glyph_index, int charsize) const try
{
    throw_if_bad_charsize_(charsize);
    return font_sizes_.at(charsize).getGlyphBitmap(glyph_index);
}
CATCH_AND_RETHROW_METHOD_EXC__

// Returns corresponding glyph outline as a bitmap
_internal::FT_BitmapGlyph_Ptr const&
Font::getOutlineBitmap(FT_UInt glyph_index, int charsize, int outline_size) const try
{
    throw_if_bad_charsize_(charsize);
    return font_sizes_.at(charsize).getOutlineBitmap(glyph_index, outline_size);
}
CATCH_AND_RETHROW_METHOD_EXC__

// Returns the internal FreeType font face.
_internal::FT_Face_Ptr const& Font::getFTFace() const noexcept
{
    return face_;
}

// Returns the corresponding internal HarfBuzz font.
_internal::HB_Font_Ptr const& Font::getHBFont(int charsize) const try
{
    throw_if_bad_charsize_(charsize);
    return font_sizes_.at(charsize).getHBFont();
}
CATCH_AND_RETHROW_METHOD_EXC__

    // --- Private functions ---

// Ensures the given charsize has been initialized
void Font::throw_if_bad_charsize_(int charsize) const
{
    if (font_sizes_.count(charsize) == 0) {
        throw_exc(ERR_MSG::NOTHING_FOUND);
    }
}

SSS_TR_END__