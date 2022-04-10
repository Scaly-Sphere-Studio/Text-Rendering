#include "SSS/Text-Rendering/_internal/Font.hpp"
#include "SSS/Text-Rendering/Lib.hpp"

__SSS_TR_BEGIN;
__INTERNAL_BEGIN;

// --- Constructor & Destructor ---

// Constructor, inits FreeType if called for the first time.
// Creates a FreeType font face.
Font::Font(std::string const& font_file) try
{
    // Find the first occurence of the font in _font_dirs
    std::string font_path;
    for (std::string const& dir : Lib::getFontDirs()) {
        std::string const path = dir + font_file;
        if (pathIsFile(path)) {
            font_path = path;
            break;
        }
    }
    // Ensure that we found a valid path
    if (font_path.empty()) {
        throw_exc("Could not find '" + font_file + "' anywhere.");
    }
    
    FT_Face face;
    FT_Error error = FT_New_Face(Lib::getPtr().get(), font_path.c_str(), 0, &face);
    __THROW_IF_FT_ERROR("FT_New_Face()");
    _face.reset(face);

    __LOG_CONSTRUCTOR;
}
__CATCH_AND_RETHROW_METHOD_EXC;

// Destructor, quits FreeType if called from last remaining instance.
// Destroys the FreeType font face.
Font::~Font() noexcept
{
    _font_sizes.clear();
    _face.release();
    __LOG_DESTRUCTOR;
}

// --- Glyph functions ---

// Call this function whenever changing charsize
void Font::setCharsize(int charsize) try
{
    if (charsize <= 0) {
        charsize = 1;
    }
    // Create font size if needed (which sets the charsize)
    if (_font_sizes.count(charsize) == 0) {
        _font_sizes.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(charsize),        // Key
            std::forward_as_tuple(_face, charsize)  // Value
        );
    }
    else {
        _font_sizes.at(charsize).setCharsize();
    }
}
__CATCH_AND_RETHROW_METHOD_EXC;

// Loads corresponding glyph.
bool Font::loadGlyph(FT_UInt glyph_index, int charsize, int outline_size) try
{
    _throw_if_bad_charsize(charsize);
    return _font_sizes.at(charsize).loadGlyph(glyph_index, outline_size);
}
__CATCH_AND_RETHROW_METHOD_EXC;

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
__CATCH_AND_RETHROW_METHOD_EXC;

// Returns corresponding glyph as a bitmap
_internal::Bitmap const&
Font::getGlyphBitmap(FT_UInt glyph_index, int charsize) const try
{
    _throw_if_bad_charsize(charsize);
    return _font_sizes.at(charsize).getGlyphBitmap(glyph_index);
}
__CATCH_AND_RETHROW_METHOD_EXC;

// Returns corresponding glyph outline as a bitmap
_internal::Bitmap const&
Font::getOutlineBitmap(FT_UInt glyph_index, int charsize, int outline_size) const try
{
    _throw_if_bad_charsize(charsize);
    return _font_sizes.at(charsize).getOutlineBitmap(glyph_index, outline_size);
}
__CATCH_AND_RETHROW_METHOD_EXC;

    // --- Private functions ---

// Ensures the given charsize has been initialized
void Font::_throw_if_bad_charsize(int &charsize) const
{
    if (charsize <= 0) {
        charsize = 1;
    }
    if (_font_sizes.count(charsize) == 0) {
        throw_exc(ERR_MSG::NOTHING_FOUND);
    }
}

__INTERNAL_END;
__SSS_TR_END;