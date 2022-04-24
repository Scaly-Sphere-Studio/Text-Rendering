#pragma once

#include "pointers.hpp"

/** @file
 *  Defines internal font sizes management classes.
 */

__SSS_TR_BEGIN;
__INTERNAL_BEGIN;

struct Bitmap {
    int pen_left;
    int pen_top;

    int width;
    int height;
    int bpp;

    std::vector<unsigned char> buffer;
    unsigned char pixel_mode;
};

// This class aims to be used within the Font class to load, store,
// and access glyphs (and their possible outlines) of a given charsize.
class FontSize {
public:
// --- Aliases ---
    using Map = std::map<int, FontSize>;

// --- Constructor & Destructor ---

    // Constructor, throws if invalid charsize
    FontSize(FT_Face_Ptr const& ft_face, int charsize);
    // Destructor. Logs
    ~FontSize();

// --- Load functions ---

    // Change FT face charsize
    void setCharsize();
    // Loads the given glyph, and its ouline if outline_size > 0.
    // Returns true on error.
    bool loadGlyph(FT_UInt glyph_index, int outline_size);

// --- Get functions ---

    // Returns the corresponding glyph's bitmap. Throws if not found.
    Bitmap const& getGlyphBitmap(FT_UInt glyph_index) const;
    // Returns the corresponding glyph outline's bitmap. Throws if not found.
    Bitmap const& getOutlineBitmap(FT_UInt glyph_index, int outline_size) const;
    // Returns the corresponding HarfBuzz font
    inline HB_Font_Ptr const& getHBFont() const noexcept { return _hb_font; }

private:
// --- Private Variables ---

    int _charsize;                  // Charsize
    int _last_outline_size{ 0 };    // Last outline size used with this charsize
    HB_Font_Ptr _hb_font;           // HarfBuzz font, created here
    FT_Stroker_Ptr _stroker;        // FreeType stroker, created here
    FT_Face_Ptr const& _ft_face;    // FreeType font face, given

    // Map of glyph bitmaps
    std::map<FT_UInt, Bitmap> _originals;
    // Map of outline bitmaps, mapped by outline size
    std::map<FT_UInt, std::map<FT_UInt, Bitmap>> _outlined;
};

__INTERNAL_END;
__SSS_TR_END;
