#pragma once

#include "_internal/AreaInternals.hpp"

__SSS_TR_BEGIN;

class Area {
private:
    // Width of area
    int _w;
    // Height of area
    int _h;
    // Height of _pixels -> must NEVER be lower than _h
    int _pixels_h{ 0 };
    // Scrolling index, in pixels
    int _scrolling{ 0 };

    // True -> enables _drawIfNeeded()
    bool _draw{ true };
    // True -> display characters one by one
    bool _typewriter{ false };

    // Managed by update(), used in "ChangesPending" functions
    bool _changes_pending{ false };

    // Double-Buffer array
    using _PixelBuffers = std::array<_internal::AreaPixels, 2>;
    // Async processing buffers
    _PixelBuffers _pixels;
    // Const iterator used in getPixels(), managed by update()
    _PixelBuffers::const_iterator _current_pixels{ _pixels.cbegin() };
    // Iterator managed by _drawIfNeeded()
    _PixelBuffers::iterator _processing_pixels{ _pixels.begin() };

    // Map of formats to feed to internal buffers
    std::map<uint32_t, Format> _formats;
    // Buffer vector, one for each differing format
    std::vector<_internal::Buffer::Ptr> _buffers;
    // Buffer informations
    _internal::BufferInfoVector _buffer_infos;

    // Total number of glyphs in all ACTIVE buffers
    size_t _glyph_count{ 0 };
    // Cursor's glyph related position
    // Managed in "Cursor" functions, used in "insertText" functions
    size_t _edit_cursor{ size_t(-1) };
    // Cursor physical position
    int _edit_x{ 0 }, _edit_y{ 0 };

    // TypeWriter -> Current character position, in glyphs
    size_t _tw_cursor{ 0 };
    // TypeWriter -> Next character position, in glyphs
    size_t _tw_next_cursor{ 0 };

    // Indexes of line breaks & charsizes
    std::vector<_internal::Line> _lines;

    // Updates _scrolling
    void _scrollingChanged() noexcept;
    // Updates _lines
    void _updateLines();
    // Updates _buffer_infos and _glyph_count, then calls _updateLines();
    void _updateBufferInfos();

    // Draws current area if _draw is set to true
    void _drawIfNeeded();
    // Prepares drawing parameters, which will be used multiple times per draw
    _internal::DrawParameters _prepareDraw();

public:
    /** All available inputs to move the edit cursor.
     *  @sa moveCursor().
     */
    enum class CursorInput {
        Right,      /**< Move the cursor one character to the right.*/
        Left,       /**< Move the cursor one character to the left.*/
        Down,       /**< Move the cursor one line down.*/
        Up,         /**< Move the cursor one line up.*/
        CtrlRight,  /**< Move the cursor one word to the right.*/
        CtrlLeft,   /**< Move the cursor one word to the left.*/
        Start,      /**< Move the cursor to the start of the line.*/
        End,        /**< Move the cursor to the end of the line.*/
    };

    /** Unique instance pointer.
     *  @sa Map, create(), remove().
     */
    using Ptr = std::unique_ptr<Area>;
    /** Instance map, stored by IDs.
     *  @sa create(), remove(), getMap(), clearMap().
     */
    using Map = std::map<uint32_t, Ptr>;

private:
    // Static map of allocated instances
    static Map _instances;

public:
    /** Creates an Area instance which will be stored in the internal #Map.
     *  @param[in] id The ID at which the new instance will be stored.
     *  @param[in] width The area's width, in pixels. Must be above 0.
     *  @param[in] height The area's height, in pixels. Must be above 0.
     *  @throw std::runtime_error If \c width or \c height are <= 0.
     *  @sa remove(), getMap().
     */
    static void create(uint32_t id, int width, int height);
    /** Removes an Area instance from the internal #Map.
     *  @param[in] id The ID of instance to be deleted.
     *  @sa create(), clearMap().
     */
    static void remove(uint32_t id);
    /** Returns a constant reference to the internal instance #Map.
     *  @sa create(), clearMap().
     */
    static Map const& getMap() noexcept { return _instances; };
    /** Clears the internal instance #Map.
     *  @sa remove(), getMap().
     */
    static void clearMap() noexcept;

private:
    // Constructor, sets width & height.
    // Throws an exception if width and/or height are <= 0.
    Area(int width, int height);
public:
    /** Destructor, clears internal cache.
     *  @sa remove(), clearMap().
     */
    ~Area() noexcept;

    /** Modifies internal format for given ID.
     *  @param[in] src The source format to be copied.
     *  @param[in] id The format ID to be modified. Default: \c 0.
     *  @sa Format, clearFormats().
     */
    void setFormat(Format const& src, uint32_t id = 0);
    /** Resets all internal formats to default value.
     *  @sa Format, setFormat().
     */
    void resetFormats() noexcept;

    /** Parses a UTF32 string thay may use multiple formats in itself.
     *
     *  The last ASCII control character -- US \a "unit separator" \c \\u001F --
     *  is used to parse format IDs.\n
     *  Any number in the given string that is surrounded by two "unit separator"
     *  control characters will define further text's format.\n
     *  An ID of \c 0 is used by default until any other ID is specified.\n
     *  As many IDs as desired can be used, multiple times each, in a
     *  single string.
     *
     *  An implicit call to clear() is made before parsing the string.\n
     *
     *  @param[in] str The UTF32 string to be parsed.
     *
     *  @eg
     *  Use a format ID of \c 0 for \c "Lorem ipsum":
     *  @code{.cpp}
     *  U"Lorem ipsum"
     *  @endcode
     *  Use a format ID of \c 42 for \c "Lorem ipsum":
     *  @code{.cpp}
     *  U"\u001F42\u001FLorem ipsum"
     *  @endcode
     *  Use a format ID of \c 42 for \c "Lorem ", and a format ID of \c 64
     *  for \c "ipsum":
     *  @code{.cpp}
     *  U"\u001F42\u001FLorem \u001F64\u001Fipsum"
     *  @endcode
     * 
     *  @sa setFormat()
     */
    void parseString(std::u32string const& str);
    /** \overload*/
    void parseString(std::string const& str);

    void update();
    inline bool hasChangesPending() const noexcept { return _changes_pending; };
    inline void setChangesAsHandled() noexcept { _changes_pending = false; }

    // Returns its rendered pixels.
    void const* getPixels() const;
    // Clears stored strings, buffers, and such
    void clear() noexcept;

    // Fills width and height with internal values
    void getDimensions(int& w, int& h) const noexcept;
    void resize(int width, int height);

// --- Format functions ---

    // Scrolls up (negative values) or down (positive values).
    // The function returns true if the scrolling didn't change.
    // Any excessive scrolling will be negated, and the function
    // will return false.
    void scroll(int pixels) noexcept;

// --- Edit functions ---

    void placeCursor(int x, int y);
    void moveCursor(CursorInput position);
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
};

__SSS_TR_END;