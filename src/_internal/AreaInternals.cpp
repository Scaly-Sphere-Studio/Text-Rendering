#include "SSS/Text-Rendering/_internal/AreaInternals.hpp"
#include "SSS/Text-Rendering/Lib.hpp"

__SSS_TR_BEGIN;
__INTERNAL_BEGIN;

Line::cit Line::which(vector const& lines, size_t cursor)
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

AreaPixels::~AreaPixels()
{
    cancel();
}

void AreaPixels::draw(DrawParameters param, int w, int h, int pixels_h,
    std::vector<Line> lines, BufferInfoVector buffer_infos)
{
    cancel();
    _w = w;
    _h = h;
    _pixels_h = pixels_h;
    _lines = lines;
    _buffer_infos = buffer_infos;
    param.line = Line::which(_lines, param.first_glyph);

    run(param);
}

void AreaPixels::_asyncFunction(DrawParameters param)
{
    _pixels.resize(_w * _pixels_h);
    if constexpr (DEBUGMODE) {
        std::fill(_pixels.begin(), _pixels.end(), RGBA32(0, 0, 0, 122));
    }
    else {
        std::fill(_pixels.begin(), _pixels.end(), RGBA32(0));
    }

    // Draw Outline shadows
    param.type.is_shadow = true;
    param.type.is_outline = true;
    _drawGlyphs(param);
    if (_beingCanceled()) return;
    
    // Draw Text shadows
    param.type.is_outline = false;
    _drawGlyphs(param);
    if (_beingCanceled()) return;

    // Draw Outlines
    param.type.is_shadow = false;
    param.type.is_outline = true;
    _drawGlyphs(param);
    if (_beingCanceled()) return;

    // Draw Text
    param.type.is_outline = false;
    _drawGlyphs(param);
}

void AreaPixels::_drawGlyphs(DrawParameters param)
{
    // Draw the glyphs
    for (size_t cursor = param.first_glyph; cursor < param.last_glyph; ++cursor) {
        if (_beingCanceled()) return;
        GlyphInfo const& glyph_info(_buffer_infos.getGlyph(cursor));
        BufferInfo const& buffer_info(_buffer_infos.getBuffer(cursor));
        try {
            _drawGlyph(param, buffer_info, glyph_info);
        }
        catch (std::exception const& e) {
            std::string str(toString("cursor #") + toString(cursor));
            throw_exc(__CONTEXT_MSG(str, e.what()));
        }
        // Handle line breaks. Return true if pen goes out of bound
        if (cursor == param.line->last_glyph && param.line != _lines.end() - 1) {
            param.pen.x = 5 << 6;
            param.pen.y -= static_cast<int>(param.line->fullsize) << 6;
            ++param.line;
        }
        // Increment pen's coordinates
        else {
            param.pen.x += glyph_info.pos.x_advance;
            param.pen.y += glyph_info.pos.y_advance;
        }
    }
}

void AreaPixels::_drawGlyph(DrawParameters param, BufferInfo const& buffer_info, GlyphInfo const& glyph_info)
{
    // Skip if the glyph alpha is zero, or if a outline is asked but not available
    if (buffer_info.color.alpha == 0
        || (param.type.is_outline && (!buffer_info.style.has_outline || buffer_info.style.outline_size <= 0))
        || (param.type.is_shadow && !buffer_info.style.has_shadow)) {
        return;
    }

    // Retrieve Font (must be loaded)
    Font::Ptr const& font = Lib::getFont(buffer_info.font);

    // Get corresponding loaded glyph bitmap
    Bitmap const& bitmap(!param.type.is_outline
        ? font->getGlyphBitmap(glyph_info.info.codepoint, buffer_info.style.charsize)
        : font->getOutlineBitmap(glyph_info.info.codepoint, buffer_info.style.charsize, buffer_info.style.outline_size));
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
        buffer_info.color.shadow : param.type.is_outline ?
        buffer_info.color.outline : buffer_info.color.text;
    args.alpha = buffer_info.color.alpha;

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
                    color = rainbow(x, _w);
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
                __LOG_METHOD_ERR("Unkown bitmap pixel mode.");
                return;
            }
        }
    }
}

__INTERNAL_END;
__SSS_TR_END;