#pragma once

#include "SSS/Text-Rendering/_includes.hpp"
#include "SSS/Text-Rendering/_pointers.hpp"
#include "SSS/Text-Rendering/_FontSize.hpp"

SSS_TR_BEGIN__

INTERNAL_BEGIN__
class FontSize;
using StringDeque = std::deque<std::string>;
INTERNAL_END__

class Font {

    friend class _internal::FontSize;

private:
// --- Static variables ---

    // Internals
    static _internal::FT_Library_Ptr lib_;      // FreeType lib pointer
    static _internal::StringDeque font_dirs_;   // Font directories
    // Simple uints
    static size_t instances_;   // Number of object instances
    static FT_UInt hdpi_;       // Screen's horizontal dpi
    static FT_UInt vdpi_;       // Screen's vertical dpi


public:
// --- Aliases ---

    using Ptr = std::unique_ptr<Font>;
    using Shared = std::shared_ptr<Font>;

// --- Static functions ---

    // Sets screen DPI for all instances (default: 96x96).
    static void setDPI(FT_UInt hdpi, FT_UInt vdpi) noexcept;

// --- Constructor & Destructor ---

    // Constructor, inits FreeType if called for the first time.
    // Creates a FreeType font face.
    Font(std::string const& font_file);
    // Destructor, quits FreeType if called from last remaining instance.
    // Destroys the FreeType font face.
    ~Font() noexcept;

// --- Glyph functions ---

    void useCharsize(int charsize);
    // Loads corresponding glyph.
    bool loadGlyph(FT_UInt glyph_index, int charsize, int outline_size);
    // Clears out the internal glyph cache.
    void unloadGlyphs() noexcept;

// --- Get functions ---

    // Returns corresponding glyph as a bitmap
    _internal::FT_BitmapGlyph_Ptr const&
        getGlyphBitmap(FT_UInt glyph_index, int charsize) const;
    // Returns corresponding glyph outline as a bitmap
    _internal::FT_BitmapGlyph_Ptr const&
        getOutlineBitmap(FT_UInt glyph_index, int charsize, int outline_size) const;
    // Returns the internal FreeType font face.
    _internal::FT_Face_Ptr const& getFTFace() const noexcept;
    // Returns the corresponding internal HarfBuzz font.
    _internal::HB_Font_Ptr const& getHBFont(int charsize) const;

private:
// --- Private Variables ---

    // Font face
    _internal::FT_Face_Ptr face_;
    // Map of different font charsizes
    std::map<int, _internal::FontSize> font_sizes_;

// --- Private functions ---

    // Ensures the given charsize has been initialized
    void throw_if_bad_charsize_(int charsize) const;
};


SSS_TR_END__