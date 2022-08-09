#include "SSS/Text-Rendering/_internal/AreaInternals.hpp"
#include "SSS/Text-Rendering/Globals.hpp"

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

int Line::x_offset() const noexcept
{
    switch (alignment) {
    case Alignment::Left:
        return direction == "ltr" ? 0 : unused_width;
    case Alignment::Center:
        return unused_width / 2;
    case Alignment::Right:
        return direction == "ltr" ? unused_width : 0;
    default:
        return 0;
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
        if (buffer.style.effect == Effect::Vibrate) {
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
        if (line.direction == "ltr")
            param.pen.x = (data.margin_v + line.x_offset()) << 6;
        else
            param.pen.x = (_w - data.margin_v - line.x_offset()) << 6;
        param.pen.y = -(data.margin_h << 6);
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
    for (size_t cursor = 0; cursor < data.last_glyph; ++cursor) {
        if (_beingCanceled()) return;
        GlyphInfo const& glyph_info(data.buffer_infos.getGlyph(cursor));
        BufferInfo const& buffer_info(data.buffer_infos.getBuffer(cursor));
        bool const is_ltr = buffer_info.lng.direction == "ltr";
        auto const move_cursor = [&]() {
            // Handle line breaks. Return true if pen goes out of bound
            if (cursor == line->last_glyph && line != data.lines.end() - 1) {
                param.pen.x = (is_ltr ? data.margin_v : (_w - data.margin_v)) << 6;
                param.pen.y -= static_cast<int>(line->fullsize) << 6;
                ++line;
                param.pen.x += (line->x_offset() << 6) * (is_ltr ? 1 : -1);
                param.charsize = line->charsize;
                ++param.effect_cursor;
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

void AreaPixels::_drawGlyph(DrawParameters const& param, BufferInfo const& buffer_info, GlyphInfo const& glyph_info)
{
    // Skip if the glyph alpha is zero, or if a outline is asked but not available
    if (buffer_info.color.alpha == 0
        || (param.is_outline && (!buffer_info.style.has_outline || buffer_info.style.outline_size <= 0))
        || (param.is_shadow && !buffer_info.style.has_shadow)) {
        return;
    }

    // Retrieve Font (must be loaded)
    Font::Ptr const& font = Lib::getFont(buffer_info.font);

    // Get corresponding loaded glyph bitmap
    Bitmap const& bitmap(!param.is_outline
        ? font->getGlyphBitmap(glyph_info.info.codepoint, buffer_info.style.charsize)
        : font->getOutlineBitmap(glyph_info.info.codepoint, buffer_info.style.charsize, buffer_info.style.outline_size));
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

    // Retrieve the color to use : either a plain one, or a function to call
    args.color = param.is_shadow ?
        buffer_info.color.shadow : param.is_outline ?
        buffer_info.color.outline : buffer_info.color.text;
    args.alpha = buffer_info.color.alpha;

    switch (buffer_info.style.effect) {
    case Effect::None:
        break;
    case Effect::Waves:
    case Effect::FadingWaves: {
        // 2PI
        static const float pi2 = std::acosf(-1.f) * 2.f;
        // Effect offset & sign
        int const n = 2 + std::labs(buffer_info.style.effect_offset);
        int const sign = buffer_info.style.effect_offset > 0 ? 1 : -1;
        // Pen value in real coordinates
        int const x = pen.x >> 6;
        // Time offset
        long long const t = _time.count() / 25;
        // Fading factor (1.f when simple waves)
        float fade = 1.f;
        if (buffer_info.style.effect == Effect::FadingWaves)
            fade += static_cast<float>(sign > 0 ? _w - x : x) / static_cast<float>(_w / 2);
        // Pen & fade based x value
        int const x_faded = static_cast<int>(fade * static_cast<float>(x * sign) / 10.f);
        // Final factor based on time, fading, x coordinates and sign
        float const factor = static_cast<float>((t - x_faded) % n) / static_cast<float>(n);
        // Size factor
        float const size = static_cast<float>(buffer_info.style.charsize) / 3.f;
        // Compute and add actual offset
        args.y0 += static_cast<int>(std::sinf(pi2 * factor) * size);
    }   break;
    case Effect::Vibrate: {
        int const n = buffer_info.style.effect_offset;
        if (n == 0) break;
        args.x0 += (n-1) - (_rng.at(param.effect_cursor).x % (n * 2));
        args.y0 += (n-1) - (_rng.at(param.effect_cursor).y % (n * 2));
    }   break;
    }

    if (param.is_shadow) {
        args.x0 += buffer_info.style.shadow_offset.x;
        args.y0 += buffer_info.style.shadow_offset.y;
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
                    using Func = Format::Color::Func;
                case Func::none:
                    color = args.color.plain;
                    break;
                case Func::rainbow:
                    color = rainbow((_time.count() / 10 - x - y * 2) % _w, _w);
                    break;
                case Func::rainbowFixed:
                    color = rainbow((x + y * 2) % _w, _w);
                    break;
                }
                // Blend with existing pixel, using the glyph's pixel value as an alpha
                pixel *= RGBA32(color, px_value);
                // Lower alpha post blending if needed
                if (pixel.bytes.a > args.alpha) {
                    pixel.bytes.a = args.alpha;
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