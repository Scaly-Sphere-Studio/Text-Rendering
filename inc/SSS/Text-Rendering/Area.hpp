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
    // Set to first be at the end of text by default.
    size_t _edit_cursor{ _CRT_SIZE_MAX };
    // Cursor physical position
    int _edit_x{ 0 }, _edit_y{ 0 };

    // TypeWriter -> Current character position, in glyphs
    size_t _tw_cursor{ 0 };

    // Indexes of line breaks & charsizes
    std::vector<_internal::Line> _lines;

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

    // Constructor, sets width & height.
    // Throws an exception if width and/or height are <= 0.
    Area(int width, int height);

public:
    /** Destructor, clears internal cache.
     *  @sa remove(), clearMap().
     */
    ~Area() noexcept;
    

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
     *  Equivalent to calling remove() on every single instance.
     *  @sa remove(), getMap().
     */
    static void clearMap() noexcept;


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
     *  > A call to update() is required for changes to take effect.
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
     *  @sa setFormat().
     */
    void parseString(std::u32string const& str);
    /** \overload*/
    void parseString(std::string const& str);

    /** Draws modifications when needed, and sets the return value of
     *  hasNewPixels() to \c true when changes are finished.
     *  @usage To be called as often as possible in your main loop.
     *  @sa clear(), getPixels().
     */
    void update();
    /** Returns \c true when internal pixels were modified and the
     *  user needs to retrieve them.
     *  When said pixels are retrieved, setPixelsAsRetrieved() needs to
     *  be called.
     *  @return \c true when new pixels are to be retrieved, and \c false
     *  otherwise.
     *  @sa update(), getPixels().
     */
    inline bool hasNewPixels() const noexcept { return _changes_pending; };
    /** Tells the Area instance that the new pixels were retrieved by the user.
     *  This effectively sets the return value of hasNewPixels() to \c false.
     *  @sa update(), getPixels().
     */
    inline void setPixelsAsRetrieved() noexcept { _changes_pending = false; }

    /** Returns a pointer to the internal pixel buffer.
     *  
     *  The returned pointer stays valid until, at least, the next call
     *  to update(). After that, there is no guarantee for the pointer
     *  to be valid.
     *  
     *  Accessible range is of <tt>Width * Height * 4</tt>.
     * 
     *  @sa update(), clear(), getDimensions().
     */
    void const* getPixels() const;
    /** Clears the internal string & pixels.
     *  Also resets scrolling, editing cursor
     *  Keeps previous format modifications <em>(see resetFormats())</em>.
     *  Keeps dimensions.
     *  @sa parseString(), getPixels();
     */
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

private:
    void _getCursorPhysicalPos(int& x, int& y) const;
    // Ensures _scrolling has a valid value
    void _scrollingChanged() noexcept;
    // Updates _lines
    void _updateLines();
    // Updates _buffer_infos and _glyph_count, then calls _updateLines();
    void _updateBufferInfos();

    // Draws current area if _draw is set to true
    void _drawIfNeeded();
};

__SSS_TR_END;