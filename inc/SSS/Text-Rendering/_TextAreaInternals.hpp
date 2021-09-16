#pragma once

#include "_includes.hpp"
#include "Buffer.hpp"

__SSS_TR_BEGIN
__INTERNAL_BEGIN

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
    
    static cit which(vector const& lines, size_t cursor);
};

// Draw parameters
struct DrawParameters {
    // Variables
    size_t first_glyph{ 0 };// First glyph to draw
    size_t last_glyph{ 0 }; // Last glyph to draw (excluded)
    FT_Vector pen{ 0, 0 };  // Pen on the canvas
    Line::cit line;         // Line of the cursor

    // Draw type : { false, false } would draw simple text,
    // and { true, true } would draw the shadows of the outlines
    struct {
        // Variables
        bool is_shadow{ true };     // Draw text or its shadow
        bool is_outline{ true };    // Draw glyphs or their outlines
    } type;     // Glyph type
};

class TextAreaPixels : public ThreadBase<DrawParameters> {
public:
    ~TextAreaPixels();

private:
    using ThreadBase::run;

public:
    void draw(DrawParameters param, int w, int h, int pixels_h,
        std::vector<Line> lines, BufferInfoVector glyph_infos);
    inline RGBA32::Pixels const& getPixels() const noexcept { return _pixels; };
    inline void getDimensions(int& w, int& h) const noexcept { w = _w; h = _h; };

protected:
    virtual void _function(DrawParameters param);

private:

    int _w{ 0 };
    int _h{ 0 };
    int _pixels_h{ 0 };
    RGBA32::Pixels _pixels;

    std::vector<Line> _lines;
    BufferInfoVector _buffer_infos;

    struct _CopyBitmapArgs {
        inline _CopyBitmapArgs(Bitmap const& _bitmap)
            : bitmap(_bitmap) {};
        // Bitmap
        Bitmap const& bitmap;
        // Coords
        FT_Int x0{ 0 };  // _pixels -> x origin
        FT_Int y0{ 0 };  // _pixels -> y origin
        // Colors
        RGB24::s color;         // Bitmap's color
        uint8_t alpha{ 0 };     // Bitmap's opacity
    };

    void _drawGlyphs(DrawParameters param);
    void _drawGlyph(DrawParameters param, BufferInfo const& buffer_info, GlyphInfo const& glyph_info);
    void _copyBitmap(_CopyBitmapArgs& args);
};

__INTERNAL_END
__SSS_TR_END