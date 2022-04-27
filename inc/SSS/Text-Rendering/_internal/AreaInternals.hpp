#pragma once

#include "Buffer.hpp"

/** @file
 *  Defines internal asynchronous drawing classes.
 */

SSS_TR_BEGIN;
INTERNAL_BEGIN;

// Stores line informations
struct Line {

    // Variables
    size_t first_glyph{ 0 }; // First glyph of the line
    size_t last_glyph{ 0 };  // Line break after rendering glyph
    int fullsize{ 0 };       // Line's full size, in pixels
    int charsize{ 0 };       // The highest charsize on the line
    int scrolling{ 0 };      // Total scrolling for this line to be above the top
    // Aliases
    using vector = std::vector<Line>;
    using it = vector::iterator;
    using cit = vector::const_iterator;
    
    static cit which(vector const& lines, size_t cursor) noexcept;
};

// Draw parameters
struct DrawParameters {
    // TODO: get actual pen values based on glyphs
    FT_Vector pen{ 5 << 6, -5 << 6 }; // Pen on the canvas
    int charsize{ 0 }; // Current Line::charsize
    // Draw type : { false, false } would draw simple text,
    // and { true, true } would draw the shadows of the outlines
    bool is_shadow{ true };     // Draw text or its shadow
    bool is_outline{ true };    // Draw glyphs or their outlines
};

struct AreaData {
    // Area size
    int w{ 0 }; // Width of the Area
    int h{ 0 }; // Height of the Area
    int pixels_h{ 0 }; // Real height of the Area
    // Cursor's physical position & height
    int cursor_x{ 0 }; // Cursor's x pos
    int cursor_y{ 0 }; // Cursor's y pos
    int cursor_h{ 0 }; // Cursor's height
    // Draw infos
    size_t last_glyph{ 0 }; // Last glyph to draw (excluded)
    Line::vector lines; // Line vector    
    BufferInfoVector buffer_infos; // Glyph infos
};

class AreaPixels : public SSS::AsyncBase<AreaData> {
public:
    inline RGBA32::Vector const& getPixels() const noexcept { return _pixels; };
    inline void getDimensions(int& w, int& h) const noexcept { w = _w; h = _h; };

private:
    virtual void _asyncFunction(AreaData param);

    int _w{ 0 };
    int _h{ 0 };
    int _pixels_h{ 0 };
    RGBA32::Vector _pixels;

    struct _CopyBitmapArgs {
        inline _CopyBitmapArgs(Bitmap const& _bitmap)
            : bitmap(_bitmap) {};
        // Bitmap
        Bitmap const& bitmap;
        // Coords
        FT_Int x0{ 0 };  // _pixels -> x origin
        FT_Int y0{ 0 };  // _pixels -> y origin
        // Colors
        Format::Color::Config color;    // Bitmap's color
        uint8_t alpha{ 0 };             // Bitmap's opacity
    };

    void _drawGlyphs(AreaData const& data, DrawParameters param);
    void _drawGlyph(DrawParameters const& param, BufferInfo const& buffer_info, GlyphInfo const& glyph_info);
    void _copyBitmap(_CopyBitmapArgs& args);
};

INTERNAL_END;
SSS_TR_END;
