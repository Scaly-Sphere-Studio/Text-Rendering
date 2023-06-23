#include "AreaInternals.hpp"

SSS_TR_BEGIN;
INTERNAL_BEGIN;

Line::cit Line::which(vector const& lines, size_t cursor) noexcept
{
    Line::cit line = lines.cbegin();
    while (line != lines.cend() - 1) {
        if (line->last_glyph >= cursor) {
            break;
        }
        ++line;
    }
    return line;
}

int Line::x_offset(bool is_ltr) const noexcept
{
    switch (alignment) {
    case Alignment::Left:
        return is_ltr ? 0 : unused_width;
    case Alignment::Center:
        return unused_width / 2;
    case Alignment::Right:
        return is_ltr ? unused_width : 0;
    default:
        return 0;
    }
}

void Line::replace_pen(FT_Vector& pen, BufferInfoVector const& buffer_infos, size_t cursor) const noexcept
{
    bool const area_is_ltr = buffer_infos.isLTR();
    bool const is_ltr = buffer_infos.getBuffer(cursor).fmt.lng_direction == "ltr";
    if (area_is_ltr != is_ltr) {
        for (size_t i = cursor; i != last_glyph; ++i) {
            if ((buffer_infos.getBuffer(i).fmt.lng_direction == buffer_infos.getDirection()))
                break;
            pen.x += buffer_infos.getGlyph(i).pos.x_advance * (area_is_ltr ? 1 : -1);
        }
    }
    else {
        for (size_t i = cursor - 1; i != first_glyph - 1; --i) {
            if ((buffer_infos.getBuffer(i).fmt.lng_direction == buffer_infos.getDirection()))
                break;
            pen.x += buffer_infos.getGlyph(i).pos.x_advance * (area_is_ltr ? 1 : -1);
        }
    }
}

void AreaPixels::_asyncFunction(AreaData data)
{
    // Copy given data
    _w = data.w;
    _h = data.h;
    _pixels_h = data.pixels_h;
    // Resize if needed
    _pixels.resize(_w * _pixels_h);
    // Clear
    std::fill(_pixels.begin(), _pixels.end(), data.bg_color);
    // Reset time
    _time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
    // Generate RNG if needed
    for (auto const& buffer : data.buffer_infos) {
        if (buffer.fmt.effect == Effect::Vibrate) {
            _rng.resize(data.buffer_infos.glyphCount());
            for (FT_Vector& vec : _rng) {
                vec.x = std::rand();
                vec.y = std::rand();
            }
            break;
        }
    }

    DrawParameters param;
    {
        Line const& line = data.lines.front();
        if (data.buffer_infos.isLTR())
            param.pen.x = (data.margin_v + line.x_offset(data.buffer_infos.isLTR())) << 6;
        else
            param.pen.x = (_w - data.margin_v - line.x_offset(data.buffer_infos.isLTR())) << 6;
        param.pen.y = -(data.margin_h << 6);
    }
    // Draw selected text's background
    if (data.selected.state) {
        param.is_selected_bg = true;
        _drawGlyphs(data, param);
        if (_beingCanceled()) return;
        param.is_selected_bg = false;
    }
    // Draw Outline shadows
    param.is_shadow = true;
    param.is_outline = true;
    _drawGlyphs(data, param);
    if (_beingCanceled()) return;
    
    // Draw Text shadows
    param.is_outline = false;
    _drawGlyphs(data, param);
    if (_beingCanceled()) return;

    // Draw Outlines
    param.is_shadow = false;
    param.is_outline = true;
    _drawGlyphs(data, param);
    if (_beingCanceled()) return;

    // Draw Text
    param.is_outline = false;
    _drawGlyphs(data, param);

    // Draw cursor
    if (data.draw_cursor) {
        for (int y = data.cursor_y - data.cursor_h; y < data.cursor_y; ++y) {
            for (int x = data.cursor_x; x < data.cursor_x + 2; ++x) {
                size_t const i = x + y * _w;
                if (x < data.w && i < _pixels.size()) {
                    _pixels[i] = 0xFFFFFFFF;
                }
            }
        }
    }
}

void AreaPixels::_drawGlyphs(AreaData const& data, DrawParameters param)
{
    // Draw the glyphs
    Line::cit line = data.lines.cbegin();
    param.charsize = line->charsize;
    bool const area_is_ltr = data.buffer_infos.isLTR();
    bool is_ltr = data.buffer_infos.isLTR();
    for (size_t cursor = 0; cursor < data.last_glyph; ++cursor) {
        if (_beingCanceled()) return;
        GlyphInfo const& glyph_info(data.buffer_infos.getGlyph(cursor));
        BufferInfo const& buffer_info(data.buffer_infos.getBuffer(cursor));
        // Re-position the pen if direction changed
        if ((buffer_info.fmt.lng_direction == "ltr") != is_ltr) {
            line->replace_pen(param.pen, data.buffer_infos, cursor);
            is_ltr = !is_ltr;
        }
        auto const move_cursor = [&]() {
            // Handle line breaks. Return true if pen goes out of bound
            if (cursor == line->last_glyph && line != data.lines.end() - 1) {
                param.pen.x = (area_is_ltr ? data.margin_v : (_w - data.margin_v)) << 6;
                param.pen.y -= line->fullsize << 6;
                ++line;
                param.pen.x += (line->x_offset(area_is_ltr) << 6) * (area_is_ltr ? 1 : -1);
                param.charsize = line->charsize;
                ++param.effect_cursor;
                if (buffer_info.fmt.lng_direction != data.buffer_infos.getDirection()) {
                    line->replace_pen(param.pen, data.buffer_infos, cursor+1);
                }
            }
            // Increment pen's coordinates
            else {
                if (glyph_info.pos.x_advance != 0) {
                    ++param.effect_cursor;
                }
                param.pen.x += glyph_info.pos.x_advance * (is_ltr ? 1 : -1);
                param.pen.y += glyph_info.pos.y_advance;
            }
        };
        if (param.is_selected_bg) {
            if (cursor >= data.selected.first && cursor < data.selected.last) {
                int x = (is_ltr ? param.pen.x : param.pen.x - glyph_info.pos.x_advance) >> 6;
                int const x_max = (is_ltr ? param.pen.x + glyph_info.pos.x_advance : param.pen.x) >> 6,
                          y_max = -(param.pen.y >> 6) + line->fullsize;
                for ( ; x < x_max; ++x) {
                    for (int y = -(param.pen.y >> 6); y < y_max; ++y) {
                        if (x < 0 || y < 0 || x >= _w || y >= _pixels_h)
                            continue;
                        _pixels[(size_t)(x + y * _w)] = RGB24(0, 0, 128);
                    }
                }
            }
            move_cursor();
        }
        else {
            if (!is_ltr)
                move_cursor();
            if (!glyph_info.is_new_line) {
                try {
                    _drawGlyph(param, buffer_info, glyph_info);
                }
                catch (std::exception const& e) {
                    std::string str(toString("cursor #") + toString(cursor));
                    throw_exc(CONTEXT_MSG(str, e.what()));
                }
            }
            if (is_ltr)
                move_cursor();
        }
    }
}

void AreaPixels::_drawGlyph(DrawParameters const& param, BufferInfo const& buffer_info, GlyphInfo const& glyph_info)
{
    // Skip if the glyph alpha is zero, or if a outline is asked but not available
    if (buffer_info.fmt.alpha == 0
        || (param.is_outline && (!buffer_info.fmt.has_outline || buffer_info.fmt.outline_size <= 0))
        || (param.is_shadow && !buffer_info.fmt.has_shadow)) {
        return;
    }

    // Retrieve Font (must be loaded)
    Font& font = Lib::getFont(buffer_info.fmt.font);

    // Get corresponding loaded glyph bitmap
    Bitmap const& bitmap(!param.is_outline
        ? font.getGlyphBitmap(glyph_info.info.codepoint, buffer_info.fmt.charsize)
        : font.getOutlineBitmap(glyph_info.info.codepoint, buffer_info.fmt.charsize, buffer_info.fmt.outline_size));
    // Skip if bitmap is empty
    if (bitmap.width == 0 || bitmap.height == 0) {
        return;
    }

    FT_Vector pen(param.pen);
    // Shadow offset

    // Prepare copy
    _CopyBitmapArgs args(bitmap);

    // The (pen.x % 64 > 31) part is used to round up pixel fractions
    args.x0 = (pen.x >> 6) + bitmap.pen_left;
    args.y0 = param.charsize - (pen.y >> 6) - bitmap.pen_top;

    // Retrieve the color to use
    if (param.is_shadow) {
        args.color = buffer_info.fmt.shadow_color;
    }
    else if (param.is_outline) {
        args.color = buffer_info.fmt.outline_color;
    }
    else [[likely]] {
        args.color = buffer_info.fmt.text_color;
    }
    args.alpha = buffer_info.fmt.alpha;

    switch (buffer_info.fmt.effect) {
    case Effect::None:
        break;
    case Effect::Waves:
    case Effect::FadingWaves: {
        // 2PI
        static const float pi2 = std::acosf(-1.f) * 2.f;
        // Effect offset & sign
        int const n = 2 + std::labs(buffer_info.fmt.effect_offset);
        int const sign = buffer_info.fmt.effect_offset > 0 ? 1 : -1;
        // Pen value in real coordinates
        int const x = pen.x >> 6;
        // Time offset
        long long const t = _time.count() / 25;
        // Fading factor (1.f when simple waves)
        float fade = 1.f;
        if (buffer_info.fmt.effect == Effect::FadingWaves)
            fade += static_cast<float>(sign > 0 ? _w - x : x) / static_cast<float>(_w / 2);
        // Pen & fade based x value
        int const x_faded = static_cast<int>(fade * static_cast<float>(x * sign) / 10.f);
        // Final factor based on time, fading, x coordinates and sign
        float const factor = static_cast<float>((t - x_faded) % n) / static_cast<float>(n);
        // Size factor
        float const size = static_cast<float>(buffer_info.fmt.charsize) / 3.f;
        // Compute and add actual offset
        args.y0 += static_cast<int>(std::sinf(pi2 * factor) * size);
    }   break;
    case Effect::Vibrate: {
        int const n = buffer_info.fmt.effect_offset;
        if (n == 0) break;
        args.x0 += (n-1) - (_rng.at(param.effect_cursor).x % (n * 2));
        args.y0 += (n-1) - (_rng.at(param.effect_cursor).y % (n * 2));
    }   break;
    }

    if (param.is_shadow) {
        args.x0 += buffer_info.fmt.shadow_offset_x;
        args.y0 += buffer_info.fmt.shadow_offset_y;
    }

    _copyBitmap(args);
}

void AreaPixels::_copyBitmap(_CopyBitmapArgs& args)
{
    // Go through each pixel
    for (FT_Int i = 0, x = args.x0; i < args.bitmap.width; x++, i += args.bitmap.bpp) {
        for (FT_Int j = 0, y = args.y0; j < args.bitmap.height; y++, j++) {

            // Skip if coordinates are out the pixel array's bounds
            if (x < 0 || y < 0 || x >= _w || y >= _pixels_h)
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
                // Determine color
                RGB24 color;
                switch (args.color.func) {
                case ColorFunc::None:
                    color = args.color;
                    break;
                case ColorFunc::Rainbow:
                    color = rainbow((_time.count() / 10 - x - y * 2) % _w, _w);
                    break;
                case ColorFunc::RainbowFixed:
                    color = rainbow((x + y * 2) % _w, _w);
                    break;
                }
                // Blend with existing pixel, using the glyph's pixel value as an alpha
                pixel *= RGBA32(color, px_value);
                // Lower alpha post blending if needed
                if (pixel.a > args.alpha) {
                    pixel.a = args.alpha;
                }
                break;
            }
            default:
                LOG_METHOD_ERR("Unkown bitmap pixel mode.");
                return;
            }
        }
    }
}

INTERNAL_END;
SSS_TR_END;