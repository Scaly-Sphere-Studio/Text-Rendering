#pragma once

#include "SSS/Text-Rendering/_Buffer.hpp"

__SSS_TR_BEGIN

    // --- Internal structures ---

__INTERNAL_BEGIN
// Stores line informations
struct Line {
    // Variables
    size_t first_glyph{ 0 }; // First glyph of the line
    size_t last_glyph{ 0 };  // Line break after rendering glyph
    size_t fullsize{ 0 };    // Line's full size, in pixels
    int charsize{ 0 };       // The highest charsize on the line
    int scrolling{ 0 };      // Total scrolling for this line to be above the top
    // Aliases
    using vector = std::vector<Line>;
    using it = vector::iterator;
    using cit = vector::const_iterator;
};

// Draw parameters
struct DrawParameters {
    // Variables
    size_t first_glyph{ 0 }; // First glyph to draw
    size_t last_glyph{ 0 };  // Last glyph to draw (excluded)
    FT_Vector pen{ 0, 0 };      // Pen on the canvas
    Line::cit line;      // _internal::Line of the cursor
    
    // Draw type : { false, false } would draw simple text,
    // and { true, true } would draw the shadows of the outlines
    struct {
        // Variables
        bool is_shadow{ true };     // Draw text or its shadow
        bool is_outline{ true };    // Draw glyphs or their outlines
    } type;     // Glyph type
};
__INTERNAL_END

    // --- Class ---

class TextArea {
public:

// --- Aliases ---
    using Ptr = std::unique_ptr<TextArea>;      // Unique ptr
    using Shared = std::shared_ptr<TextArea>;   // Shared ptr

// --- Constructor, destructor & clear function ---

    // Constructor, sets width & height.
    // Throws an exception if width and/or height are <= 0.
    TextArea(int width, int height);
    // Destructor, clears out buffer cache.
    ~TextArea() noexcept;
    // Resets the object to newly constructed state
    void clear() noexcept;

// --- Basic functions ---

    // Loads passed string in cache.
    void loadString(std::u32string const& str, TextOpt const& opt);
    // Loads passed string in cache.
    void loadString(std::string const& str, TextOpt const& opt);
    // Renders text to a 2D pixel array in the RGBA32 format.
    // The passed array should have the same width and height
    // as the TextArea object.
    void renderTo(void* pixels);

// --- Format functions ---

    // Update the text format.
    // To be called when a charsize changes internally, for example.
    void updateFormat();
    // Scrolls up (negative values) or down (positive values)
    // Any excessive scrolling will be negated,
    // hence, this function is safe.
    void scroll(int pixels) noexcept;

// --- Typewriter functions ---

    // Sets the writing mode (default: false):
    // - true: Text is rendered char by char, see incrementCursor();
    // - false: Text is fully rendered
    void setTypeWriter(bool activate) noexcept;
    // Increments the typewriter's cursor. Start point is 0.
    // The first call will render the 1st character.
    // Second call will render the both the 1st and 2nd character.
    // Etc...
    bool incrementCursor() noexcept;

private:

// --- Object info ---
    
    size_t _w;              // Width of area
    size_t _h;              // Height of area
    size_t _pixels_h{ 0 };  // Height of _pixels -> must NEVER be lower than _h
    int _scrolling{ 0 };    // Scrolling index, in pixels
    
    bool _clear{ true };        // True -> clear _pixels before drawing
    bool _resize{ true };       // True -> resize _pixels before drawing
    bool _draw{ true };         // True -> (re)draw _pixels
    bool _typewriter{ false };  // True -> display characters one by one

    RGBA32::Pixels _pixels;  // Pixels vector

    _internal::Buffer::vector _buffers;   // Buffer array for multiple layouts
    size_t _buffer_count{ 0 };  // Number of ACTIVE buffers, != _buffers.size()
    size_t _glyph_count{ 0 };   // Total number of glyphs in all ACTIVE buffers
    size_t _tw_cursor{ 0 };     // TypeWriter -> Current character position
    size_t _tw_next_cursor{ 0}; // TypeWriter -> Next character position

    _internal::Line::vector _lines;   // Indexes of line breaks & charsizes

// --- Private functions ---

    // Calls the at(); function from corresponding Buffer
    _internal::GlyphInfo _at(size_t cursor) const;
    // Returns corresponding _internal::Line iterator, or cend()
    _internal::Line::cit _which_Line(size_t cursor) const noexcept;
    // Updates _scrolling
    void _scrollingChanged() noexcept;
    // Updates _lines
    void _update_Lines();

// --- Private functions -> Draw functions ---

    // Draws current area if _draw is set to true
    void _drawIfNeeded();
    // Prepares drawing parameters, which will be used multiple times per draw
    _internal::DrawParameters _prepareDraw();
    // Draws shadows, outlines, or plain glyphs
    void _drawGlyphs(_internal::DrawParameters param);
    // Draws a single shadow, outline, or plain glyph
    void _drawGlyph(_internal::DrawParameters param, _internal::GlyphInfo const& glyph);

    struct _CopyBitmapArgs {
        // Coords
        FT_Int x0{ 0 };  // _pixels -> x origin
        FT_Int y0{ 0 };  // _pixels -> y origin
        // Bitmap
        FT_Bitmap bitmap{ 0 };  // Bitmap structure
        RGB24::s color;         // Bitmap's color
        uint8_t alpha{ 0 };     // Bitmap's opacity
    };
    // Copies a bitmap with given coords and color in _pixels
    void _copyBitmap(_CopyBitmapArgs& coords);
};

__SSS_TR_END