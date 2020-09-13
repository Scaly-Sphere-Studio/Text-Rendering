#include "SSS/Text-Rendering/TextArea.h"

SSS_TR_BEGIN__

// Constructor, sets width & height.
// Throws an exception if width and/or height are <= 0.
TextArea::TextArea(int width, int height, TextAreaOpt const& opt) try
    : opt_(opt), w_(width), h_(height), pixels_h_(0), scrolling_(0),
    resize_(true), clear_(true), draw_(true), typewriter_(false),
    buffer_count_(0), glyph_count_(0), tw_cursor_(0), tw_next_cursor_(0)
{
    if (w_ <= 0 || h_ <= 0) {
        throw_exc(INVALID_ARGUMENT);
    }
}
CATCH_AND_RETHROW_METHOD_EXC__

// Destructor, clears out buffer cache.
TextArea::~TextArea() noexcept
{
}

// Resets the object to newly constructed state
void TextArea::clear() noexcept
{
    pixels_h_ = h_;
    resize_ = true;
    clear_ = true;
    draw_ = false;
    scrolling_ = 0;
    buffer_count_ = 0;
    glyph_count_ = 0;
    tw_cursor_ = 0;
    tw_next_cursor_ = 0;
    lines_.clear();
}

// Loads passed string in cache.
void TextArea::loadString(Font_Shared font, std::u32string const& str, BufferOpt const& opt) try
{
    // If an old buffer is available, update its contents
    if (buffers_.size() > buffer_count_) {
        buffers_.at(buffer_count_)->changeContents(font, str, opt);
    }
    // Else, create new buffer ptr & add it to the buffers_ vector
    else {
        buffers_.emplace_back(std::make_unique<Buffer>(font, str, opt));
    }
    Buffer_Ptr const& buffer(buffers_.at(buffer_count_));
    for (size_t i(0); i < buffer->size(); ++i) {
        font->loadGlyph(buffer->at(i).info.codepoint);
    }
    // Update counts
    glyph_count_ += buffer->size();
    ++buffer_count_;
    // Append format_
    updateInternalFormat_();
}
CATCH_AND_RETHROW_METHOD_EXC__

void TextArea::loadString(Font_Shared font, std::string const& str, BufferOpt const& opt) try
{
    std::u32string const u32str(str.cbegin(), str.cend());
    loadString(font, u32str, opt);
}
CATCH_AND_RETHROW_METHOD_EXC__

// Update the text format. To be called when charsize changes, for example.
void TextArea::updateFormat() try
{
    // Reshape all buffers
    for (Buffer_Ptr& buffer : buffers_) {
        buffer->reshape();
    }
    // Update global format
    float pre_size = static_cast<float>(pixels_h_ - h_);
    updateInternalFormat_();
    float const size_diff = static_cast<float>(pixels_h_ - h_) / pre_size;
    scrolling_ = (size_t)std::round(static_cast<float>(scrolling_) * size_diff);
    scrollingChanged_();
    clear_ = true;
    draw_ = true;
}
CATCH_AND_RETHROW_METHOD_EXC__

// Renders text to a 2D pixel array in the BGRA32 format.
// The passed array should have the same width and height
// as the TextArea object.
void TextArea::renderTo(void* ptr)
{
    drawIfNeeded_();
    if (ptr == nullptr) {
        LOG_ERR__(get_error(METHOD__, INVALID_ARGUMENT));
        return;
    }
    memcpy(ptr, &pixels_.at(scrolling_ * w_), (size_t)(4 * w_ * h_));
}

// Sets the writing mode (default: false):
// - true: No text is rendered at first, see writeNextChar();
// - false: Text is fully rendered
void TextArea::setTypeWriter(bool activate)
{
    typewriter_ = activate;
}

// Increments the typewriter mode's cursor
bool TextArea::incrementCursor()
{
    // Skip if the writing mode is set to default, or if all glyphs have been written
    if (!typewriter_ || tw_next_cursor_ >= glyph_count_) {
        return false;
    }

    ++tw_next_cursor_;
    Line_cit line = whichLine_(tw_next_cursor_);
    if (line->first_glyph == tw_next_cursor_) {
        if (line->scrolling - scrolling_ > static_cast<int>(h_)) {
            --tw_next_cursor_;
            return true;
        }
    }
    draw_ = true;
    return false;
}

// Scrolls down the text area, allowing next characters to be
// written. If scrolling down is not possible, this function
// returns *true*.
bool TextArea::scroll(int pixels)
{
    scrolling_ += pixels;
    scrollingChanged_();
    return false;
}

// Calls the at(); function from corresponding buffer to retrieve glyph infos
GlyphInfo TextArea::at_(size_t cursor) const try
{
    for (size_t i(0); i < buffer_count_; ++i) {
        Buffer_Ptr const& buffer = buffers_.at(i);
        if (buffer->size() > cursor) {
            return buffer->at(cursor);
        }
        cursor -= buffer->size();
    }
    throw_exc(OUT_OF_BOUND);
}
CATCH_AND_RETHROW_METHOD_EXC__

// Updates format_ by calling updateLineBreaks_ and updateSizes_.
void TextArea::updateInternalFormat_()
{
    lines_.clear();
    lines_.push_back({ 0, 0, 0, 0, 0 });
    Line_it line = lines_.begin();
    size_t cursor = 0;

    // Place pen at (0, 0), we don't take scrolling into account
    size_t last_divider(0);
    FT_Vector pen({ 0, 0 });
    while (cursor < glyph_count_) {
        // Retrieve glyph infos
        GlyphInfo const glyph(at_(cursor));
        // Update max_size
        int const charsize = glyph.font->getCharsize();
        if (line->charsize < charsize) {
            line->charsize = charsize;
        }
        size_t fullsize = (int)((float)charsize * glyph.line_spacing);
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
        if (pen.x < 0 || (pen.x >> 6) >= static_cast<int>(w_)) {
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
            lines_.push_back({ 0, 0, 0, 0, 0 });
            line = lines_.end() - 1;
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

    pixels_h_ = line->scrolling;
    if (pixels_h_ < h_) {
        pixels_h_ = h_;
    }
    resize_ = true;

    tw_cursor_ = 0;
    clear_ = true;
    draw_ = true;
}

void TextArea::scrollingChanged_()
{
    if (scrolling_ <= 0) {
        scrolling_ = 0;
    }
    int const max_scrolling = static_cast<int>(pixels_h_ - h_);
    if (scrolling_ >= max_scrolling) {
        scrolling_ = max_scrolling;
    }
}

Line_cit TextArea::whichLine_(size_t cursor)
{
    Line_cit line = lines_.cbegin();
    while (line != lines_.cend() - 1) {
        if (line->last_glyph >= cursor) {
            break;
        }
        ++line;
    }
    return line;
}

// Draws current area if needed
void TextArea::drawIfNeeded_()
{
    // Resize pixels array if needed
    if (resize_) {
        pixels_.resize(w_ * pixels_h_);
        resize_ = false;
    }
    // Clear out pixels array if needed
    if (clear_) {
        std::fill(pixels_.begin(), pixels_.end(), 0);
        clear_ = false;
    }
    // Skip if already drawn
    if (!draw_) {
        return;
    }

    // Determine draw parameters
    DrawParameters param(prepareDraw_());

    if (param.first_glyph > param.last_glyph || param.last_glyph > glyph_count_) {
        LOG_ERR__(get_error(METHOD__, INVALID_ARGUMENT));
        return;
    }


    // Render glyphs to pixels
    param.type = GlyphType(true, true);
    drawGlyphs_(param); // Border shadows
    param.type = GlyphType(false, true);
    drawGlyphs_(param); // Text shadows
    param.type = GlyphType(true, false);
    drawGlyphs_(param); // Borders
    param.type = GlyphType(false, false);
    drawGlyphs_(param); // Text
    // Update draw statement
    draw_ = false;
}

DrawParameters TextArea::prepareDraw_()
{
    DrawParameters param;
    // Determine range of glyphs to render
    if (!typewriter_) {
        param.first_glyph = 0;
        param.last_glyph = glyph_count_;
    }
    else {
        param.first_glyph = tw_cursor_;
        param.last_glyph = tw_next_cursor_;
        for (size_t cursor = tw_cursor_ + 1; cursor <= tw_next_cursor_
            && cursor < glyph_count_; ++cursor) {
            if (at_(cursor).is_word_divider) {
                tw_cursor_ = cursor;
            }
        }
    }

    // Determine first glyph's corresponding line
    param.line = whichLine_(param.first_glyph);
    if (param.line != lines_.cend()) {
        // Lower pen.y accordingly
        param.pen = { 5 * 64, -5 * 64 };
        param.pen.y -= (param.line->scrolling
            - static_cast<int>(param.line->fullsize)) << 6;

        // Determine the pen's position on the line
        for (size_t cursor = param.line->first_glyph; cursor < param.first_glyph; ++cursor) {
            hb_glyph_position_t const pos(at_(cursor).pos);
            param.pen.x += pos.x_advance;
            param.pen.y += pos.y_advance;
        }
    }
    return param;
}

// Renders glyphs to the pixels pointer and sets last_rendered_glyph_
// accordingly if pixels is non null.
void TextArea::drawGlyphs_(DrawParameters param)
{
    // Draw the glyphs
    for (size_t cursor = param.first_glyph; cursor < param.last_glyph; ++cursor) {
        drawGlyph_(param, cursor);
        // Handle line breaks. Return true if pen goes out of bound
        if (cursor == param.line->last_glyph && param.line != lines_.end() - 1) {
            param.pen.x = 5 << 6;
            param.pen.y -= static_cast<int>(param.line->fullsize) << 6;
            ++param.line;
        }
        // Increment pen's coordinates
        else {
            hb_glyph_position_t const pos(at_(cursor).pos);
            param.pen.x += pos.x_advance;
            param.pen.y += pos.y_advance;
        }
    }
}

// Loads (if needed) and renders the corresponding glyph to the
// pixels pointer at the given pen's coordinates.
void TextArea::drawGlyph_(DrawParameters param, size_t cursor)
{
    // Retrieve glyph infos
    GlyphInfo const glyph(at_(cursor));
    // Skip if the glyph alpha is zero, or if a outline is asked but not available
    if (glyph.color.alpha == 0
        || (param.type.is_stroked && (!glyph.style.outline || glyph.font->getOutline() == 0))
        || (param.type.is_shadow && !glyph.style.shadow)) {
        return;
    }

    // Retrieve loaded bitmaps and select the one to use
    Bitmaps const& bitmaps(glyph.font->getBitmaps(glyph.info.codepoint));
    FT_BitmapGlyph_Ptr const& bmp_glyph(param.type.is_stroked ? bitmaps.stroked : bitmaps.original);
    FT_Bitmap const& bitmap(bmp_glyph->bitmap);
    // Skip if bitmap is empty
    if (bitmap.width == 0 || bitmap.rows == 0) {
        return;
    }

    if (param.type.is_shadow) {
        param.pen.x += 3 << 6;
        param.pen.y -= 3 << 6;
    }
    
    // Retrieve coordinates and dimensions
    // Here, the (pen.x % 64 > 31) part is used to avoid 1px misplacements
    FT_Int const x0((param.pen.x >> 6) + (param.pen.x % 64 > 31) + bmp_glyph->left),
                 y0(param.line->charsize - (param.pen.y >> 6) - bmp_glyph->top),
                 i_max(bitmap.pitch),
                 i_add(bitmap.pitch / bitmap.width),
                 j_max(bitmap.rows);

    // Retrieve the color to use : either a plain one, or a function to call
    BGR24_s color;
    if (param.type.is_shadow) {
        color = glyph.color.shadow;
    }
    else {
        if (param.type.is_stroked) {
            color = glyph.color.outline;
        }
        else {
            color = glyph.color.text;
        }
    }

    // Copy bitmap to pixels
    for (FT_Int i = 0, x = x0; i < i_max; x++, i += i_add) {
        for (FT_Int j = 0, y = y0; j < j_max; y++, j++) {
            // Skip if coordinates are out the pixel array's bounds
            if (x < 0 || y < 0
                || x >= static_cast<int>(w_) || y >= static_cast<int>(pixels_h_))
                continue;
            // Retrieve buffer index and corresponding pixel reference
            size_t const buf_index = (size_t)(j * bitmap.pitch + i);
            BGRA32& pixel = pixels_[(size_t)(x + y * w_)];
            // Copy pixel
            switch (bitmap.pixel_mode) {
            case FT_PIXEL_MODE_GRAY:
                // Skip if buffer pixel value at index is zero
                if (bitmap.buffer[buf_index] == 0) {
                    continue;
                }
                // Determine color via function if needed
                if (!color.is_plain) {
                    color.plain = color.func(x * 1530 / static_cast<int>(w_));
                }
                // Blend with existing pixel, using the glyph's value as an alpha
                pixel *= BGRA32(color.plain, bitmap.buffer[buf_index]);
                // Lower alpha post blending if needed
                if (pixel.bytes.a > glyph.color.alpha) {
                    pixel.bytes.a = glyph.color.alpha;
                }
                break;
            default:
                LOG_ERR__(get_error(METHOD__, "Unkown bitmap pixel mode."));
                return;
            }
        }
    }
}

SSS_TR_END__