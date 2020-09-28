#pragma once

#include "SSS/Text-Rendering/_Buffer.hpp"

SSS_TR_BEGIN__

    // --- Structures ---

// Stores line informations
struct _Line {
    // Variables
    size_t first_glyph{ 0 }; // First glyph of the line
    size_t last_glyph{ 0 };  // Line break after rendering glyph
    size_t fullsize{ 0 };    // Line's full size, in pixels
    int charsize{ 0 };       // The highest charsize on the line
    int scrolling{ 0 };      // Total scrolling for this line to be above the top
    // Aliases
    using vector = std::vector<_Line>;
    using it = vector::iterator;
    using cit = vector::const_iterator;
};

// Draw parameters
struct _DrawParameters {
    // Variables
    size_t first_glyph{ 0 }; // First glyph to draw
    size_t last_glyph{ 0 };  // Last glyph to draw (excluded)
    FT_Vector pen{ 0, 0 };      // Pen on the canvas
    _Line::cit line;      // _Line of the cursor
    // Draw type : { false, false } would draw simple text,
    // and { true, true } would draw the shadows of the outlines
    struct _Outline_Shadow {
        // Constructors
        _Outline_Shadow() {};
        _Outline_Shadow(bool is_outline_, bool is_shadow_) noexcept
            : is_outline(is_outline_), is_shadow(is_shadow_) {};
        // Variables
        bool is_outline{ true };    // Draw glyphs or their outlines
        bool is_shadow{ true };     // Draw text or its shadow
    };
    _Outline_Shadow type;   // Glyph type
};

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
    // Renders text to a 2D pixel array in the BGRA32 format.
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
    
    size_t w_;              // Width of area
    size_t h_;              // Height of area
    size_t pixels_h_{ 0 };  // Height of pixels_ -> must NEVER be lower than h_
    int scrolling_{ 0 };    // Scrolling index, in pixels
    
    bool clear_{ true };        // True -> clear pixels_ before drawing
    bool resize_{ true };       // True -> resize pixels_ before drawing
    bool draw_{ true };         // True -> (re)draw pixels_
    bool typewriter_{ false };  // True -> display characters one by one

    BGRA32_Pixels pixels_;  // Pixels vector

    _Buffer::vector buffers_;   // Buffer array for multiple layouts
    size_t buffer_count_{ 0 };  // Number of ACTIVE buffers, != buffers_.size()
    size_t glyph_count_{ 0 };   // Total number of glyphs in all ACTIVE buffers
    size_t tw_cursor_{ 0 };     // TypeWriter -> Current character position
    size_t tw_next_cursor_{ 0}; // TypeWriter -> Next character position

    _Line::vector lines_;   // Indexes of line breaks & charsizes

// --- Private functions ---

    // Calls the at(); function from corresponding Buffer
    _GlyphInfo at_(size_t cursor) const;
    // Returns corresponding _Line iterator, or cend()
    _Line::cit which_Line_(size_t cursor) const noexcept;
    // Updates scrolling_
    void scrollingChanged_() noexcept;
    // Updates lines_
    void update_Lines_();

// --- Private functions -> Draw functions ---

    // Draws current area if draw_ is set to true
    void drawIfNeeded_();
    // Prepares drawing parameters, which will be used multiple times per draw
    _DrawParameters prepareDraw_();
    // Draws shadows, outlines, or plain glyphs
    void drawGlyphs_(_DrawParameters param);
    // Draws a single shadow, outline, or plain glyph
    void drawGlyph_(_DrawParameters param, _GlyphInfo const& glyph);

    struct _CopyBitmapArgs {
        // Coords
        FT_Int x0;  // pixels_ -> x origin
        FT_Int y0;  // pixels_ -> y origin
        // Bitmap
        FT_Bitmap bitmap;   // Bitmap structure
        uint8_t alpha;      // Bitmap's opacity
        BGR24_s color;      // Bitmap's color
    };
    // Copies a bitmap with given coords and color in pixels_
    void copyBitmap_(_CopyBitmapArgs& coords);
};

SSS_TR_END__