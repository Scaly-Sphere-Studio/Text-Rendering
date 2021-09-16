#include "SSS/Text-Rendering/TextArea.hpp"

__SSS_TR_BEGIN

std::vector<TextArea::Weak> TextArea::_instances{};

    // --- Constructor, destructor & clear function ---

// Constructor, sets width & height.
// Throws an exception if width and/or height are <= 0.
TextArea::TextArea(int width, int height) try
    : _w(width), _h(height)
{
}
__CATCH_AND_RETHROW_METHOD_EXC

// Destructor, clears out buffer cache.
TextArea::~TextArea() noexcept
{
}

// Resets the object to newly constructed state
void TextArea::clear() noexcept
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
    // Reset lines
    _updateLines();
    // Reset typewriter
    _tw_cursor = 0;
    _tw_next_cursor = 0;
}

TextArea::Shared TextArea::create(int width, int height)
{
    Shared shared(new TextArea(width, height));
    _instances.push_back(shared);
    return shared;
}

void TextArea::resize(int width, int height) try
{
    _w = width;
    _h = height;
    _updateLines();
}
__CATCH_AND_RETHROW_METHOD_EXC

    // --- Basic functions ---

// Loads passed string in cache.
void TextArea::useBuffer(Buffer::Shared buffer) try
{
    // Update counts
    _buffers.push_back(buffer);
    _updateBufferInfos();
}
__CATCH_AND_RETHROW_METHOD_EXC

void TextArea::update()
{
    if (_processing_pixels->isPending()) {
        _current_pixels = _processing_pixels;
        _processing_pixels->setAsHandled();
        _changes_pending = true;
    }
    _drawIfNeeded();
}

void const* TextArea::getPixels() const try
{
    RGBA32::Pixels const& pixels = _current_pixels->getPixels();
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
        throw_exc(ERR_MSG::OUT_OF_BOUND);
    }
    return &pixels.at(index);
}
__CATCH_AND_RETHROW_METHOD_EXC

void TextArea::getDimensions(int& w, int& h) const noexcept
{
    _current_pixels->getDimensions(w, h);
}

    // --- Format functions ---

// Scrolls up (negative values) or down (positive values)
// Any excessive scrolling will be negated,
// hence, this function is safe.
void TextArea::scroll(int pixels) noexcept
{
    int tmp = _scrolling;
    _scrolling += pixels;
    _scrollingChanged();
    if (!_changes_pending) {
        _changes_pending = tmp != _scrolling;
    }
}

void TextArea::placeCursor(int x, int y) try
{
    if (_glyph_count == 0) {
        _edit_cursor = 0;
        return;
    }
    y += _scrolling;
    _edit_x = x;
    _edit_y = y;

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

void TextArea::moveCursor(Cursor position) try
{
    static auto const set_cursor = [&](_internal::Line::cit line)->size_t {
        size_t cursor = line->first_glyph;
        for (int x = 0; cursor <= line->last_glyph && cursor < _glyph_count; ++cursor) {
            x += _buffer_infos.getGlyph(cursor).pos.x_advance;
            if ((x >> 6) > _edit_x) {
                break;
            }
        }
        return cursor;
    };
    static auto const set_x = [&](size_t cursor, size_t cursor_max) {
        int x = 0;
        for (; cursor <= cursor_max && cursor < _glyph_count; ++cursor) {
            x += _buffer_infos.getGlyph(cursor).pos.x_advance;
        }
        return (x >> 6);
    };
    static auto const ctrl_jump = [&](int coeff) {
        _edit_cursor += coeff;
        while (_edit_cursor > 0 && _edit_cursor < _glyph_count) {
            _internal::GlyphInfo const& glyph(_buffer_infos.getGlyph(_edit_cursor));
            _internal::BufferInfo const& buffer(_buffer_infos.getBuffer(_edit_cursor));

            char32_t c = buffer.str[glyph.info.cluster];
            if (std::isalnum(c, buffer.locale))
                break;
            _edit_cursor += coeff;
        }
        while (_edit_cursor > 0 && _edit_cursor < _glyph_count) {
            _internal::GlyphInfo const& glyph(_buffer_infos.getGlyph(_edit_cursor));
            _internal::BufferInfo const& buffer(_buffer_infos.getBuffer(_edit_cursor));
            char32_t c = buffer.str[glyph.info.cluster];
            if (!std::isalnum(c, buffer.locale))
                break;
            _edit_cursor += coeff;
        }
        if (coeff == -1 && _edit_cursor != 0) {
            ++_edit_cursor;
        }
    };

    if (_edit_cursor > _glyph_count) {
        _edit_cursor = _glyph_count;
    }
    _internal::Line::cit line = _internal::Line::which(_lines, _edit_cursor);

    switch (position) {
    case Cursor::Up:
        if (line == _lines.cbegin()) break;
        --line;
        _edit_cursor = set_cursor(line);
        break;

    case Cursor::Down:
        if (line == _lines.cend() - 1) break;
        ++line;
        _edit_cursor = set_cursor(line);
        break;

    case Cursor::Left:
        if (_edit_cursor == 0) break;
        --_edit_cursor;
        line = _internal::Line::which(_lines, _edit_cursor);
        _edit_x = set_x(line->first_glyph, _edit_cursor);
        break;

    case Cursor::Right:
        if (_edit_cursor >= _glyph_count) break;
        ++_edit_cursor;
        line = _internal::Line::which(_lines, _edit_cursor);
        _edit_x = set_x(line->first_glyph, _edit_cursor);
        break;

    case Cursor::CtrlLeft:
        if (_edit_cursor == 0) break;
        ctrl_jump(-1);
        line = _internal::Line::which(_lines, _edit_cursor);
        _edit_x = set_x(line->first_glyph, _edit_cursor);
        break;

    case Cursor::CtrlRight:
        if (_edit_cursor >= _glyph_count) break;
        ctrl_jump(1);
        line = _internal::Line::which(_lines, _edit_cursor);
        _edit_x = set_x(line->first_glyph, _edit_cursor);
        break;

    case Cursor::Home:
        _edit_cursor = line->first_glyph;
        _edit_x = set_x(line->first_glyph, _edit_cursor);
        break;

    case Cursor::End:
        _edit_cursor = line->last_glyph + 1;
        _edit_x = set_x(line->first_glyph, _edit_cursor);
        break;
    }
}
__CATCH_AND_RETHROW_METHOD_EXC

void TextArea::insertText(std::u32string str) try
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
        Buffer::Shared const& buffer = _buffers.back();
        size_t const tmp = buffer->glyphCount();
        buffer->insertText(str, buffer->glyphCount());
        size = buffer->glyphCount() - tmp;
    }
    else {
        for (Buffer::Shared const& buffer : _buffers) {
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
    for (size_t i = 0; i < size; ++i) {
        moveCursor(Cursor::Right);
    }
}
__CATCH_AND_RETHROW_METHOD_EXC

void TextArea::insertText(std::string str)
{
    insertText(_internal::toU32str(str));
}

    // --- Typewriter functions ---

// Sets the writing mode (default: false):
// - true: Text is rendered char by char, see incrementCursor();
// - false: Text is fully rendered
void TextArea::TWset(bool activate) noexcept
{
    if (_typewriter != activate) {
        _tw_cursor = 0;
        _tw_next_cursor = 0;
        _draw = true;
        _typewriter = activate;
    }
}

// Increments the typewriter's cursor. Start point is 0.
// The first call will render the 1st character.
// Second call will render the both the 1st and 2nd character.
// Etc...
bool TextArea::TWprint() noexcept
{
    // Skip if the writing mode is set to default, or if all glyphs have been written
    if (!_typewriter || _tw_next_cursor >= _glyph_count) {
        return false;
    }

    ++_tw_next_cursor;
    _internal::Line::cit line = _internal::Line::which(_lines, _tw_next_cursor);
    if (line->first_glyph == _tw_next_cursor) {
        if (line->scrolling - _scrolling > static_cast<int>(_h)) {
            --_tw_next_cursor;
            return true;
        }
    }
    _draw = true;
    return false;
}

// Updates _scrolling
void TextArea::_scrollingChanged() noexcept
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
void TextArea::_updateLines() try
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

void TextArea::_updateBufferInfos() try
{
    _buffer_infos.update(_buffers);
    _glyph_count = _buffer_infos.glyphCount();
    _updateLines();
}
__CATCH_AND_RETHROW_METHOD_EXC

// Draws current area if needed & possible
void TextArea::_drawIfNeeded()
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

    // Determine draw parameters
    _internal::DrawParameters param(_prepareDraw());
    if (param.first_glyph > param.last_glyph || param.last_glyph > _glyph_count) {
        __LOG_METHOD_ERR(ERR_MSG::INVALID_ARGUMENT);
        return;
    }

    // Draw in thread
    _processing_pixels->draw(param, _w, _h, _pixels_h, _lines, _buffer_infos);
    _draw = false;
}

// Prepares drawing parameters, which will be used multiple times per draw
_internal::DrawParameters TextArea::_prepareDraw()
{
    _internal::DrawParameters param;
    // Determine range of glyphs to render
    // If _typewriter is set to false, this means all glyphs
    if (!_typewriter) {
        param.first_glyph = 0;
        param.last_glyph = _glyph_count;
    }
    // Else, only render needed glyphs
    else {
        param.first_glyph = _tw_cursor;
        param.last_glyph = _tw_next_cursor;
        // TypeWriter -> Update current character position
        for (size_t cursor = _tw_cursor + 1; cursor <= _tw_next_cursor
            && cursor < _glyph_count; ++cursor) {
            // It is important to note that we only update the current
            // character position after a whole word has been written.
            // This is to avoid drawing over previous glyphs.
            if (_buffer_infos.getGlyph(cursor).is_word_divider) {
                _tw_cursor = cursor;
            }
        }
    }

    // Determine first glyph's corresponding line
    param.line = _internal::Line::which(_lines, param.first_glyph);
    if (param.line != _lines.cend()) {
        // Lower pen.y accordingly
        // TODO: determine exact offsets needed, instead of using 5px
        param.pen = { 5 * 64, -5 * 64 };
        param.pen.y -= (param.line->scrolling - param.line->fullsize) << 6;

        // Shift the pen on the line accordingly
        for (size_t cursor = param.line->first_glyph; cursor < param.first_glyph; ++cursor) {
            hb_glyph_position_t const pos(_buffer_infos.getGlyph(cursor).pos);
            param.pen.x += pos.x_advance;
            param.pen.y += pos.y_advance;
        }
    }
    return param;
}

__SSS_TR_END