#pragma once

#include "SSS/Text-Rendering/_TextAreaInternals.hpp"

__SSS_TR_BEGIN

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
    bool wasUpdated();
    // Returns its rendered pixels.
    void const* getPixels() const;

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

private:

// --- Object info ---
    
    size_t _w;              // Width of area
    size_t _h;              // Height of area
    size_t _pixels_h{ 0 };  // Height of _pixels -> must NEVER be lower than _h
    int _scrolling{ 0 };    // Scrolling index, in pixels
    
    bool _update_lines{ true }; // True -> update _lines
    bool _clear{ true };        // True -> clear _pixels before drawing
    bool _typewriter{ false };  // True -> display characters one by one

    using _PixelBuffers = std::array<_internal::TextAreaPixels, 2>;
    _PixelBuffers _pixels;
    _PixelBuffers::const_iterator _current_pixels{ _pixels.cbegin() };
    _PixelBuffers::iterator _processing_pixels{ _pixels.begin() };

    std::vector<Buffer::Shared> _buffers;   // Buffer array for multiple layouts
    _internal::BufferInfoVector _buffer_infos;   // Buffer infos

    size_t _glyph_count{ 0 };   // Total number of glyphs in all ACTIVE buffers
    size_t _edit_cursor{ size_t(-1) };  // Cursor used in edit
    int _edit_x{ 0 }, _edit_y{ 0 };     // Cursor 
    size_t _tw_cursor{ 0 };     // TypeWriter -> Current character position
    size_t _tw_next_cursor{ 0}; // TypeWriter -> Next character position

    std::vector<_internal::Line> _lines;   // Indexes of line breaks & charsizes

// --- Private functions ---
    
    // Updates _scrolling
    void _scrollingChanged() noexcept;
    // Updates _lines
    void _updateLines();

// --- Private functions -> Draw functions ---

    // Draws current area if _draw is set to true
    void _drawIfNeeded();
    // Prepares drawing parameters, which will be used multiple times per draw
    _internal::DrawParameters _prepareDraw();
};

__SSS_TR_END