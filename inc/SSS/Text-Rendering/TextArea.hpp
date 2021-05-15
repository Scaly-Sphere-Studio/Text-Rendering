#pragma once

#include "SSS/Text-Rendering/Buffer.hpp"

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

class TextArea : public std::enable_shared_from_this<TextArea> {
    friend class Buffer;
public:

// --- Aliases ---
    using Shared = std::shared_ptr<TextArea>;
protected:
    using Weak = std::weak_ptr<TextArea>;

    static std::vector<Weak> _instances;

// --- Constructor, destructor & clear function ---
    
    // Constructor, sets width & height.
    // Throws an exception if width and/or height are <= 0.
    TextArea(int width, int height);
public:
    // Destructor, clears out buffer cache.
    ~TextArea() noexcept;
    // Resets the object to newly constructed state
    void clear() noexcept;

    static Shared create(int width, int height);

// --- Basic functions ---

    // Use buffer in text area
    void useBuffer(Buffer::Shared buffer);
    // Renders text to a 2D pixel array in the RGBA32 format.
    // The passed array should have the same width and height
    // as the TextArea object.
    void renderTo(void* pixels);
    // Returns its rendered pixels.
    void const* getPixels();

// --- Format functions ---

    // Scrolls up (negative values) or down (positive values).
    // The function returns true if the scrolling didn't change.
    // Any excessive scrolling will be negated, and the function
    // will return false.
    bool scroll(int pixels) noexcept;

// --- Edit functions ---

    void placeCursor(int x, int y);

    enum class Cursor {
        Up,
        Down,
        Left,
        Right,
        CtrlLeft,
        CtrlRight,
        Home,
        End,
    };
    void moveCursor(Cursor position);
    void insertText(std::u32string str);
    void insertText(std::string str);

// --- Typewriter functions ---

    // Sets the writing mode (default: false):
    // - true: Text is rendered char by char, see incrementCursor();
    // - false: Text is fully rendered
    void TWset(bool activate) noexcept;
    // Increments the typewriter's cursor. Start point is 0.
    // The first call will render the 1st character.
    // Second call will render the both the 1st and 2nd character.
    // Etc...
    bool TWprint() noexcept;

    inline bool willDraw() const noexcept
        { return _update_format || _update_lines || _draw || _clear; };

private:

// --- Object info ---
    
    size_t _w;              // Width of area
    size_t _h;              // Height of area
    size_t _pixels_h{ 0 };  // Height of _pixels -> must NEVER be lower than _h
    int _scrolling{ 0 };    // Scrolling index, in pixels
    
    bool _update_format{ true }; // True -> update _lines & _scrolling
    bool _update_lines{ true }; // True -> update _lines
    bool _clear{ true };        // True -> clear _pixels before drawing
    bool _resize{ true };       // True -> resize _pixels before drawing
    bool _draw{ true };         // True -> (re)draw _pixels
    bool _typewriter{ false };  // True -> display characters one by one

    RGBA32::Pixels _pixels;  // Pixels vector

    std::vector<Buffer::Shared> _buffers;   // Buffer array for multiple layouts

    size_t _glyph_count{ 0 };   // Total number of glyphs in all ACTIVE buffers
    size_t _edit_cursor{ size_t(-1) };  // Cursor used in edit
    int _edit_x{ 0 }, _edit_y{ 0 };     // Cursor 
    size_t _tw_cursor{ 0 };     // TypeWriter -> Current character position
    size_t _tw_next_cursor{ 0}; // TypeWriter -> Next character position

    _internal::Line::vector _lines;   // Indexes of line breaks & charsizes

// --- Private functions ---

    // Update the text format.
    // To be called when a charsize changes internally, for example.
    void _updateFormat();
    // Calls the at(); function from corresponding Buffer
    _internal::GlyphInfo _at(size_t cursor) const;
    // Returns corresponding _internal::Line iterator, or cend()
    _internal::Line::cit _whichLine(size_t cursor) const noexcept;
    // Updates _scrolling
    void _scrollingChanged() noexcept;
    // Updates _lines
    void _updateLines();

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
        inline _CopyBitmapArgs(_internal::Bitmap const& _bitmap)
            : bitmap(_bitmap) {};
        // Bitmap
        _internal::Bitmap const& bitmap;
        // Coords
        FT_Int x0{ 0 };  // _pixels -> x origin
        FT_Int y0{ 0 };  // _pixels -> y origin
        // Colors
        RGB24::s color;         // Bitmap's color
        uint8_t alpha{ 0 };     // Bitmap's opacity
    };
    // Copies a bitmap with given coords and color in _pixels
    void _copyBitmap(_CopyBitmapArgs& coords);
};

__SSS_TR_END