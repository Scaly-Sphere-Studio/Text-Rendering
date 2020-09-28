#include "SSS/Text-Rendering/Font.hpp"

SSS_TR_BEGIN__

    // --- Static variables ---

// FreeType init
FT_Library_Ptr Font::lib_{
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
_StringDeque Font::font_dirs_{
    // Get the local font directories based on the OS
    []()->_StringDeque {
        // Retrieve the environment variable
        auto copyEnv = [](std::string varname)->std::string
        {
            std::string ret;
            char* buffer;
            _dupenv_s(&buffer, NULL, varname.c_str());
            if (buffer != nullptr) {
                ret = buffer;
                free(buffer);
            }
            return ret;
        };
        // Checks that the path is a directory
        auto isDir = [](std::string path)->bool
        {
            struct stat s;
            return stat(path.c_str(), &s) == 0 && (s.st_mode & S_IFDIR) != 0;
        };
        // Return value
        _StringDeque ret;

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

    // --- Constructor & Destructor ---

// Constructor, inits FreeType if called for the first time.
// Creates a FreeType font face.
Font::Font(std::string const& font_file) try
{
    // Find the first occurence of the font in font_dirs_
    std::string font_path;
    for (std::string const& dir : font_dirs_) {
        std::string path = dir + font_file;
        struct stat s;
        if (stat(path.c_str(), &s) == 0 && (s.st_mode & S_IFREG) != 0) {
            font_path = path;
            break;
        }
    }
    // Ensure that we found a valid path
    if (font_path.empty()) {
        throw_exc("Could not find '" + font_file + "' anywhere.");
    }
    
    // Create FreeType font face.
    face_.create(lib_, font_path);

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
FT_BitmapGlyph_Ptr const&
Font::getGlyphBitmap(FT_UInt glyph_index, int charsize) const try
{
    throw_if_bad_charsize_(charsize);
    return font_sizes_.at(charsize).getGlyphBitmap(glyph_index);
}
CATCH_AND_RETHROW_METHOD_EXC__

// Returns corresponding glyph outline as a bitmap
FT_BitmapGlyph_Ptr const&
Font::getOutlineBitmap(FT_UInt glyph_index, int charsize, int outline_size) const try
{
    throw_if_bad_charsize_(charsize);
    return font_sizes_.at(charsize).getOutlineBitmap(glyph_index, outline_size);
}
CATCH_AND_RETHROW_METHOD_EXC__

// Returns the internal FreeType font face.
FT_Face_Ptr const& Font::getFTFace() const noexcept
{
    return face_;
}

// Returns the corresponding internal HarfBuzz font.
HB_Font_Ptr const& Font::getHBFont(int charsize) const try
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
        throw_exc(NOTHING_FOUND);
    }
}

SSS_TR_END__