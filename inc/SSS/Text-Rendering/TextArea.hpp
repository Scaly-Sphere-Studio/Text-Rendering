#pragma once

#include "SSS/Text-Rendering/_TextAreaInternals.hpp"

__SSS_TR_BEGIN

class TextArea {
protected:
// --- Constructor, destructor & clear function ---
    
    // Constructor, sets width & height.
    // Throws an exception if width and/or height are <= 0.
    TextArea(int width, int height);
public:
    // Destructor, clears out buffer cache.
    ~TextArea() noexcept;
    // Resets the object to newly constructed state (except for TextOpt map)
    void clear() noexcept;

    using Ptr = std::shared_ptr<TextArea>;
    using Map = std::map<uint32_t, Ptr>;
private:
    static Map _instances;
public:
    static Map const& getTextAreas() noexcept { return _instances; };
    static void createInstance(uint32_t id, int width, int height);
    static void removeInstance(uint32_t id);
    static void clearInstances() noexcept;

    void resize(int width, int height);
// --- Basic functions ---

    void setTextOpt(uint32_t id, TextOpt const& opt);
    void clearTextOpt();
    void parseString(std::u32string const& str);
    void parseString(std::string const& str);

    void update();
    inline bool changesPending() const noexcept { return _changes_pending; };
    inline void changesHandled() noexcept { _changes_pending = false; }

    // Returns its rendered pixels.
    void const* getPixels() const;

    // Fills width and height with internal values
    void getDimensions(int& w, int& h) const noexcept;

// --- Format functions ---

    // Scrolls up (negative values) or down (positive values).
    // The function returns true if the scrolling didn't change.
    // Any excessive scrolling will be negated, and the function
    // will return false.
    void scroll(int pixels) noexcept;

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
    
    int _w;                 // Width of area
    int _h;                 // Height of area
    int _pixels_h{ 0 };     // Height of _pixels -> must NEVER be lower than _h
    int _scrolling{ 0 };    // Scrolling index, in pixels
    
    bool _draw{ true };         // True -> enables _drawIfNeeded
    bool _typewriter{ false };  // True -> display characters one by one

    bool _changes_pending{ false }; // True -> update() returns true

    using _PixelBuffers = std::array<_internal::TextAreaPixels, 2>;
    _PixelBuffers _pixels;
    _PixelBuffers::const_iterator _current_pixels{ _pixels.cbegin() };
    _PixelBuffers::iterator _processing_pixels{ _pixels.begin() };

    std::map<uint32_t, TextOpt> _text_opt;          // Map of TextOpt
    std::vector<_internal::Buffer::Ptr> _buffers;   // Buffer array for multiple layouts
    _internal::BufferInfoVector _buffer_infos;      // Buffer infos

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
    // Updates _buffer_infos and _glyph_count, then calls _updateLines();
    void _updateBufferInfos();

// --- Private functions -> Draw functions ---

    // Draws current area if _draw is set to true
    void _drawIfNeeded();
    // Prepares drawing parameters, which will be used multiple times per draw
    _internal::DrawParameters _prepareDraw();
};

__SSS_TR_END