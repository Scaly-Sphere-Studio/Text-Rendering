#pragma once

#include "SSS/Text-Rendering/Font.hpp"

SSS_TR_BEGIN__

// This class aims to be used within the Font class to load, store,
// and access glyphs (and their possible outlines) of a given charsize.
class _FontSize {
public:
// --- Constructor ---

    // Constructor, throws if invalid charsize
    _FontSize(FT_Face_Ptr const& ft_face, int charsize);

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
    HB_Font_Ptr const& getHBFont() const noexcept;

private:
// --- Private Variables ---

    int charsize_;                  // Charsize
    int last_outline_size_{ 0 };    // Last outline size used with this charsize
    HB_Font_Ptr hb_font_;           // HarfBuzz font, created here
    FT_Stroker_Ptr stroker_;        // FreeType stroker, created here
    FT_Face_Ptr const& ft_face_;    // FreeType font face, given

    struct _Glyph {
        // Variables
        FT_Glyph_Ptr glyph;
        FT_BitmapGlyph_Ptr bitmap;
        // Aliases
        using Map = std::map<FT_UInt, _Glyph>;
        using Map2D = std::map<FT_UInt, _Glyph::Map>;
    };
    _Glyph::Map originals_;     // Map of glyphs
    _Glyph::Map2D outlined_;    // Map of outline map of glyphs

// --- Private Functions ---

    // Change FT face charsize
    void setCharsize_();
    // Loads the original glyph. Returns true on error
    bool loadOriginal_(FT_UInt glyph_index);
    // Loads the glyph's outline. Returns true on error
    bool loadOutlined_(FT_UInt glyph_index, int outline_size);
    // Stores the glyph and its ouline's bitmaps. Returns true on error
    bool storeBitmaps_(FT_UInt glyph_index, int outline_size);
};

SSS_TR_END__