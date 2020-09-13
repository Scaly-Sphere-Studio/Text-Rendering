#pragma once

#include <SSS/Text-Rendering/includes.h>

SSS_TR_BEGIN__

// unique_ptr aliases
using FT_Library_Ptr = std::unique_ptr<FT_LibraryRec_, FT_Error(*)(FT_Library)>;
using FT_Face_Ptr = std::unique_ptr<FT_FaceRec_, FT_Error(*)(FT_Face)>;
using FT_Glyph_Ptr = std::unique_ptr<FT_GlyphRec_, void(*)(FT_Glyph)>;
using FT_BitmapGlyph_Ptr = std::unique_ptr<FT_BitmapGlyphRec_, void(*)(FT_BitmapGlyph)>;
using FT_Stroker_Ptr = std::unique_ptr<FT_StrokerRec_, void(*)(FT_Stroker)>;
using HB_Font_Ptr = std::unique_ptr<hb_font_t, void(*)(hb_font_t*)>;

// Necesary for FT_BitmapGlyph_Ptr destructor.
// Simply calls FT_Done_Glyph with a cast
void FT_Done_BitmapGlyph(FT_BitmapGlyph glyph) noexcept;

struct Bitmaps {
    // Constructor, specifies the bitmaps destructors
    Bitmaps() noexcept :
        original(nullptr, FT_Done_BitmapGlyph), // Specify FT_BitmapGlyph destructor
        stroked(nullptr, FT_Done_BitmapGlyph)   // Specify FT_BitmapGlyph destructor
    {};
    FT_BitmapGlyph_Ptr original;
    FT_BitmapGlyph_Ptr stroked;
};

class Font {
#ifndef NDEBUG
private:
    static constexpr bool log_constructor_ = true;
    static constexpr bool log_destructor_ = true;
#endif // NDEBUG

private:
    // Static variables
    static FT_Library_Ptr lib_; // FreeType lib pointer
    static size_t instances_; // Number of object instances
    static FT_UInt hdpi_; // Screen's horizontal dpi
    static FT_UInt vdpi_; // Screen's vertical dpi

public:
    // --- Constructor & Destructor ---

    // Constructor, inits FreeType if called for the first time.
    // Creates a FreeType font face & a HarfBuzz font.
    Font(std::string const& path, int charsize = 12, int outline_size = 0);
    // Destructor, quits FreeType if called from last remaining instance.
    // Destroys the FreeType font face & the HarfBuzz font.
    ~Font() noexcept;

    // --- Glyph functions ---

    // Loads corresponding glyph.
    bool loadGlyph(FT_UInt index);
    // Clears out the internal glyph cache.
    void unloadGlyphs() noexcept;

    // --- Set functions ---

    // Sets screen DPI for all instances (default: 96x96).
    static void setDPI(FT_UInt hdpi, FT_UInt vdpi) noexcept;
    // Changes character size and recreates glyph cache.
    void setCharsize(int charsize);
    // Changes outline size and recreates glyph cache.
    void setOutline(int size);

    // --- Get functions ---

    // Returns corresponding loaded glyph.
    // Throws exception if none corresponds to passed index.
    Bitmaps const& getBitmaps(FT_UInt index) const;
    // Returns the internal FreeType font face.
    FT_Face_Ptr const& getFTFace() const noexcept;
    // Returns the internal HarfBuzz font.
    HB_Font_Ptr const& getHBFont() const noexcept;
    // Returns current character size.
    int getCharsize() const noexcept;
    // Returns current outline size.
    int getOutline() const noexcept;

private:
    // Object info
    FT_Face_Ptr face_;          // Font face
    HB_Font_Ptr hb_font_;       // HarfBuzz font
    int charsize_;              // Charsize (in 32.0)
    int outline_size_;          // Outline size, in percents. 0 = no outline
    FT_Stroker_Ptr stroker_;    // Used to create outlines

    std::map<FT_UInt, Bitmaps> bitmaps_; // Map of loaded glyphs
};

using Font_Ptr = std::unique_ptr<Font>;
using Font_Shared = std::shared_ptr<Font>;

SSS_TR_END__