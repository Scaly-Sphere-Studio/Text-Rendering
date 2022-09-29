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
    int used_width{ 0 };     // Line's used vertical width, in pixels
    int unused_width{ 0 };   // Line's unused vertical width, in pixels
    Alignment alignment{ Alignment::Left }; // Text alignment
    // Aliases
    using vector = std::vector<Line>;
    using it = vector::iterator;
    using cit = vector::const_iterator;
    
    static cit which(vector const& lines, size_t cursor) noexcept;
    int x_offset(bool is_ltr) const noexcept;
    // Function to replace pen when text direction changes on a line
    void replace_pen(FT_Vector& pen, BufferInfoVector const& buffer_infos, size_t cursor) const noexcept;
};

// Draw parameters
struct DrawParameters {
    FT_Vector pen{ 0, 0 }; // Pen on the canvas
    int charsize{ 0 }; // Current Line::charsize
    size_t effect_cursor{ 0 }; // Current cursor index (groups arabic glyphs)
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
    int margin_v{ 0 }; // Vertical margin of the Area, in pixels
    int margin_h{ 0 }; // Horizontal margin of the Area, in pixels
    // Cursor's physical position & height
    bool draw_cursor{ false };
    int cursor_x{ 0 }; // Cursor's x pos
    int cursor_y{ 0 }; // Cursor's y pos
    int cursor_h{ 0 }; // Cursor's height
    // Draw infos
    size_t last_glyph{ 0 }; // Last glyph to draw (excluded)
    Line::vector lines;     // Line vector
    BufferInfoVector buffer_infos; // Glyph infos
    RGBA32 bg_color{ 0 };   // Area's background clear color
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
    std::chrono::milliseconds _time;
    std::vector<FT_Vector> _rng; // Used for effects (grouped vibrations)

    struct _CopyBitmapArgs {
        inline _CopyBitmapArgs(Bitmap const& _bitmap)
            : bitmap(_bitmap) {};
        // Bitmap
        Bitmap const& bitmap;
        // Coords
        FT_Int x0{ 0 };  // _pixels -> x origin
        FT_Int y0{ 0 };  // _pixels -> y origin
        // Colors
        Color color;        // Bitmap's color
        uint8_t alpha{ 0 }; // Bitmap's opacity
    };

    void _drawGlyphs(AreaData const& data, DrawParameters param);
    void _drawGlyph(DrawParameters const& param, BufferInfo const& buffer_info, GlyphInfo const& glyph_info);
    void _copyBitmap(_CopyBitmapArgs& args);
};

INTERNAL_END;
SSS_TR_END;
