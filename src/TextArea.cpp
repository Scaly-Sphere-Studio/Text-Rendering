#include "SSS/Text-Rendering/TextArea.hpp"

__SSS_TR_BEGIN

std::vector<TextArea::Weak> TextArea::_instances{};

    // --- Constructor, destructor & clear function ---

// Constructor, sets width & height.
// Throws an exception if width and/or height are <= 0.
TextArea::TextArea(int width, int height) try
    : _w(width), _h(height)
{
    if (_w <= 0 || _h <= 0) {
        throw_exc(ERR_MSG::INVALID_ARGUMENT);
    }
    __LOG_CONSTRUCTOR
}
__CATCH_AND_RETHROW_METHOD_EXC

// Destructor, clears out buffer cache.
TextArea::~TextArea() noexcept
{
    __LOG_DESTRUCTOR
}

// Resets the object to newly constructed state
void TextArea::clear() noexcept
{
    // Reset scrolling
    _scrolling = 0;
    if (_pixels_h != _h) {
        _pixels_h = _h;
        _resize = true;
    }
    // Reset buffers
    _buffers.clear();
    _glyph_count = 0;
    // Reset lines
    _lines.clear();
    _update_lines = true;
    // Reset pixels
    _clear = true;
    _draw = false;
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

    // --- Basic functions ---

// Loads passed string in cache.
void TextArea::useBuffer(Buffer::Shared buffer) try
{
    // Update counts
    _buffers.push_back(buffer);
    _glyph_count += buffer->_size();
    _update_lines = true;
}
__CATCH_AND_RETHROW_METHOD_EXC

// Renders text to a 2D pixel array in the RGBA32 format.
// The passed array should have the same width and height
// as the TextArea object.
void TextArea::renderTo(void* ptr)
{
    if (ptr == nullptr) {
        __LOG_METHOD_ERR(ERR_MSG::INVALID_ARGUMENT)
        return;
    }
    _drawIfNeeded();
    memcpy(ptr, &_pixels.at(_scrolling * _w), (size_t)(4 * _w * _h));
}

void const* TextArea::getPixels()
{
    _drawIfNeeded();
    return &_pixels.at(_scrolling * _w);
}

    // --- Format functions ---

// Scrolls up (negative values) or down (positive values)
// Any excessive scrolling will be negated,
// hence, this function is safe.
bool TextArea::scroll(int pixels) noexcept
{
    int tmp = _scrolling;
    _scrolling += pixels;
    _scrollingChanged();
    return tmp != _scrolling;
}

    // --- Typewriter functions ---

// Sets the writing mode (default: false):
// - true: Text is rendered char by char, see incrementCursor();
// - false: Text is fully rendered
void TextArea::setTypeWriter(bool activate) noexcept
{
    if (_typewriter != activate) {
        _tw_cursor = 0;
        _tw_next_cursor = 0;
        _clear = true;
        _draw = true;
        _typewriter = activate;
    }
}

// Increments the typewriter's cursor. Start point is 0.
// The first call will render the 1st character.
// Second call will render the both the 1st and 2nd character.
// Etc...
bool TextArea::incrementCursor() noexcept
{
    // Skip if the writing mode is set to default, or if all glyphs have been written
    if (!_typewriter || _tw_next_cursor >= _glyph_count) {
        return false;
    }

    ++_tw_next_cursor;
    _internal::Line::cit line = _whichLine(_tw_next_cursor);
    if (line->first_glyph == _tw_next_cursor) {
        if (line->scrolling - _scrolling > static_cast<int>(_h)) {
            --_tw_next_cursor;
            return true;
        }
    }
    _draw = true;
    return false;
}

// Update the text format. To be called from Buffers upon them changing
void TextArea::_updateFormat() try
{
    // Update global format
    float pre_size = static_cast<float>(_pixels_h - _h);
    _updateLines();
    float const size_diff = static_cast<float>(_pixels_h - _h) / pre_size;
    _scrolling = (size_t)std::round(static_cast<float>(_scrolling) * size_diff);
    _scrollingChanged();
    _clear = true;
    _draw = true;

    _update_format = false;
}
__CATCH_AND_RETHROW_METHOD_EXC

// Calls the at(); function from corresponding Buffer
_internal::GlyphInfo TextArea::_at(size_t cursor) const try
{
    for (size_t i(0); i < _buffers.size(); ++i) {
        Buffer::Shared const& buffer = _buffers.at(i);
        if (buffer->_size() > cursor) {
            return buffer->_at(cursor);
        }
        cursor -= buffer->_size();
    }
    throw_exc(ERR_MSG::OUT_OF_BOUND);
}
__CATCH_AND_RETHROW_METHOD_EXC

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
void TextArea::_updateLines()
{
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
        _internal::GlyphInfo const glyph(_at(cursor));
        // Update max_size
        int const charsize = glyph.style.charsize;
        if (line->charsize < charsize) {
            line->charsize = charsize;
        }
        size_t fullsize = (int)((float)charsize * glyph.style.line_spacing);
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
        if (pen.x < 0 || (pen.x >> 6) >= static_cast<int>(_w)) {
            // If no word divider was found, hard break the line
            if (last_divider == 0) {
                --cursor;
            }
            else {
                cursor = last_divider;
            }
            line->last_glyph = cursor;
            line->scrolling += static_cast<int>(line->fullsize);
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
    line->scrolling += static_cast<int>(line->fullsize);

    _pixels_h = line->scrolling;
    if (_pixels_h < _h) {
        _pixels_h = _h;
    }
    _resize = true;

    _tw_cursor = 0;
    _clear = true;
    _draw = true;

    _update_lines = false;
}

// Returns corresponding _internal::Line iterator, or cend()
_internal::Line::cit TextArea::_whichLine(size_t cursor) const noexcept
{
    _internal::Line::cit line = _lines.cbegin();
    while (line != _lines.cend() - 1) {
        if (line->last_glyph >= cursor) {
            break;
        }
        ++line;
    }
    return line;
}

    // --- Private functions -> Draw functions ---

// Draws current area if _draw is set to true
void TextArea::_drawIfNeeded()
{
    // Update format if needed
    if (_update_format) {
        _updateFormat();
    }
    // Update lines if needed
    if (_update_lines) {
        _updateLines();
    }
    // Resize pixels array if needed
    if (_resize) {
        _pixels.resize(_w * _pixels_h);
        _resize = false;
    }
    // Clear out pixels array if needed
    if (_clear) {
        std::fill(_pixels.begin(), _pixels.end(), 0);
        _clear = false;
    }
    // Skip if already drawn
    if (!_draw) {
        return;
    }

    // Determine draw parameters
    _internal::DrawParameters param(_prepareDraw());

    if (param.first_glyph > param.last_glyph || param.last_glyph > _glyph_count) {
        __LOG_METHOD_ERR(ERR_MSG::INVALID_ARGUMENT);
        return;
    }

    // Draw Outline shadows
    param.type.is_shadow = true;
    param.type.is_outline = true;
    _drawGlyphs(param);

    // Draw Text shadows
    param.type.is_outline = false;
    _drawGlyphs(param);

    // Draw Outlines
    param.type.is_shadow = false;
    param.type.is_outline = true;
    _drawGlyphs(param);

    // Draw Text
    param.type.is_outline = false;
    _drawGlyphs(param);

    // Update draw statement
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
            if (_at(cursor).is_word_divider) {
                _tw_cursor = cursor;
            }
        }
    }

    // Determine first glyph's corresponding line
    param.line = _whichLine(param.first_glyph);
    if (param.line != _lines.cend()) {
        // Lower pen.y accordingly
        // TODO: determine exact offsets needed, instead of using 5px
        param.pen = { 5 * 64, -5 * 64 };
        param.pen.y -= (param.line->scrolling
            - static_cast<int>(param.line->fullsize)) << 6;

        // Shift the pen on the line accordingly
        for (size_t cursor = param.line->first_glyph; cursor < param.first_glyph; ++cursor) {
            hb_glyph_position_t const pos(_at(cursor).pos);
            param.pen.x += pos.x_advance;
            param.pen.y += pos.y_advance;
        }
    }
    return param;
}

// Draws shadows, outlines, or plain glyphs
void TextArea::_drawGlyphs(_internal::DrawParameters param)
{
    // Draw the glyphs
    for (size_t cursor = param.first_glyph; cursor < param.last_glyph; ++cursor) {
        _internal::GlyphInfo const glyph(_at(cursor));
        _drawGlyph(param, glyph);
        // Handle line breaks. Return true if pen goes out of bound
        if (cursor == param.line->last_glyph && param.line != _lines.end() - 1) {
            param.pen.x = 5 << 6;
            param.pen.y -= static_cast<int>(param.line->fullsize) << 6;
            ++param.line;
        }
        // Increment pen's coordinates
        else {
            param.pen.x += glyph.pos.x_advance;
            param.pen.y += glyph.pos.y_advance;
        }
    }
}

// Loads (if needed) and renders the corresponding glyph to the
// pixels pointer at the given pen's coordinates.
void TextArea::_drawGlyph(_internal::DrawParameters param, _internal::GlyphInfo const& glyph_info)
{
    // Skip if the glyph alpha is zero, or if a outline is asked but not available
    if (glyph_info.color.alpha == 0
        || (param.type.is_outline && (!glyph_info.style.has_outline || glyph_info.style.outline_size <= 0))
        || (param.type.is_shadow && !glyph_info.style.has_shadow)) {
        return;
    }

    // Get corresponding loaded glyph bitmap
    _internal::Bitmap const& bitmap(!param.type.is_outline
        ? glyph_info.font->getGlyphBitmap(glyph_info.info.codepoint, glyph_info.style.charsize)
        : glyph_info.font->getOutlineBitmap(glyph_info.info.codepoint, glyph_info.style.charsize, glyph_info.style.outline_size));
    // Skip if bitmap is empty
    if (bitmap.width == 0 || bitmap.height == 0) {
        return;
    }

    // Shadow offset
    // TODO: turn this into an option
    if (param.type.is_shadow) {
        param.pen.x += 3 << 6;
        param.pen.y -= 3 << 6;
    }

    // Prepare copy
    _CopyBitmapArgs args(bitmap);

    // The (pen.x % 64 > 31) part is used to round up pixel fractions
    args.x0 = (param.pen.x >> 6) + (param.pen.x % 64 > 31) + bitmap.pen_left;
    args.y0 = param.line->charsize - (param.pen.y >> 6) - bitmap.pen_top;

    // Retrieve the color to use : either a plain one, or a function to call
    args.color = param.type.is_shadow ?
        glyph_info.color.shadow : param.type.is_outline ?
        glyph_info.color.outline : glyph_info.color.text;
    args.alpha = glyph_info.color.alpha;

    _copyBitmap(args);
}

// Copies a bitmap with given coords and color in _pixels
void TextArea::_copyBitmap(_CopyBitmapArgs& args)
{
    // Go through each pixel
    for (FT_Int i = 0, x = args.x0; i < args.bitmap.width; x++, i += args.bitmap.bpp) {
        for (FT_Int j = 0, y = args.y0; j < args.bitmap.height; y++, j++) {

            // Skip if coordinates are out the pixel array's bounds
            if (x < 0 || y < 0
                || x >= static_cast<int>(_w) || y >= static_cast<int>(_pixels_h))
                continue;

            // Retrieve buffer index and corresponding pixel reference
            size_t const buf_index = (size_t)(j * args.bitmap.width + i);
            RGBA32& pixel = _pixels[(size_t)(x + y * _w)];

            // Copy pixel
            switch (args.bitmap.pixel_mode) {
                // In this case, bitmaps have 1 byte per pixel.
                // Hence, they are monochrome (gray).
            case FT_PIXEL_MODE_GRAY: {
                // Skip if the glyph's pixel value is 0
                uint8_t const px_value = args.bitmap.buffer[buf_index];
                if (px_value == 0) {
                    continue;
                }
                // Determine color via function if needed
                if (!args.color.is_plain) {
                    args.color.plain = args.color.func(x * 1530 / static_cast<int>(_w));
                }
                // Blend with existing pixel, using the glyph's pixel value as an alpha
                pixel *= RGBA32(args.color.plain, px_value);
                // Lower alpha post blending if needed
                if (pixel.bytes.a > args.alpha) {
                    pixel.bytes.a = args.alpha;
                }
                break;
            }
            default:
                __LOG_METHOD_ERR("Unkown bitmap pixel mode.");
                return;
            }
        }
    }
}

__SSS_TR_END