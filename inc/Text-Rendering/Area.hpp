#ifndef SSS_TR_AREA_HPP
#define SSS_TR_AREA_HPP

#include "Format.hpp"
#include <stack>
#include <nlohmann/json.hpp>

/** @file
 *  Defines SSS::TR::Area.
 */

namespace SSS::Log::TR {
    /** Logging properties for SSS::TR::Area instances.*/
    struct Areas : public LogBase<Areas> {
        using LOG_STRUCT_BASICS(TR, Areas);
        /** Logs both constructor and destructor.*/
        bool life_state = false;
    };
}

SSS_TR_BEGIN;

INTERNAL_BEGIN;

struct Line;
class Buffer;
class BufferInfoVector;
class AreaPixels;

INTERNAL_END;

enum class Move;    // Pre-declaration
enum class Delete;  // Pre-declaration
class AreaCommand;

enum class PrintMode {
    Instant,
    Typewriter,
    // More later?
};

struct TextPart {
    TextPart() = default;
    TextPart(std::u32string const& s, Format const& f) : str(s), fmt(f) {};
    std::u32string str;
    Format fmt;
};

class TextParts : public std::vector<TextPart> {
public:
    bool move_cursor = false;
};

// Ignore warning about STL exports as they're private members
#pragma warning(push, 2)
#pragma warning(disable: 4251)
#pragma warning(disable: 4275)

/** The class handling most of the text rendering logic.
 *
 *  Use static functions to create() instances or retrieve
 *  them later with get().
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
class SSS_TR_API Area : public InstancedClass<Area> {
    friend SharedClass;
    friend AreaCommand;
protected:
    // Constructor
    Area();

public:
    /** Destructor, clears internal cache.
     *  Can only be indirectly called.
     *  @sa remove() and clearAll().
     */
    ~Area() noexcept;

    static CommandHistory history;
    
    void setWrapping(bool wrapping) noexcept;
    bool getWrapping() const noexcept;

    void setWrappingMinWidth(int min_w) noexcept;
    int getWrappingMinWidth() const noexcept;
    void setWrappingMaxWidth(int max_w) noexcept;
    int getWrappingMaxWidth() const noexcept;

    int getUsedWidth() const noexcept;

    using InstancedClass::create;
    /** Creates an Area instance which will be stored in the internal #Map.
     *  @param[in] width The area's width, in pixels. Must be above 0.
     *  @param[in] height The area's height, in pixels. Must be above 0.
     *  @return A const-ref to the created Area::Ptr.
     *  @throw std::runtime_error If \c width or \c height are <= 0.
     *  @sa get(), remove().
     */
    static Shared create(int width, int height);

    static Shared create(std::u32string const& str, Format fmt = Format());
    static Shared create(std::string const& str, Format fmt = Format());

    inline Format getFormat() const noexcept { return _format; };

    /** Modifies internal format for given ID.
     *  @param[in] src The source format to be copied.
     *  @sa Format.
     */
    void setFormat(Format const& src);

    static void setDefaultMargins(int marginV, int marginH) noexcept;
    static void getDefaultMargins(int& marginV, int& marginH) noexcept;

    void setMargins(int marginV, int marginH);
    void setMarginV(int marginV);
    void setMarginH(int marginH);
    inline int getMarginV() const noexcept { return _margin_v; };
    inline int getMarginH() const noexcept { return _margin_h; };

    void setClearColor(RGBA32 color);
    inline RGBA32 getClearColor() const noexcept { return _bg_color; };

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
    void parseStringU32(std::u32string const& str);
    /** \overload*/
    void parseString(std::string const& str);

    void setTextParts(std::vector<TextPart> const& text_parts, bool move_cursor = true);
    std::vector<TextPart> getTextParts() const;

    std::u32string getStringU32() const;
    std::string getString() const;

    std::u32string getUnparsedStringU32() const;
    std::string getUnparsedString() const;


    // TODO: get string keeping format
    //std::u32string getStringU32Formatted() const;
    //std::string getStringFormatted() const;

    /** Clears the internal string & pixels.
     *  Also resets scrolling, cursors.\n
     *  Keeps previous format modifications & dimensions.
     *  @sa update(), parseString(), pixelsGet();
     */
    void clear() noexcept;

    static void updateAll();
    static void notifyAll();
    static void cancelAll();

    /** Draws modifications when needed, and sets the return value of
     *  pixelsWereChanged() to \c true when changes are finished.
     *  @usage To be called as often as possible in your main loop.
     *  @sa clear(), pixelsGet().
     */
    void update();

    bool hasRunningThread() const noexcept;
    
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
     *  @sa update(), clear(), pixelsGetDimensions().
     */
    void const* pixelsGet() const;
    /** Retrieves dimensions of current pixels.
     *  Current pixels are the ones returned by pixelsGet().\n
     *  If you just called setDimensions(), this function won't retrieve
     *  new values until pixelsWereChanged() returns \c true.
     *  @param[out] width Will be filled with pixels width.
     *  @param[out] height Will be filled with pixels height.
     */
    void pixelsGetDimensions(int& width, int& height) const noexcept;

    void getDimensions(int& width, int& height) const noexcept;
    inline auto getDimensions() const noexcept { return std::make_tuple(_w, _h); };
    inline int getWidth() const noexcept { return _w; };
    inline int getHeight() const noexcept { return _h; };
    /** Sets new dimensions values for internal pixels.
     *  A call to update() is necessary for changes to take effect.
     *  @param[in] width Defines the pixels width. Must be above \c 0.
     *  @param[in] height Defines the pixels height. Must be above \c 0.
     *  @throw std::runtime_error If at least one of the given values
     *  
     */
    void setDimensions(int width, int height);
    inline void setWidth(int width) { setDimensions(width, _h); };
    inline void setHeight(int height) { setDimensions(_w, height); };
    /** Scrolls up with negative values, and down with positive values.
     *  Takes effect immediatley and sets pixelsWereChanged() to \c true.\n
     *  Trying to scroll too high or too low will have no effect.
     *  param[in] pixels The amount of pixels to scroll in either direction.
     */
    void scroll(int pixels) noexcept;

    void setFocusable(bool focusable);
    inline bool isFocusable() const noexcept { return _is_focusable; }
    void setFocus(bool state);
    bool isFocused() const noexcept;

    static void resetFocus();
    static Shared getFocused() noexcept;

    /** Places the editing cursor at given coordinates.
     *  The cursor is by default at the end of the text.
     *  @param[in] x The X coordinate to place the cursor to.
     *  @param[in] y The Y coordinate to place the cursor to.
     *  @sa cursorMove(), cursorAddText(), cursorDeleteText()
     */
    void cursorPlace(int x, int y);
    inline void lockSelection() noexcept { _lock_selection = true; };
    inline void unlockSelection() noexcept { _lock_selection = false; };
    void selectAll() noexcept;

    void formatSelection(nlohmann::json const& json);

private:
    size_t _move_cursor_line(_internal::Line const* line, int x);
    void _cursorMove(Move direction);
    std::optional<TextParts> _cursorAddText(std::u32string str) const;
    std::tuple<size_t, size_t> _cursorGetInfo() const;
    std::tuple<TextParts, TextParts> _splitText(size_t cursor, size_t count) const;
    std::optional<TextParts> _cursorDeleteText(Delete direction) const;
    std::optional<TextParts> _cursorGetText() const;

public:
    /** Moves the editing cursor in given direction.
     *  The cursor is by default at the end of the text.
     *  @param[in] direction The direction for the cursor to be moved.
     *  @sa Move, cursorPlace(), cursorAddText(), cursorDeleteText()
     */
    static void cursorMove(Move direction);
    /** Inserts text at the cursor's position.
     *  The cursor is by default at the end of the text.
     *  @param[in] str The UTF32 string to be added to existing text.
     *  @sa cursorPlace(), cursorMove(), cursorDeleteText()
     */
    static void cursorAddText(std::u32string str);
    /** \overload*/
    static void cursorAddText(std::string str);
    /** Inserts text at the cursor's position.
     *  The cursor is by default at the end of the text.
     *  @param[in] str The UTF32 string to be added to existing text.
     *  @sa cursorPlace(), cursorMove(), cursorDeleteText()
     */
    static void cursorAddChar(char32_t c);
    /** \overload*/
    static void cursorAddChar(char c);
    /** Deletes text at the cursor's position, in the given direction.
     *  The cursor is by default at the end of the text.
     *  @param[in] direction The deletion direction.
     *  @sa Delete, cursorPlace(), cursorMove(), cursorAddText()
     */
    static void cursorDeleteText(Delete direction);

    static std::u32string cursorGetText();

    void setPrintMode(PrintMode mode) noexcept;
    inline PrintMode getPrintMode() const noexcept { return _print_mode; };

    void setTypeWriterSpeed(int char_per_second);
    int getTypeWriterSpeed() const noexcept { return _tw_cps; };

private:

    bool _wrapping{ true };
    int _min_w{ 0 };
    int _max_w{ 0 };
    // Width of area
    int _w;
    // Height of area
    int _h;
    // Height of _pixels -> must NEVER be lower than _h
    int _pixels_h{ 0 };
    // Scrolling index, in pixels
    int _scrolling{ 0 };

    // Default vertical margin, in pixels
    static int _default_margin_v;
    // Default horizontal margin, in pixels
    static int _default_margin_h;
    // Vertical margin, in pixels
    int _margin_v{ _default_margin_v };
    // Horizontal margin, in pixels
    int _margin_h{ _default_margin_h };

    // True -> enables _drawIfNeeded()
    bool _draw{ true };
    // Print mode, default = instantaneous
    PrintMode _print_mode{ PrintMode::Instant };
    // TypeWriter -> characters per second.
    int _tw_cps{ 60 };
    // TypeWriter -> cursor advance
    float _tw_cursor{ 0.f };
    std::chrono::duration<float> _tw_sleep{ 0 };

    // Managed by update(), used in "pixelsXXX" functions
    bool _changes_pending{ false };
    // Last time the update() function was called
    std::chrono::steady_clock::time_point _last_update{ std::chrono::steady_clock::now() };
    // Last time an Effect::Vibrate was updated
    std::chrono::steady_clock::time_point _last_vibrate_update{};

    // Double-Buffer array
    using _PixelBuffers = std::array<std::unique_ptr<_internal::AreaPixels>, 2>;
    // Async processing buffers
    _PixelBuffers _pixels;
    // Const iterator used in pixelsGet(), managed by update()
    _PixelBuffers::const_iterator _current_pixels{ _pixels.cbegin() };
    // Iterator managed by _drawIfNeeded()
    _PixelBuffers::iterator _processing_pixels{ _pixels.begin() };

    RGBA32 _bg_color{ 0, 0, 0, 0 };
    // Base format to feed to internal buffers
    Format _format;
    // Buffer vector, one for each differing format
    std::vector<std::unique_ptr<_internal::Buffer>> _buffers;
    // Buffer informations
    std::unique_ptr<_internal::BufferInfoVector> _buffer_infos;
    // Total number of glyphs in all ACTIVE buffers
    size_t _glyph_count{ 0 };

    // Whether this Area is focusable
    bool _is_focusable{ false };
    // Cursor's glyph related position
    // Managed in "Cursor" functions, used in "cursorAddText" functions
    // Set to first be at the end of text by default.
    size_t _edit_cursor{ 0 };
    // Cursor physical position for moving lines
    int _edit_x{ -1 };
    // Timer determining if the edit cursor should be displayed
    std::chrono::nanoseconds _edit_timer{ 0 };
    // Whether to display the edit cursor
    bool _edit_display_cursor{ false };

    bool _lock_selection{ false };
    size_t _locked_cursor{ 0 };

    // Indexes of line breaks & charsizes
    std::vector<_internal::Line> _lines;

    /** Unique instance pointer, which are stored in a map.
     *  This is the only way to refer to Area instances.
     *  @sa create(), remove().
     */
    using Ptr = std::unique_ptr<Area>;

    static Weak _focused;

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

class AreaCommand : public CommandBase {
public:
    enum class Type {
        Addition,
        Deletion,
        Paste,
        Formatting
    };

private:
    Type const _type;
    Area::Weak const _area;
    size_t const _old_cursor = 0;
    size_t const _old_locked_cursor = 0;
    std::vector<TextPart> const _old_parts;
    std::vector<TextPart> _new_parts;
    bool const _move_cursor = false;

public:

    AreaCommand() = default;
    ~AreaCommand() = default;
    AreaCommand(Type type, Area::Shared area, TextParts parts)
        : _type(type),
          _area(area),
          _old_cursor(area->_edit_cursor),
          _old_locked_cursor(area->_locked_cursor),
          _old_parts(area->getTextParts()),
          _new_parts(parts),
          _move_cursor(parts.move_cursor) {};
    
    virtual void execute() override final {
        Area::Shared area = _area.lock();
        if (!area) return;
        area->_edit_cursor = _old_cursor;
        area->_locked_cursor = _old_locked_cursor;
        area->setTextParts(_new_parts, _move_cursor);
        if (area->isFocused()) {
            area->_draw = true;
            area->_edit_display_cursor = true;
            area->_edit_timer = std::chrono::nanoseconds(0);
        }
    }

    virtual void undo() override final {
        Area::Shared area = _area.lock();
        if (!area) return;
        area->setTextParts(_old_parts, false);
        area->_edit_cursor = _old_cursor;
        area->_locked_cursor = _old_locked_cursor;
        if (area->isFocused()) {
            area->_draw = true;
            area->_edit_display_cursor = true;
            area->_edit_timer = std::chrono::nanoseconds(0);
        }
    }

    bool merge(AreaCommand const& new_cmd) {
        if (_type != new_cmd._type || _area.lock() != new_cmd._area.lock())
            return false;
        if (_type == Type::Paste || _type == Type::Formatting)
            return false;
        _new_parts = new_cmd._new_parts;
        return true;
    }
};

#pragma warning(pop)

SSS_TR_END;


#endif // SSS_TR_AREA_HPP