#ifndef SSS_TR_FONTSIZE_HPP
#define SSS_TR_FONTSIZE_HPP

#include "Lib.hpp"

/** @file
 *  Defines internal font sizes management classes.
 */

namespace SSS::Log::TR {
    /** Logging properties for SSS::TR internal fonts.*/
    struct Fonts : public LogBase<Fonts> {
        using LOG_STRUCT_BASICS(TR, Fonts);
        /** Logs constructors and destructors of fonts and font sizes.*/
        bool life_state = false;
        /** Logs loading and unloading of glyphs.*/
        bool glyph_load = false;
    };
}

SSS_TR_BEGIN;
INTERNAL_BEGIN;

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
    FontSize(FT_Face ft_face, int charsize);
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
    inline hb_font_t* getHBFont() const noexcept { return _hb_font.get(); }

private:
// --- Private Variables ---

    int const _charsize;            // Charsize set in constructor
    int _last_outline_size{ 0 };    // Last outline size used with this charsize
    HB_Font_Ptr _hb_font;           // HarfBuzz font, created here
    FT_Stroker_Ptr _stroker;        // FreeType stroker, created here
    FT_Face _ft_face;               // FreeType font face, given

    // Map of glyph bitmaps
    std::map<FT_UInt, Bitmap> _originals;
    // Map of outline bitmaps, mapped by outline size
    std::map<FT_UInt, std::map<FT_UInt, Bitmap>> _outlined;
};

INTERNAL_END;
SSS_TR_END;

#endif // SSS_TR_FONTSIZE_HPP