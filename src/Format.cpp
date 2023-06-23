#include "Text-Rendering/Format.hpp"

SSS_TR_BEGIN;

SSS_TR_API Format default_fmt{};

bool Color::operator==(Color const& color) const
{
    if (func != color.func)
        return false;
    else if (func == ColorFunc::None || color.func == ColorFunc::None)
        return rgb == color.rgb;
    return true;
}

Format::Format() noexcept
{
    static bool init = false;
    if (init)
        *this = default_fmt;
    else
        init = true;
};

SSS_TR_END;