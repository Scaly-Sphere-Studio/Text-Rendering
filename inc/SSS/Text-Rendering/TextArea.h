#pragma once

#include <SSS/Text-Rendering/includes.h>
#include <SSS/Text-Rendering/Buffer.h>

SSS_TR_BEGIN__

// TextArea options
struct TextAreaOpt {
};

// Stores line informations
struct Line {
    size_t first_glyph; // First glyph of the line
    size_t last_glyph;  // Line break after rendering glyph
    size_t fullsize;    // Line's full size
    int charsize;       // The highest charsize on the line
    int scrolling;      // Total scrolling for this line to be above the top
};
using Lines = std::vector<Line>;
using Line_it = Lines::iterator;
using Line_cit = Lines::const_iterator;

// Glyph type
struct GlyphType {
    GlyphType(
        bool is_stroked_,
        bool is_shadow_
    ) :
        is_stroked(is_stroked_),
        is_shadow(is_shadow_)
    {};
    bool is_stroked;    // Whether the glyph is the original or its stroked variant
    bool is_shadow;     // Whether the glyph is a shadow or plain text
};

// Draw parameters
struct DrawParameters {
    DrawParameters() :
        first_glyph(0),
        last_glyph(0),
        pen({ 0, 0 }),
        line(),
        type(false, false)
    {};
    size_t first_glyph; // First glyph to draw
    size_t last_glyph;  // Last glyph to draw (excluded)
    FT_Vector pen;      // Pen on the canvas
    Line_cit line;      // Line of the cursor
    GlyphType type;     // Glyph type
};

class TextArea {
public:
    // Constructor, sets width & height.
    // Throws an exception if width and/or height are <= 0.
    TextArea(int width, int height, TextAreaOpt const& opt = TextAreaOpt());
    // Destructor, clears out buffer cache.
    ~TextArea() noexcept;
    // Resets the object to newly constructed state
    void clear() noexcept;

    // Loads passed string in cache.
    void loadString(Font_Shared font, std::u32string const& str, BufferOpt const& opt = BufferOpt());
    // Loads passed string in cache.
    void loadString(Font_Shared font, std::string const& str, BufferOpt const& opt = BufferOpt());
    // Update the text format. To be called when charsize changes, for example.
    void updateFormat();
    // Renders text to a 2D pixel array in the BGRA32 format.
    // The passed array should have the same width and height
    // as the TextArea object.
    void renderTo(void* pixels);

    // Sets the writing mode (default: false):
    // - true: No text is rendered at first, see writeNextChar();
    // - false: Text is fully rendered
    void setTypeWriter(bool activate);

    // Increments the typewriter mode's cursor
    bool incrementCursor();

    // Scrolls down the text area, allowing next characters to be
    // written. If scrolling down is not possible, this function
    // returns *true*.
    bool scroll(int pixels);

private:

    // --- Object info ---
    
    TextAreaOpt opt_; // Options

    size_t w_; // Width in pixels
    size_t h_; // Height in pixels
    size_t pixels_h_; // Height of pixels vector. Should never be lower than h_
    int scrolling_; // Scrolling index
    
    bool resize_;
    bool clear_;
    bool draw_;
    bool typewriter_; // Wether characters should be displayed one by one

    BGRA32_Pixels pixels_; // Pixels vector

    Buffers buffers_; // BufferS to handle multiple layoutS at once
    size_t buffer_count_; // Number of ACTIVE buffers (!= buffers_.size())
    size_t glyph_count_; // Total number of glyphs in buffers
    size_t tw_cursor_; // Current character position in typewriter mode
    size_t tw_next_cursor_; // Current character position in typewriter mode

    Lines lines_; // indexes of line breaks & charsizes

    // --- Private functions ---

    // Calls the at(); function from corresponding buffer to retrieve glyph infos
    GlyphInfo at_(size_t cursor) const;

    // Updates format_ by calling updateLineBreaks_ and updateSizes_.
    void updateInternalFormat_();

    void scrollingChanged_();

    Line_cit whichLine_(size_t cursor);

    // Draws current area if needed
    void drawIfNeeded_();
    // 
    DrawParameters prepareDraw_();
    // Renders glyphs to the pixels pointer and sets last_rendered_glyph_
    // accordingly if pixels is non null.
    void drawGlyphs_(DrawParameters param);
    // Loads (if needed) and renders the corresponding glyph to the
    // pixels pointer at the given pen's coordinates.
    void drawGlyph_(DrawParameters param, size_t cursor);
};

using TextArea_Ptr = std::unique_ptr<TextArea>;
using TextArea_Shared = std::shared_ptr<TextArea>;

SSS_TR_END__