#pragma once

#include "SSS/Text-Rendering/_includes.hpp"
#include "SSS/Text-Rendering/_pointers.hpp"
#include "SSS/Text-Rendering/_FontSize.hpp"

__SSS_TR_BEGIN

class Font {
public:
// --- Aliases ---

    using Ptr = std::unique_ptr<Font>;
    using Shared = std::shared_ptr<Font>;
    using Weak = std::weak_ptr<Font>;

private:
// --- Static variables ---

    // Internals
    static _internal::FT_Library_Ptr _lib;      // FreeType library pointer
    static std::deque<std::string> _font_dirs;  // Font directories (partly user defined)
    static std::map<std::string, Weak> _shared; // Shared Fonts (see getShared();)
    // Simple uints
    static FT_UInt _hdpi;   // Screen's horizontal dpi
    static FT_UInt _vdpi;   // Screen's vertical dpi

public:
// --- Static functions ---

    // Sets screen DPI for all instances (default: 96x96).
    static void setDPI(FT_UInt hdpi, FT_UInt vdpi) noexcept;
    // Adds a font directory to the system ones. To be called before creating a font.
    static void addFontDir(std::string const& font_dir);
    // Creates a shared Font instance to be used & re-used everywhere
    static Shared getShared(std::string const& font_file);

    // Returns static internal library
    static inline _internal::FT_Library_Ptr const& getFTLib() noexcept { return _lib; }
    // Returns static internal horizontal DPI (dots per inches)
    static inline FT_UInt const& getHDPI() noexcept { return _hdpi; }
    // Returns static internal vertical DPI (dots per inches)
    static inline FT_UInt const& getVDPI() noexcept { return _vdpi; }

// --- Constructor & Destructor ---

    // Constructor, inits FreeType if called for the first time.
    // Creates a FreeType font face.
    Font(std::string const& font_file);
    // Destructor, quits FreeType if called from last remaining instance.
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
    inline _internal::FT_Face_Ptr const& getFTFace() const noexcept { return _face; };
    // Returns the corresponding internal HarfBuzz font.
    _internal::HB_Font_Ptr const& getHBFont(int charsize) const;
    // Returns corresponding glyph as a bitmap
    _internal::Bitmap const&
        getGlyphBitmap(FT_UInt glyph_index, int charsize) const;
    // Returns corresponding glyph outline as a bitmap
    _internal::Bitmap const&
        getOutlineBitmap(FT_UInt glyph_index, int charsize, int outline_size) const;

private:
// --- Private Variables ---

    // Font face
    _internal::FT_Face_Ptr _face;
    // Map of different font charsizes
    _internal::FontSize::Map _font_sizes;

// --- Private functions ---

    // Ensures the given charsize has been initialized
    void _throw_if_bad_charsize(int &charsize) const;
};


__SSS_TR_END