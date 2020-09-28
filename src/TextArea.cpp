#include "SSS/Text-Rendering/TextArea.hpp"

SSS_TR_BEGIN__

    // --- Constructor, destructor & clear function ---

// Constructor, sets width & height.
// Throws an exception if width and/or height are <= 0.
TextArea::TextArea(int width, int height) try
    : w_(width), h_(height)
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
    // Reset scrolling
    scrolling_ = 0;
    if (pixels_h_ != h_) {
        pixels_h_ = h_;
        resize_ = true;
    }
    // Reset buffers
    buffer_count_ = 0;
    glyph_count_ = 0;
    lines_.clear();
    // Reset pixels
    clear_ = true;
    draw_ = false;
    // Reset typewriter
    tw_cursor_ = 0;
    tw_next_cursor_ = 0;
}

    // --- Basic functions ---

// Loads passed string in cache.
void TextArea::loadString(std::u32string const& str, TextOpt const& opt) try
{
    // Ensure we set the corresponding charsize
    opt.font->useCharsize(opt.style.charsize);

    // If an old buffer is available, update its contents
    if (buffers_.size() > buffer_count_) {
        buffers_.at(buffer_count_)->changeContents(str, opt);
    }
    // Else, create & fill new buffer ptr & add it to the buffers_ vector
    else {
        buffers_.emplace_back(std::make_unique<_Buffer>(str, opt));
    }
    _Buffer::Ptr const& buffer(buffers_.at(buffer_count_));

    // Load glyphs retrieved by the buffer
    int const outline_size = opt.style.has_outline ? opt.style.outline_size : 0;
    for (size_t i(0); i < buffer->size(); ++i) {
        opt.font->loadGlyph(buffer->at(i).info.codepoint, opt.style.charsize, outline_size);
    }
    // Update counts
    glyph_count_ += buffer->size();
    ++buffer_count_;
    // Append format_
    update_Lines_();
}
CATCH_AND_RETHROW_METHOD_EXC__

// Loads passed string in cache.
void TextArea::loadString(std::string const& str, TextOpt const& opt) try
{
    // Convert string to u32string
    std::u32string const u32str(str.cbegin(), str.cend());
    loadString(u32str, opt);
}
CATCH_AND_RETHROW_METHOD_EXC__

// Renders text to a 2D pixel array in the BGRA32 format.
// The passed array should have the same width and height
// as the TextArea object.
void TextArea::renderTo(void* ptr)
{
    drawIfNeeded_();
    if (ptr == nullptr) {
        LOG_METHOD_ERR__(INVALID_ARGUMENT)
        return;
    }
    memcpy(ptr, &pixels_.at(scrolling_ * w_), (size_t)(4 * w_ * h_));
}

    // --- Format functions ---

// Update the text format. To be called when charsize changes, for example.
void TextArea::updateFormat() try
{
    // Reshape all buffers
    for (_Buffer::Ptr& buffer : buffers_) {
        buffer->reshape();
    }
    // Update global format
    float pre_size = static_cast<float>(pixels_h_ - h_);
    update_Lines_();
    float const size_diff = static_cast<float>(pixels_h_ - h_) / pre_size;
    scrolling_ = (size_t)std::round(static_cast<float>(scrolling_) * size_diff);
    scrollingChanged_();
    clear_ = true;
    draw_ = true;
}
CATCH_AND_RETHROW_METHOD_EXC__

// Scrolls up (negative values) or down (positive values)
// Any excessive scrolling will be negated,
// hence, this function is safe.
void TextArea::scroll(int pixels) noexcept
{
    scrolling_ += pixels;
    scrollingChanged_();
}

    // --- Typewriter functions ---

// Sets the writing mode (default: false):
// - true: Text is rendered char by char, see incrementCursor();
// - false: Text is fully rendered
void TextArea::setTypeWriter(bool activate) noexcept
{
    if (typewriter_ != activate) {
        tw_cursor_ = 0;
        tw_next_cursor_ = 0;
        clear_ = true;
        draw_ = true;
        typewriter_ = activate;
    }
}

// Increments the typewriter's cursor. Start point is 0.
// The first call will render the 1st character.
// Second call will render the both the 1st and 2nd character.
// Etc...
bool TextArea::incrementCursor() noexcept
{
    // Skip if the writing mode is set to default, or if all glyphs have been written
    if (!typewriter_ || tw_next_cursor_ >= glyph_count_) {
        return false;
    }

    ++tw_next_cursor_;
    _Line::cit line = which_Line_(tw_next_cursor_);
    if (line->first_glyph == tw_next_cursor_) {
        if (line->scrolling - scrolling_ > static_cast<int>(h_)) {
            --tw_next_cursor_;
            return true;
        }
    }
    draw_ = true;
    return false;
}

    // --- Static functions ---

// Calls the at(); function from corresponding Buffer
_GlyphInfo TextArea::at_(size_t cursor) const try
{
    for (size_t i(0); i < buffer_count_; ++i) {
        _Buffer::Ptr const& buffer = buffers_.at(i);
        if (buffer->size() > cursor) {
            return buffer->at(cursor);
        }
        cursor -= buffer->size();
    }
    throw_exc(OUT_OF_BOUND);
}
CATCH_AND_RETHROW_METHOD_EXC__


// Updates scrolling_
void TextArea::scrollingChanged_() noexcept
{
    if (scrolling_ <= 0) {
        scrolling_ = 0;
    }
    int const max_scrolling = static_cast<int>(pixels_h_ - h_);
    if (scrolling_ >= max_scrolling) {
        scrolling_ = max_scrolling;
    }
}

// Updates lines_
void TextArea::update_Lines_()
{
    // Reset lines_
    lines_.clear();
    lines_.emplace_back();
    _Line::it line = lines_.begin();
    size_t cursor = 0;

    // Place pen at (0, 0), we don't take scrolling into account
    size_t last_divider(0);
    FT_Vector pen({ 0, 0 });
    while (cursor < glyph_count_) {
        // Retrieve glyph infos
        _GlyphInfo const glyph(at_(cursor));
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

// Returns corresponding _Line iterator, or cend()
_Line::cit TextArea::which_Line_(size_t cursor) const noexcept
{
    _Line::cit line = lines_.cbegin();
    while (line != lines_.cend() - 1) {
        if (line->last_glyph >= cursor) {
            break;
        }
        ++line;
    }
    return line;
}

    // --- Private functions -> Draw functions ---

// Draws current area if draw_ is set to true
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
    _DrawParameters param(prepareDraw_());

    if (param.first_glyph > param.last_glyph || param.last_glyph > glyph_count_) {
        LOG_ERR__(get_error(METHOD__, INVALID_ARGUMENT));
        return;
    }

    // Render glyphs to pixels
    param.type = _DrawParameters::_Outline_Shadow(true, true);
    drawGlyphs_(param); // Border shadows
    param.type = _DrawParameters::_Outline_Shadow(false, true);
    drawGlyphs_(param); // Text shadows
    param.type = _DrawParameters::_Outline_Shadow(true, false);
    drawGlyphs_(param); // Borders
    param.type = _DrawParameters::_Outline_Shadow(false, false);
    drawGlyphs_(param); // Text
    // Update draw statement
    draw_ = false;
}

// Prepares drawing parameters, which will be used multiple times per draw
_DrawParameters TextArea::prepareDraw_()
{
    _DrawParameters param;
    // Determine range of glyphs to render
    // If typewriter_ is set to false, this means all glyphs
    if (!typewriter_) {
        param.first_glyph = 0;
        param.last_glyph = glyph_count_;
    }
    // Else, only render needed glyphs
    else {
        param.first_glyph = tw_cursor_;
        param.last_glyph = tw_next_cursor_;
        // TypeWriter -> Update current character position
        for (size_t cursor = tw_cursor_ + 1; cursor <= tw_next_cursor_
            && cursor < glyph_count_; ++cursor) {
            // It is important to note that we only update the current
            // character position after a whole word has been written.
            // This is to avoid drawing over previous glyphs.
            if (at_(cursor).is_word_divider) {
                tw_cursor_ = cursor;
            }
        }
    }

    // Determine first glyph's corresponding line
    param.line = which_Line_(param.first_glyph);
    if (param.line != lines_.cend()) {
        // Lower pen.y accordingly
        // TODO: determine exact offsets needed, instead of using 5px
        param.pen = { 5 * 64, -5 * 64 };
        param.pen.y -= (param.line->scrolling
            - static_cast<int>(param.line->fullsize)) << 6;

        // Shift the pen on the line accordingly
        for (size_t cursor = param.line->first_glyph; cursor < param.first_glyph; ++cursor) {
            hb_glyph_position_t const pos(at_(cursor).pos);
            param.pen.x += pos.x_advance;
            param.pen.y += pos.y_advance;
        }
    }
    return param;
}

// Draws shadows, outlines, or plain glyphs
void TextArea::drawGlyphs_(_DrawParameters param)
{
    // Draw the glyphs
    for (size_t cursor = param.first_glyph; cursor < param.last_glyph; ++cursor) {
        _GlyphInfo const glyph(at_(cursor));
        drawGlyph_(param, glyph);
        // Handle line breaks. Return true if pen goes out of bound
        if (cursor == param.line->last_glyph && param.line != lines_.end() - 1) {
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
void TextArea::drawGlyph_(_DrawParameters param, _GlyphInfo const& glyph_info)
{
    // Skip if the glyph alpha is zero, or if a outline is asked but not available
    if (glyph_info.color.alpha == 0
        || (param.type.is_outline && !glyph_info.style.has_outline)
        || (param.type.is_shadow && !glyph_info.style.has_shadow)) {
        return;
    }

    // Get corresponding loaded glyph bitmap
    FT_BitmapGlyph_Ptr const& bmp_glyph(!param.type.is_outline
        ? glyph_info.font->getGlyphBitmap(glyph_info.info.codepoint, glyph_info.style.charsize)
        : glyph_info.font->getOutlineBitmap(glyph_info.info.codepoint, glyph_info.style.charsize, glyph_info.style.outline_size));
    FT_Bitmap const& bitmap(bmp_glyph->bitmap);
    // Skip if bitmap is empty
    if (bitmap.width == 0 || bitmap.rows == 0) {
        return;
    }

    // Shadow offset
    // TODO: turn this into an option
    if (param.type.is_shadow) {
        param.pen.x += 3 << 6;
        param.pen.y -= 3 << 6;
    }
    
    // Prepare copy
    _CopyBitmapArgs args;

    // The (pen.x % 64 > 31) part is used to round up pixel fractions
    args.x0 = (param.pen.x >> 6) + (param.pen.x % 64 > 31) + bmp_glyph->left;
    args.y0 = param.line->charsize - (param.pen.y >> 6) - bmp_glyph->top;

    // Retrieve the color to use : either a plain one, or a function to call
    args.bitmap = bitmap;
    args.alpha = glyph_info.color.alpha;
    args.color = param.type.is_shadow ?
        glyph_info.color.shadow : param.type.is_outline ?
            glyph_info.color.outline : glyph_info.color.text;

    copyBitmap_(args);
}

// Copies a bitmap with given coords and color in pixels_
void TextArea::copyBitmap_(_CopyBitmapArgs& args)
{
    // Create loop const variables
    FT_Int const
        width = args.bitmap.pitch,
        height = args.bitmap.rows,
        bpp = args.bitmap.pitch / args.bitmap.width;

    // Go through each pixel
    for (FT_Int i = 0, x = args.x0; i < width; x++, i += bpp) {
        for (FT_Int j = 0, y = args.y0; j < height; y++, j++) {

            // Skip if coordinates are out the pixel array's bounds
            if (x < 0 || y < 0
                || x >= static_cast<int>(w_) || y >= static_cast<int>(pixels_h_))
                continue;

            // Retrieve buffer index and corresponding pixel reference
            size_t const buf_index = (size_t)(j * args.bitmap.pitch + i);
            BGRA32& pixel = pixels_[(size_t)(x + y * w_)];

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
                    args.color.plain = args.color.func(x * 1530 / static_cast<int>(w_));
                }
                // Blend with existing pixel, using the glyph's pixel value as an alpha
                pixel *= BGRA32(args.color.plain, px_value);
                // Lower alpha post blending if needed
                if (pixel.bytes.a > args.alpha) {
                    pixel.bytes.a = args.alpha;
                }
                break;
            }
            default:
                LOG_ERR__(get_error(METHOD__, "Unkown bitmap pixel mode."));
                return;
            }
        }
    }
}

SSS_TR_END__