#pragma once

#include "SSS/Text-Rendering/_includes.hpp"
#include "SSS/Text-Rendering/_pointers.hpp"

__SSS_TR_BEGIN
__INTERNAL_BEGIN

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

// --- Glyph functions ---

    // Loads the given glyph, and its ouline if outline_size > 0.
    // Returns true on error.
    bool loadGlyph(FT_UInt glyph_index, int outline_size);

    // Returns the corresponding glyph's bitmap. Throws if not found.
    FT_BitmapGlyph_Ptr const&
        getGlyphBitmap(FT_UInt glyph_index) const;
    // Returns the corresponding glyph outline's bitmap. Throws if not found.
    FT_BitmapGlyph_Ptr const&
        getOutlineBitmap(FT_UInt glyph_index, int outline_size) const;

// --- Get functions ---

    // Returns the corresponding HarfBuzz font
    inline HB_Font_Ptr const& getHBFont() const noexcept { return _hb_font; }

private:
// --- Private Variables ---

    int _charsize;                  // Charsize
    int _last_outline_size{ 0 };    // Last outline size used with this charsize
    HB_Font_Ptr _hb_font;           // HarfBuzz font, created here
    FT_Stroker_Ptr _stroker;        // FreeType stroker, created here
    FT_Face_Ptr const& _ft_face;    // FreeType font face, given

    struct _Glyph {
        // Variables
        FT_Glyph_Ptr glyph;
        FT_BitmapGlyph_Ptr bitmap;
        // Aliases
        using Map = std::map<FT_UInt, _Glyph>;
        using Map2D = std::map<FT_UInt, _Glyph::Map>;
    };
    _Glyph::Map _originals;     // Map of glyphs
    _Glyph::Map2D _outlined;    // Map of outline map of glyphs

// --- Private Functions ---

    // Change FT face charsize
    void _setCharsize();
    // Loads the original glyph. Returns true on error
    bool _loadOriginal(FT_UInt glyph_index);
    // Loads the glyph's outline. Returns true on error
    bool _loadOutlined(FT_UInt glyph_index, int outline_size);
    // Stores the glyph and its ouline's bitmaps. Returns true on error
    bool _storeBitmaps(FT_UInt glyph_index, int outline_size);
};

__INTERNAL_END
__SSS_TR_END