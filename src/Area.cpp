#include "SSS/Text-Rendering/Area.hpp"
#include "SSS/Text-Rendering/Lib.hpp"

__SSS_TR_BEGIN;

Area::Map Area::_instances{};

    // --- Constructor, destructor & clear function ---

// Constructor, sets width & height.
// Throws an exception if width and/or height are <= 0.
Area::Area(int width, int height) try
    : _w(width), _h(height)
{
    if (_w <= 0 || _h <= 0) {
        throw_exc("Width & Height should both be above 0.");
    }
}
__CATCH_AND_RETHROW_METHOD_EXC

// Destructor, clears out buffer cache.
Area::~Area() noexcept
{
}

void Area::create(uint32_t id, int width, int height) try
{
    _instances.try_emplace(id);
    _instances.at(id).reset(new Area(width, height));
}
__CATCH_AND_RETHROW_FUNC_EXC

void Area::remove(uint32_t id) try
{
    if (_instances.count(id) != 0) {
        _instances.erase(id);
    }
}
__CATCH_AND_RETHROW_FUNC_EXC

void Area::clearMap() noexcept
{
    _instances.clear();
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
}
__CATCH_AND_RETHROW_METHOD_EXC

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
__CATCH_AND_RETHROW_METHOD_EXC

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
__CATCH_AND_RETHROW_METHOD_EXC

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
    _tw_cursor = 0;
}

void Area::getDimensions(int& w, int& h) const noexcept
{
    _current_pixels->getDimensions(w, h);
}

void Area::setDimensions(int width, int height) try
{
    _w = width;
    _h = height;
    _updateLines();
}
__CATCH_AND_RETHROW_METHOD_EXC

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

void Area::cursorPlace(int x, int y) try
{
    if (_glyph_count == 0) {
        return;
    }
    y += _scrolling;

    FT_Vector pen{ 0, 0 };
    _internal::Line::cit line = _lines.cbegin();
    for (; line != _lines.cend(); ++line) {
        pen.y += line->fullsize;
        if (pen.y > y) break;
    }
    if (line == _lines.cend()) {
        --line;
    }

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
__CATCH_AND_RETHROW_METHOD_EXC

static size_t _move_cursor_line(_internal::BufferInfoVector const& buffer_infos,
    _internal::Line::cit line, int x)
{
    size_t cursor = line->first_glyph;
    size_t const glyph_count = buffer_infos.glyphCount();
    for (int new_x = 0; cursor <= line->last_glyph && cursor < glyph_count; ++cursor) {
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
    cursor += coeff;
    while (cursor > 0 && cursor < glyph_count) {
        _internal::GlyphInfo const& glyph(buffer_infos.getGlyph(cursor));
        _internal::BufferInfo const& buffer(buffer_infos.getBuffer(cursor));

        char32_t c = buffer.str[glyph.info.cluster];
        if (std::isalnum(c, buffer.locale))
            break;
        cursor += coeff;
    }
    while (cursor > 0 && cursor < glyph_count) {
        _internal::GlyphInfo const& glyph(buffer_infos.getGlyph(cursor));
        _internal::BufferInfo const& buffer(buffer_infos.getBuffer(cursor));
        char32_t c = buffer.str[glyph.info.cluster];
        if (!std::isalnum(c, buffer.locale))
            break;
        cursor += coeff;
    }
    if (coeff == -1 && cursor != 0) {
        ++cursor;
    }
    return cursor;
}

void Area::cursorMove(Move direction) try
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
        // TODO: fix this working partially on Areas with multiple lines
        _edit_cursor = line->last_glyph + 1;
    }
    _draw = true;
}
__CATCH_AND_RETHROW_METHOD_EXC

void Area::cursorAddText(std::u32string str) try
{
    if (str.empty()) {
        __LOG_OBJ_METHOD_WRN("Empty string.");
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
    // Move cursor
    _edit_cursor += size;
}
__CATCH_AND_RETHROW_METHOD_EXC

void Area::cursorAddText(std::string str)
{
    cursorAddText(strToStr32(str));
}

void Area::cursorDeleteText(Delete direction) try
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
}
__CATCH_AND_RETHROW_METHOD_EXC

    // --- Typewriter functions ---

// Sets the writing mode (default: false):
// - true: Text is rendered char by char, see incrementCursor();
// - false: Text is fully rendered
void Area::twSet(bool activate) noexcept
{
    if (_typewriter != activate) {
        _tw_cursor = 0;
        _draw = true;
        _typewriter = activate;
    }
}

// Increments the typewriter's cursor. Start point is 0.
// The first call will render the 1st character.
// Second call will render the both the 1st and 2nd character.
// Etc...
bool Area::twPrint() noexcept
{
    // Skip if the writing mode is set to default, or if all glyphs have been written
    if (!_typewriter) {
        return false;
    }

    // TODO: auto scroll
    ++_tw_cursor;
    _internal::Line::cit line = _internal::Line::which(_lines, _tw_cursor);
    if (line->first_glyph == _tw_cursor) {
        if (line->scrolling - _scrolling > static_cast<int>(_h)) {
            --_tw_cursor;
            return true;
        }
    }
    _draw = true;
    return false;
}

void Area::_getCursorPhysicalPos(int& x, int& y) const noexcept
{
    _internal::Line::cit line(_internal::Line::which(_lines, _edit_cursor));

    y = 5 + _lines.cbegin()->charsize; // TODO: related to FT_Pen
    for (_internal::Line::cit it = _lines.cbegin() + 1; it <= line; ++it) {
        y += it->fullsize;
    }

    x = 5 << 6; // TODO: related to FT_Pen
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
    size_t cursor = 0;

    // Place pen at (0, 0), we don't take scrolling into account
    size_t last_divider(0);
    FT_Vector pen({ 0, 0 });
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
        }

        // Update pen position
        pen.x += glyph.pos.x_advance;
        pen.y += glyph.pos.y_advance;
        // If the pen is now out of bound, we should line break
        if (pen.x < 0 || (pen.x >> 6) >= _w) {
            // If no word divider was found, hard break the line
            if (last_divider == 0) {
                --cursor;
            }
            else {
                cursor = last_divider;
            }
            line->last_glyph = cursor;
            line->scrolling += line->fullsize;
            last_divider = 0;
            // Add line if needed
            _lines.push_back({ 0, 0, 0, 0, 0 });
            line = _lines.end() - 1;
            line->first_glyph = cursor + 1;
            line->scrolling = (line - 1)->scrolling;
            // Reset pen
            pen = { 0, 0 };
        }
        // Only increment cursor if not a line break
        ++cursor;
    }

    line->last_glyph = cursor;
    line->scrolling += line->fullsize;
    
    _tw_cursor = 0;

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
__CATCH_AND_RETHROW_METHOD_EXC

// Updates _buffer_infos and _glyph_count, then calls _updateLines();
void Area::_updateBufferInfos() try
{
    _buffer_infos.update(_buffers);
    _glyph_count = _buffer_infos.glyphCount();
    _updateLines();
}
__CATCH_AND_RETHROW_METHOD_EXC

// Draws current area if _draw is set to true
void Area::_drawIfNeeded()
{
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
    _getCursorPhysicalPos(data.cursor_x, data.cursor_y);
    data.cursor_h = _buffer_infos.getBuffer(_edit_cursor).style.charsize;
    data.last_glyph = _typewriter ? _tw_cursor : _glyph_count;
    data.buffer_infos = _buffer_infos;
    data.lines = _lines;
    // Async draw
    _processing_pixels->run(data);
    _draw = false;
}

__SSS_TR_END;