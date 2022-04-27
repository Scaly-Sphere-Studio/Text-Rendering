#pragma once

#include "FontSize.hpp"

/** @file
 *  Defines the internal font management class.
 */

SSS_TR_BEGIN;
/** \cond INTERNAL */
void loadFont(std::string const& font_name); // Pre-declaration
/** \endcond */

INTERNAL_BEGIN;

class Font {
    friend void SSS::TR::loadFont(std::string const& font_file);
public:
    using Ptr = std::unique_ptr<Font>;

// --- Constructor & Destructor ---
private:
    // Creates a FreeType font face.
    Font(std::string const& font_file);
public:
    // Destroys the FreeType font face.
    ~Font() noexcept;

// --- Glyph functions ---

    void setCharsize(int charsize);
    // Loads corresponding glyph.
    bool loadGlyph(FT_UInt glyph_index, int charsize, int outline_size);
    // Clears out the internal glyph cache.
    void unloadGlyphs() noexcept;

// --- Get functions ---

    // Returns the internal FreeType font face.
    inline FT_Face_Ptr const& getFTFace() const noexcept { return _face; };
    // Returns the corresponding internal HarfBuzz font.
    HB_Font_Ptr const& getHBFont(int charsize) const;
    // Returns corresponding glyph as a bitmap
    Bitmap const&
        getGlyphBitmap(FT_UInt glyph_index, int charsize) const;
    // Returns corresponding glyph outline as a bitmap
    Bitmap const&
        getOutlineBitmap(FT_UInt glyph_index, int charsize, int outline_size) const;

private:
// --- Private Variables ---

    // Font face
    FT_Face_Ptr _face;
    // Map of different font charsizes
    FontSize::Map _font_sizes;

// --- Private functions ---

    // Ensures the given charsize has been initialized
    void _throw_if_bad_charsize(int &charsize) const;
};


INTERNAL_END;
SSS_TR_END;
