#include "SSS/Text-Rendering/Area.hpp"
#include "SSS/Text-Rendering/Globals.hpp"

SSS_TR_BEGIN;

Area::Map Area::_instances{};
int Area::_default_margin_h{ 10 };
int Area::_default_margin_v{ 10 };
bool Area::_focused_state{ false };
uint32_t Area::_focused_id{ 0 };

    // --- Constructor, destructor & clear function ---

// Constructor, sets width & height.
// Throws an exception if width and/or height are <= 0.
Area::Area(uint32_t id, int width, int height) try
    : _id(id), _w(width), _h(height)
{
    if (_w <= 0 || _h <= 0) {
        throw_exc("Width & Height should both be above 0.");
    }

    if (Log::TR::Areas::query(Log::TR::Areas::get().life_state)) {
        char buff[256];
        sprintf_s(buff, "Created an Area of dimensions (%dx%d).", _w, _h);
        LOG_TR_MSG(buff);
    }
}
CATCH_AND_RETHROW_METHOD_EXC;

// Destructor, clears out buffer cache.
Area::~Area() noexcept
{
    if (Log::TR::Areas::query(Log::TR::Areas::get().life_state)) {
        char buff[256];
        sprintf_s(buff, "Deleted an Area of dimensions (%dx%d).", _w, _h);
        LOG_TR_MSG(buff);
    }
}

Area::Ptr const& Area::create(uint32_t id, int width, int height) try
{
    Area::Ptr& area = _instances[id];
    area.reset(new Area(id, width, height));
    return area;
}
CATCH_AND_RETHROW_FUNC_EXC

Area::Ptr const& Area::create(int width, int height)
{
    uint32_t id = 0;
    // Increment ID until no similar value is found
    while (_instances.count(id) != 0) {
        ++id;
    }
    return create(id, width, height);
}

static void _eol(int& x, int& y, FT_Vector const& src, Format const& fmt)
{
    if (x < src.x)
        x = src.x;
    y += static_cast<int>(static_cast<float>(fmt.style.charsize)
        * fmt.style.line_spacing);
}

Area::Ptr const& Area::create(std::u32string const& str, Format fmt)
{

    int x = 0, y = 0;
    // Get values
    {
        _internal::Buffer::Ptr buffer = std::make_unique<_internal::Buffer>(fmt);
        buffer->changeString(str);
        FT_Vector pen({ 0, 0 });
        for (_internal::GlyphInfo const& glyph : buffer->getInfo().glyphs) {
            if (glyph.is_new_line) {
                _eol(x, y, pen, fmt);
                pen = { 0, 0 };
            }
            else {
                pen.x += glyph.pos.x_advance;
            }
        }
        _eol(x, y, pen, fmt);
        x = (x >> 6) + 1;
    }
    
    Ptr const& ptr = ::SSS::TR::Area::create(x + _default_margin_v * 2, y + _default_margin_h * 2);
    ptr->setFormat(fmt);
    ptr->parseString(str);
    return ptr;
}

Area::Ptr const& Area::create(std::string const& str, Format fmt)
{
    return create(strToStr32(str), fmt);
}

void Area::remove(uint32_t id) try
{
    if (_instances.count(id) != 0) {
        _instances.erase(id);
    }
}
CATCH_AND_RETHROW_FUNC_EXC

void Area::clearMap() noexcept
{
    _instances.clear();
}

Format Area::getFormat(uint32_t id)
{
    if (_formats.count(id) != 0) {
        return _formats.at(id);
    }
    return Format();
}

    // --- Basic functions ---

void Area::setFormat(Format const& format, uint32_t id) try
{
    if (_formats.count(id) == 0) {
        _formats.insert(std::make_pair(id, format));
    }
    else {
        _formats.at(id) = format;
    }
    parseString(_buffer_infos.getString());
}
CATCH_AND_RETHROW_METHOD_EXC;

void Area::setDefaultMargins(int marginV, int marginH) noexcept
{
    _default_margin_v = marginV;
    _default_margin_h = marginH;
}

void Area::getDefaultMargins(int& marginV, int& marginH) noexcept
{
    marginV = _default_margin_v;
    marginH = _default_margin_h;
}

void Area::setMargins(int marginH, int marginV)
{
    if (_margin_h != marginH || _margin_v != marginV) {
        _margin_h = marginH;
        _margin_v = marginV;
        _updateLines();
    }
}

void Area::setMarginH(int marginH)
{
    setMargins(marginH, _margin_v);
}

void Area::setMarginV(int marginV)
{
    setMargins(_margin_h, marginV);
}

void Area::setClearColor(RGBA32 color)
{
    _bg_color = color;
    _draw = true;
}

void Area::parseString(std::u32string const& str) try
{
    clear();
    Format opt = _formats[0];
    size_t i = 0;
    while (i != str.size()) {
        size_t const unit_separator = str.find(U'\u001F', i);
        if (unit_separator == std::string::npos) {
            // add buffer and load sub string
            _buffers.push_back(std::make_unique<_internal::Buffer>(opt));
            _buffers.back()->changeString(str.substr(i));
            break;
        }
        else {
            // find next US instance
            size_t const next_unit_separator = str.find(U'\u001F', unit_separator + 1);
            if (next_unit_separator == std::string::npos) {
                _buffers.push_back(std::make_unique<_internal::Buffer>(opt));
                _buffers.back()->changeString(str.substr(i));
                break;
            }
            // add buffer if needed
            size_t diff = unit_separator - i;
            if (diff > 0) {
                _buffers.push_back(std::make_unique<_internal::Buffer>(opt));
                _buffers.back()->changeString(str.substr(i, diff));
            }
            // parse id and change options
            diff = next_unit_separator - unit_separator - 1;
            opt = _formats[std::stoul(str32ToStr(
                str.substr(unit_separator + 1, diff)))];
            // skip to next sub strings
            i = next_unit_separator + 1;
        }
    }
    size_t tmp = _glyph_count;
    _updateBufferInfos();
    _edit_cursor += _glyph_count - tmp;
}
CATCH_AND_RETHROW_METHOD_EXC;

void Area::parseString(std::string const& str)
{
    parseString(strToStr32(str));
}

void Area::update()
{
    if (_processing_pixels->isPending()) {
        _current_pixels = _processing_pixels;
        _processing_pixels->setAsHandled();
        _changes_pending = true;
    }
    _drawIfNeeded();
    _last_update = std::chrono::steady_clock::now();
}

void const* Area::pixelsGet() const try
{
    RGBA32::Vector const& pixels = _current_pixels->getPixels();
    if (pixels.empty()) {
        return nullptr;
    }
    // Retrieve cropped dimensions of current pixels
    int w, h;
    _current_pixels->getDimensions(w, h);
    size_t size = static_cast<size_t>(w) * static_cast<size_t>(h);
    // Ensure current scrolling doesn't go past the pixels vector
    size_t const index = static_cast<size_t>(_scrolling) * static_cast<size_t>(w);
    if (index > pixels.size() - size) {
        throw_exc("Scrolling error");
    }
    return &pixels.at(index);
}
CATCH_AND_RETHROW_METHOD_EXC;

void Area::clear() noexcept
{
    // Reset scrolling
    _scrolling = 0;
    if (_pixels_h != _h) {
        _pixels_h = _h;
    }
    // Reset buffers
    _buffers.clear();
    _buffer_infos.clear();
    _glyph_count = 0;
    _edit_cursor = 0;
    // Reset lines
    _updateLines();
    // Reset typewriter
    _tw_cursor = 0.f;
    // Reset cursor timer
    _edit_timer = std::chrono::nanoseconds(0);
}

void Area::pixelsGetDimensions(int& w, int& h) const noexcept
{
    _current_pixels->getDimensions(w, h);
}

void Area::getDimensions(int& width, int& height) const noexcept
{
    width = _w;
    height = _h;
}

void Area::setDimensions(int width, int height) try
{
    _w = width;
    _h = height;
    _updateLines();
}
CATCH_AND_RETHROW_METHOD_EXC;

    // --- Format functions ---

// Scrolls up (negative values) or down (positive values)
// Any excessive scrolling will be negated,
// hence, this function is safe.
void Area::scroll(int pixels) noexcept
{
    int tmp = _scrolling;
    _scrolling += pixels;
    _scrollingChanged();
    if (!_changes_pending) {
        _changes_pending = tmp != _scrolling;
    }
}

void Area::resetFocus()
{
    _focused_state = false;
    if (_instances.count(_focused_id) != 0) {
        Ptr const& area = _instances.at(_focused_id);
        if (area) {
            area->setFocus(false);
        }
    }
}

Area::Ptr const& Area::getFocused() noexcept
{
    if (_focused_state && _instances.count(_focused_id) != 0) {
        return _instances.at(_focused_id);
    }
    static Ptr const n;
    return n;
}

void Area::setFocus(bool state)
{
    // Make this window focused
    if (state) {
        // Unfocus previous window if different
        if (_focused_id != _id && _instances.count(_focused_id) != 0) {
            Ptr const& area = _instances.at(_focused_id);
            if (area) {
                area->setFocus(false);
            }
        }
        _focused_id = _id;
        _focused_state = true;
        _edit_display_cursor = true;
        _edit_timer = std::chrono::nanoseconds(0);
        _draw = true;
    }
    // Unfocus this window
    else if (_focused_id == _id) {
        _focused_state = false;
        _edit_display_cursor = false;
        _draw = true;
    }
}

bool Area::isFocused() const noexcept
{
    return _focused_state && _focused_id == _id;
}

void Area::cursorPlace(int x, int y) try
{
    setFocus(true);
    if (_glyph_count == 0) {
        return;
    }
    y += _scrolling;

    FT_Vector pen{ _margin_v << 6, _margin_h };
    _internal::Line::cit line = _lines.cbegin();
    for (; line != _lines.cend(); ++line) {
        pen.y += line->fullsize;
        if (pen.y > y) break;
    }
    if (line == _lines.cend()) {
        --line;
    }
    pen.x += line->x_offset() << 6;

    for (size_t i = line->first_glyph; i < line->last_glyph; ++i) {
        _internal::GlyphInfo const& glyph(_buffer_infos.getGlyph(i));
        pen.x += glyph.pos.x_advance;
        if ((pen.x >> 6) > x) {
            _edit_cursor = i;
            return;
        }
    }
    _edit_cursor = line->last_glyph + 1;
}
CATCH_AND_RETHROW_METHOD_EXC;

static size_t _move_cursor_line(_internal::BufferInfoVector const& buffer_infos,
    _internal::Line::cit line, int x)
{
    size_t cursor = line->first_glyph;
    size_t const glyph_count = buffer_infos.glyphCount();
    for (int new_x = line->x_offset() << 6;
        cursor < line->last_glyph && cursor < glyph_count;
        ++cursor)
    {
        new_x += buffer_infos.getGlyph(cursor).pos.x_advance;
        if ((new_x >> 6) > x) {
            break;
        }
    }
    return cursor;
}

static size_t _ctrl_jump(_internal::BufferInfoVector const& buffer_infos,
    size_t cursor, int coeff)
{
    size_t const glyph_count = buffer_infos.glyphCount();
    bool flag = true;
    if (coeff == -1 || cursor == 0) {
        cursor += coeff;
    }
    while (cursor > 0 && cursor < glyph_count) {
        _internal::GlyphInfo const& glyph(buffer_infos.getGlyph(cursor));
        _internal::BufferInfo const& buffer(buffer_infos.getBuffer(cursor));

        char32_t const c = buffer.str[glyph.info.cluster];
        if (std::isalnum(c, buffer.locale) == flag) {
            if (flag)
                flag = false;
            else
                break;
        }
        cursor += coeff;
        if (glyph.is_new_line)
            break;
    }
    if (coeff == -1 && cursor != 0) {
        cursor -= coeff;
    }
    return cursor;
}

void Area::_cursorMove(Move direction) try
{
    _internal::Line::cit line = _internal::Line::which(_lines, _edit_cursor);
    int x, y;
    _getCursorPhysicalPos(x, y);

    switch (direction) {

    case Move::Right:
        if (_edit_cursor >= _glyph_count) break;
        ++_edit_cursor;
        break;

    case Move::Left:
        if (_edit_cursor == 0) break;
        --_edit_cursor;
        break;

    case Move::Down:
        if (line == _lines.cend() - 1) break;
        ++line;
        _edit_cursor = _move_cursor_line(_buffer_infos, line, x);
        break;

    case Move::Up:
        if (line == _lines.cbegin()) break;
        --line;
        _edit_cursor = _move_cursor_line(_buffer_infos, line, x);
        break;

    case Move::CtrlRight:
        if (_edit_cursor >= _glyph_count) break;
        _edit_cursor = _ctrl_jump(_buffer_infos, _edit_cursor, 1);
        break;

    case Move::CtrlLeft:
        if (_edit_cursor == 0) break;
        _edit_cursor = _ctrl_jump(_buffer_infos, _edit_cursor, -1);
        break;

    case Move::Start:
        _edit_cursor = line->first_glyph;
        break;

    case Move::End:
        _edit_cursor = line->last_glyph;
        break;

    }
    _draw = true;
    _edit_display_cursor = true;
    _edit_timer = std::chrono::nanoseconds(0);
}
CATCH_AND_RETHROW_METHOD_EXC;

void Area::_cursorAddText(std::u32string str) try
{
    if (str.empty()) {
        LOG_OBJ_METHOD_WRN("Empty string.");
        return;
    }
    if (_buffers.empty()) {
        throw_exc("No buffer was given beforehand.");
    }
    size_t cursor = _edit_cursor;
    size_t size = 0;
    if (cursor >= _glyph_count) {
        _internal::Buffer::Ptr const& buffer = _buffers.back();
        size_t const tmp = buffer->glyphCount();
        buffer->insertText(str, buffer->glyphCount());
        size = buffer->glyphCount() - tmp;
    }
    else {
        for (_internal::Buffer::Ptr const& buffer : _buffers) {
            if (buffer->glyphCount() >= cursor) {
                size_t const tmp = buffer->glyphCount();
                buffer->insertText(str, cursor);
                size = buffer->glyphCount() - tmp;
                break;
            }
            cursor -= buffer->glyphCount();
        }
    }
    // Update lines as they need to be updated before moving cursor
    _updateBufferInfos();
    // Move cursors
    if (static_cast<size_t>(_tw_cursor) < _edit_cursor) {
        _tw_cursor += static_cast<float>(size);
    }
    _edit_cursor += size;
    _edit_display_cursor = true;
    _edit_timer = std::chrono::nanoseconds(0);
}
CATCH_AND_RETHROW_METHOD_EXC;

void Area::_cursorDeleteText(Delete direction) try
{
    size_t cursor = _edit_cursor;
    size_t count = 0;
    if (cursor > _glyph_count) {
        cursor = _glyph_count;
    }

    size_t tmp;
    switch (direction) {
    case Delete::Right:
        if (cursor >= _glyph_count) break;
        count = 1;
        break;

    case Delete::Left:
        if (cursor == 0) break;
        count = 1;
        --cursor;
        break;

    case Delete::CtrlRight:
        if (cursor >= _glyph_count) break;
        tmp = _ctrl_jump(_buffer_infos, cursor, 1);
        count = tmp - cursor;
        break;

    case Delete::CtrlLeft:
        if (cursor == 0) break;
        tmp = _ctrl_jump(_buffer_infos, cursor, -1);
        count = cursor - tmp;
        cursor = tmp;
        break;

    }

    if (count == 0)
        return;

    size_t size = 0;
    for (_internal::Buffer::Ptr const& buffer : _buffers) {
        if (buffer->glyphCount() > cursor) {
            size_t const tmp = buffer->glyphCount();
            buffer->deleteText(cursor, count);
            size = tmp - buffer->glyphCount();
            break;
        }
        cursor -= buffer->glyphCount();
    }
    _updateBufferInfos();
    if (direction == Delete::Left || direction == Delete::CtrlLeft) {
        // Move cursor
        _edit_cursor -= size;
    }
    _edit_display_cursor = true;
    _edit_timer = std::chrono::nanoseconds(0);
}
CATCH_AND_RETHROW_METHOD_EXC;

void Area::cursorMove(Move direction)
{
    Ptr const& area = getFocused();
    if (area) {
        area->_cursorMove(direction);
    }
}

void Area::cursorAddText(std::u32string str)
{
    Ptr const& area = getFocused();
    if (area) {
        area->_cursorAddText(str);
    }
}

void Area::cursorAddText(std::string str)
{
    cursorAddText(strToStr32(str));
}

void Area::cursorDeleteText(Delete direction)
{
    Ptr const& area = getFocused();
    if (area) {
        area->_cursorDeleteText(direction);
    }
}

void Area::setPrintMode(PrintMode mode) noexcept
{
    if (_print_mode == mode) {
        return;
    }
    _tw_cursor = 0.f;
    _print_mode = mode;
    _draw = true;
}

void Area::setTypeWriterSpeed(int char_per_second)
{
    _tw_cps = char_per_second;
}

void Area::_getCursorPhysicalPos(int& x, int& y) const noexcept
{
    _internal::Line::cit line(_internal::Line::which(_lines, _edit_cursor));

    y = _margin_h + _lines.cbegin()->charsize * 4 / 3;
    for (_internal::Line::cit it = _lines.cbegin() + 1; it <= line; ++it) {
        y += it->fullsize;
    }

    x = (_margin_v  + line->x_offset()) << 6;
    for (size_t n = line->first_glyph; n < _edit_cursor && n < _glyph_count; ++n) {
        x += _buffer_infos.getGlyph(n).pos.x_advance;
    }
    x >>= 6;
}

// Ensures _scrolling has a valid value
void Area::_scrollingChanged() noexcept
{
    if (_scrolling <= 0) {
        _scrolling = 0;
    }
    int const max_scrolling = static_cast<int>(_pixels_h - _h);
    if (_scrolling >= max_scrolling) {
        _scrolling = max_scrolling;
    }
}

// Updates _lines
void Area::_updateLines() try
{
    if (_w <= 0 || _h <= 0) {
        throw_exc("width and/or height <= 0");
    }
    // Reset _lines
    _lines.clear();
    _lines.emplace_back();
    _internal::Line::it line = _lines.begin();
    if (!_buffer_infos.empty()) {
        line->alignment = _buffer_infos.front().style.aligmnent;
    }
    size_t cursor = 0;

    // Place pen at (0, 0), we don't take scrolling into account
    size_t last_divider(0);
    int last_divider_x{ 0 };
    FT_Vector pen({ _margin_v << 6, _margin_h << 6 });
    while (cursor < _glyph_count) {
        // Retrieve glyph infos
        _internal::GlyphInfo const& glyph = _buffer_infos.getGlyph(cursor);
        _internal::BufferInfo const& buffer = _buffer_infos.getBuffer(cursor);

        // Update max_size
        int const charsize = buffer.style.charsize;
        if (line->charsize < charsize) {
            line->charsize = charsize;
        }
        int const fullsize = static_cast<int>(static_cast<float>(charsize) * buffer.style.line_spacing);
        if (line->fullsize < fullsize) {
            line->fullsize = fullsize;
        }
        // If glyph is a word divider, mark next character as possible line break
        if (glyph.is_word_divider) {
            last_divider = cursor;
            last_divider_x = pen.x;
        }

        // Update pen position
        if (!glyph.is_new_line) {
            pen.x += glyph.pos.x_advance;
            pen.y += glyph.pos.y_advance;
        }
        // If the pen is now out of bound, we should line break
        if ((pen.x >> 6) < _margin_v || (pen.x >> 6) >= (_w - _margin_v)|| glyph.is_new_line) {
            // If no word divider was found, hard break the line
            if (last_divider == 0) {
                --cursor;
                line->unused_width = _w - _margin_v - ((pen.x - glyph.pos.x_advance) >> 6);
            }
            else {
                cursor = last_divider;
                line->unused_width = _w - _margin_v - (last_divider_x >> 6);
            }
            if (glyph.is_new_line)
                line->unused_width = _w - _margin_v - (pen.x >> 6);
            line->last_glyph = cursor;
            line->scrolling += line->fullsize;
            last_divider = 0;
            last_divider_x = 0;
            // Add line if needed
            _lines.emplace_back();
            line = _lines.end() - 1;
            line->first_glyph = cursor + 1;
            line->scrolling = (line - 1)->scrolling;
            line->alignment = buffer.style.aligmnent;
            // Reset pen
            pen = { _margin_v << 6, _margin_h << 6 };
        }
        // Only increment cursor if not a line break
        ++cursor;
    }

    line->last_glyph = cursor;
    line->scrolling += line->fullsize;
    line->unused_width = (_w - _margin_v - (pen.x >> 6));
    
    // Update size & scrolling
    size_t const size_before = _pixels_h;
    _pixels_h = line->scrolling;
    if (_pixels_h < _h) {
        _pixels_h = _h;
    }
    if (_pixels_h != size_before) {
        float const size_diff = static_cast<float>(_pixels_h - _h) / (size_before - _h);
        _scrolling = (size_t)std::round(static_cast<float>(_scrolling) * size_diff);
        _scrollingChanged();
    }
    _draw = true;
}
CATCH_AND_RETHROW_METHOD_EXC;

// Updates _buffer_infos and _glyph_count, then calls _updateLines();
void Area::_updateBufferInfos() try
{
    _buffer_infos.update(_buffers);
    _glyph_count = _buffer_infos.glyphCount();
    _updateLines();
}
CATCH_AND_RETHROW_METHOD_EXC;

// Draws current area if _draw is set to true
void Area::_drawIfNeeded()
{
    auto const now = std::chrono::steady_clock::now();
    auto const diff = now - _last_update;

    // Determine if TypeWriter is needed
    if (_print_mode == PrintMode::Typewriter && static_cast<size_t>(_tw_cursor) < _glyph_count) {
        auto const ns_per_char = std::chrono::duration<float>(1) / _tw_cps;
        _tw_cursor += diff / ns_per_char;
        if (static_cast<size_t>(_tw_cursor) > _glyph_count) {
            _tw_cursor = static_cast<float>(_glyph_count);
        }
        _draw = true;
    }
    // Determine if cursor needs to be drawn
    if (isFocused()) {
        _edit_timer += diff;
        using namespace std::chrono_literals;
        if (_edit_timer >= 500ms) {
            _edit_timer %= 500ms;
            _edit_display_cursor = !_edit_display_cursor;
            _draw = true;
        }
    }
    // Determine if a function needs to be edited
    for (auto const& buffer : _buffer_infos) {
        if (buffer.color.text.func == ColorFunc::rainbow
            || (buffer.style.has_outline && buffer.color.outline.func == ColorFunc::rainbow)
            || (buffer.style.has_shadow && buffer.color.shadow.func == ColorFunc::rainbow))
        {
            _draw = true;
            break;
        }
    }
    // Skip if drawing is not needed
    if (!_draw) {
        return;
    }
    // Skip if a draw call is already running
    if (_processing_pixels->isRunning()) {
        return;
    }
    // Update processing pixels if needed
    if (_processing_pixels == _current_pixels) {
        ++_processing_pixels;
        if (_processing_pixels == _pixels.end()) {
            _processing_pixels = _pixels.begin();
        }
    }

    // Copy internal data
    _internal::AreaData data;
    data.w = _w;
    data.h = _h;
    data.pixels_h = _pixels_h;
    data.margin_v = _margin_v;
    data.margin_h = _margin_h;
    data.draw_cursor = _edit_display_cursor;
    _getCursorPhysicalPos(data.cursor_x, data.cursor_y);
    data.cursor_h = _internal::Line::which(_lines, _edit_cursor)->fullsize;
    data.last_glyph = _print_mode == PrintMode::Typewriter ? static_cast<size_t>(_tw_cursor) : _glyph_count;
    data.buffer_infos = _buffer_infos;
    data.lines = _lines;
    data.bg_color = _bg_color;
    // Async draw
    _processing_pixels->run(data);
    _draw = false;
}

SSS_TR_END;