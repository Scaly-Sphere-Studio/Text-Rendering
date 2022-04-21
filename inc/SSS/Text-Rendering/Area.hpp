#pragma once

#include "_internal/AreaInternals.hpp"

__SSS_TR_BEGIN;

enum class Move;    // Pre-declaration
enum class Delete;  // Pre-declaration

/** The class handling most of the text rendering logic.
 *
 *  Use static functions to create() instances, and retrieve
 *  them with getMap().
 * 
 *  Customize multiple text formats via setFormat(), which will be used
 *  in the parseString() functions.
 * 
 *  Don't forget to update() the changes, and retrieve resulting pixels
 *  with pixelsGet() when pixelsWereChanged() returns \c true.
 * 
 *  Scroll up and down with scroll(), or use setDimensions() to resize
 *  the text area.
 * 
 *  Place and move the editing cursor with cursorPlace() and cursorMove(),
 *  and add or remove text with cursorAddText() and cursorDeleteText().
 * 
 *  @sa Format, init(), loadFont()
 */
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

    // Managed by update(), used in "pixelsXXX" functions
    bool _changes_pending{ false };

    // Double-Buffer array
    using _PixelBuffers = std::array<_internal::AreaPixels, 2>;
    // Async processing buffers
    _PixelBuffers _pixels;
    // Const iterator used in pixelsGet(), managed by update()
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
    // Managed in "Cursor" functions, used in "cursorAddText" functions
    // Set to first be at the end of text by default.
    size_t _edit_cursor{ 0 };
    // Cursor physical position
    int _edit_x{ 0 }, _edit_y{ 0 };

    // TypeWriter -> Current character position, in glyphs
    size_t _tw_cursor{ 0 };

    // Indexes of line breaks & charsizes
    std::vector<_internal::Line> _lines;

public:
    /** Unique instance pointer, which are stored in a Map.
     *  This is the only way to refer to Area instances.
     *  @sa create(), remove().
     */
    using Ptr = std::unique_ptr<Area>;
    /** Instance map, stored by IDs.
     *  This is the only way to refer to Area instances.
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
     *  Can only be indirectly called.
     *  @sa remove() and clearMap().
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
     *  @sa Format.
     */
    void setFormat(Format const& src, uint32_t id = 0);

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
    
    /** Clears the internal string & pixels.
     *  Also resets scrolling, cursors.\n
     *  Keeps previous format modifications & dimensions.
     *  @sa update(), parseString(), pixelsGet();
     */
    void clear() noexcept;
    /** Draws modifications when needed, and sets the return value of
     *  pixelsWereChanged() to \c true when changes are finished.
     *  @usage To be called as often as possible in your main loop.
     *  @sa clear(), pixelsGet().
     */
    void update();
    
    /** Returns \c true when internal pixels were modified and the
     *  user needs to retrieve them.
     *  When said pixels are retrieved, pixelsAreRetrieved() needs to
     *  be called.
     *  @return \c true when new pixels are to be retrieved, and \c false
     *  otherwise.
     *  @sa update(), pixelsGet().
     */
    inline bool pixelsWereChanged() const noexcept { return _changes_pending; };
    /** Tells the Area instance that the new pixels were retrieved by the user.
     *  This effectively sets the return value of pixelsWereChanged() to \c false.
     *  @sa update(), pixelsGet().
     */
    inline void pixelsAreRetrieved() noexcept { _changes_pending = false; }
    /** Returns a const pointer to the internal pixels array.
     *  
     *  The returned pointer stays valid until -- at least -- the next call
     *  to update(). After that, there is no guarantee for the pointer
     *  to be valid.
     *  
     *  Accessible range is of <tt>Width * Height * 4</tt>.
     * 
     *  @sa update(), clear(), getDimensions().
     */
    void const* pixelsGet() const;

    /** Retrieves dimensions of current pixels.
     *  Current pixels are the ones returned by pixelsGet().\n
     *  If you just called setDimensions(), this function won't retrieve
     *  new values until pixelsWereChanged() returns \c true.
     *  @param[out] width Will be filled with pixels width.
     *  @param[out] height Will be filled with pixels height.
     */
    void getDimensions(int& width, int& height) const noexcept;
    /** Sets new dimensions values for internal pixels.
     *  A call to update() is necessary for changes to take effect.
     *  @param[in] width Defines the pixels width. Must be above \c 0.
     *  @param[in] height Defines the pixels height. Must be above \c 0.
     *  @throw std::runtime_error If at least one of the given values
     *  
     */
    void setDimensions(int width, int height);
    /** Scrolls up with negative values, and down with positive values.
     *  Takes effect immediatley and sets pixelsWereChanged() to \c true.\n
     *  Trying to scroll too high or too low will have no effect.
     *  param[in] pixels The amount of pixels to scroll in either direction.
     */
    void scroll(int pixels) noexcept;

    /** Places the editing cursor at given coordinates.
     *  The cursor is by default at the end of the text.
     *  @param[in] x The X coordinate to place the cursor to.
     *  @param[in] y The Y coordinate to place the cursor to.
     *  @sa cursorMove(), cursorAddText(), cursorDeleteText()
     */
    void cursorPlace(int x, int y);
    /** Moves the editing cursor in given direction.
     *  The cursor is by default at the end of the text.
     *  @param[in] direction The direction for the cursor to be moved.
     *  @sa Move, cursorPlace(), cursorAddText(), cursorDeleteText()
     */
    void cursorMove(Move direction);
    /** Inserts text at the cursor's position.
     *  The cursor is by default at the end of the text.
     *  @param[in] str The UTF32 string to be added to existing text.
     *  @sa cursorPlace(), cursorMove(), cursorDeleteText()
     */
    void cursorAddText(std::u32string str);
    /** \overload*/
    void cursorAddText(std::string str);
    /** Deletes text at the cursor's position, in the given direction.
     *  The cursor is by default at the end of the text.
     *  @param[in] direction The deletion direction.
     *  @sa Delete, cursorPlace(), cursorMove(), cursorAddText()
     */
    void cursorDeleteText(Delete direction);

    /** Enables or disables the \b typewriter mode (disabled by default).
     *  The typewriter mode makes characters being written one by one
     *  instead of being all rendered instantly.\n
     *  This is typically useful in Visual Novel games.
     */
    void twSet(bool activate) noexcept;

    /** \cond TODO*/
    bool twPrint() noexcept;
    /** \endcond*/

private:
    // Computes _edit_cursor's relative position on the Area
    void _getCursorPhysicalPos(int& x, int& y) const noexcept;
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