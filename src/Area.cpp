#include "_internal/AreaInternals.hpp"
#include "Text-Rendering/Area.hpp"
#include "Text-Rendering/Globals.hpp"

#include <cwctype>

SSS_TR_BEGIN;

static void from_json(nlohmann::json const& j, Color& color)
{
    if (j.is_number()) {
        color.rgb = j.get<uint32_t>();
        color.func = ColorFunc::None;
        return;
    }
    if (auto const f = j.get<ColorFunc>(); f != ColorFunc::Invalid) {
        color.func = f;
        return;
    }
    if (j.contains("r"))
        j.at("r").get_to(color.r);
    if (j.contains("g"))
        j.at("g").get_to(color.g);
    if (j.contains("b"))
        j.at("b").get_to(color.b);
    if (j.contains("rgb"))
        color.rgb = j.at("rgb").get<uint32_t>();
    if (j.contains("func"))
        j.at("func").get_to(color.func);
}

static void to_json(nlohmann::json& j, Color const& color)
{
    if (color.func == ColorFunc::None)
        j = color.rgb ;
    else
        j = color.func ;
}

NLOHMANN_JSON_SERIALIZE_ENUM(Alignment, {
    { Alignment::Invalid, nullptr },
    { Alignment::Left, "Left" },
    { Alignment::Center, "Center" },
    { Alignment::Right, "Right" },
})

NLOHMANN_JSON_SERIALIZE_ENUM(Effect, {
    { Effect::Invalid, nullptr },
    { Effect::None, "None" },
    { Effect::Vibrate, "Vibrate" },
    { Effect::Waves, "Waves" },
    { Effect::FadingWaves, "FadingWaves" },
})

NLOHMANN_JSON_SERIALIZE_ENUM(ColorFunc, {
    { ColorFunc::Invalid, nullptr },
    { ColorFunc::None, "None" },
    { ColorFunc::Rainbow, "Rainbow" },
    { ColorFunc::RainbowFixed, "RainbowFixed" },
})

CommandHistory Area::history{};
int Area::_default_margin_h{ 10 };
int Area::_default_margin_v{ 10 };
Area::Weak Area::_focused{};

    // --- Constructor, destructor & clear function ---

// Constructor, creates a default Buffer
Area::Area() try
    : _buffer_infos(std::make_unique<_internal::BufferInfoVector>())
{
    for (auto& pixels : _pixels) {
        pixels.reset(new _internal::AreaPixels);
        if (!pixels)
            throw_exc("Couldn't allocate internal data");
        _observe(*pixels);
    }
    _buffers.push_back(std::make_unique<_internal::Buffer>(TextPart(U"", _format)));
    _updateBufferInfos();
    if (Log::TR::Areas::query(Log::TR::Areas::get().life_state)) {
        char buff[256];
        sprintf_s(buff, "Created Area #%p.", this);
        LOG_TR_MSG(buff);
    }
}
CATCH_AND_RETHROW_METHOD_EXC;

// Destructor, clears out buffer cache.
Area::~Area() noexcept
{
    if (Log::TR::Areas::query(Log::TR::Areas::get().life_state)) {
        char buff[256];
        sprintf_s(buff, "Deleted Area #%p.", this);
        LOG_TR_MSG(buff);
    }
}

void Area::setWrapping(bool wrapping) noexcept
{
    if (_wrapping != wrapping) {
        _wrapping = wrapping;
        _updateBufferInfos();
    }
}

bool Area::getWrapping() const noexcept
{
    return _wrapping;
}

void Area::setWrappingMinWidth(int min_w) noexcept
{
    _min_w = min_w;
    _updateBufferInfos();
}

int Area::getWrappingMinWidth() const noexcept
{
    return _min_w;
}

void Area::setWrappingMaxWidth(int max_w) noexcept
{
    _max_w = max_w;
    _updateBufferInfos();
}

int Area::getWrappingMaxWidth() const noexcept
{
    return _max_w;
}

int Area::getUsedWidth() const noexcept
{
    return std::max_element(_lines.cbegin(), _lines.cend(),
        [](auto& a, auto& b)->bool { return a.used_width < b.used_width; }
    )->used_width + static_cast<int>(_wrapping);
}

Area::Shared Area::create(int width, int height)
{
    Shared area = create();
    area->setDimensions(width, height);
    return area;
}

Area::Shared Area::create(std::u32string const& str, Format fmt)
{
    Shared area = create();
    area->setFormat(fmt);
    area->parseStringU32(str);
    return area;
}

Area::Shared Area::create(std::string const& str, Format fmt)
{
    return create(strToStr32(str), fmt);
}

    // --- Basic functions ---

void Area::setFormat(Format const& format) try
{
    _format = format;
    if (!_buffers.empty())
        parseStringU32(_buffer_infos->getString());
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

void Area::setMargins(int marginV, int marginH)
{
    if (_margin_v != marginV || _margin_h != marginH) {
        _margin_v = marginV;
        _margin_h = marginH;
        _updateLines();
    }
}

void Area::setMarginH(int marginH)
{
    setMargins(_margin_v, marginH);
}

void Area::setMarginV(int marginV)
{
    setMargins(marginV, _margin_h);
}

void Area::setClearColor(RGBA32 color)
{
    _bg_color = color;
    _draw = true;
}

static void jsonToFmt(nlohmann::json const& json, Format& fmt)
{
    auto const has_value = [&json](char const* key) {
        return json.contains(key) && !json.at(key).is_null();
    };

    // Font
    if (has_value("font"))
        fmt.font = json.at("font").get<std::string>();
    // Style
    if (has_value("charsize"))
        fmt.charsize = json.at("charsize").get<int>();
    if (has_value("has_outline"))
        fmt.has_outline = json.at("has_outline").get<bool>();
    if (has_value("outline_size"))
        fmt.outline_size = json.at("outline_size").get<int>();
    if (has_value("has_shadow"))
        fmt.has_shadow = json.at("has_shadow").get<bool>();
    if (has_value("shadow_offset_x"))
        fmt.shadow_offset_x = json.at("shadow_offset_x").get<int>();
    if (has_value("shadow_offset_y"))
        fmt.shadow_offset_y = json.at("shadow_offset_y").get<int>();
    if (has_value("line_spacing"))
        fmt.line_spacing = json.at("line_spacing").get<float>();
    if (has_value("alignment"))
        fmt.alignment = json.at("alignment").get<Alignment>();
    if (has_value("effect"))
        fmt.effect = json.at("effect").get<Effect>();
    if (has_value("effect_offset"))
        fmt.effect_offset = json.at("effect_offset").get<int>();
    // Color
    if (has_value("text_color"))
        fmt.text_color = json.at("text_color").get<Color>();
    if (has_value("outline_color"))
        fmt.outline_color = json.at("outline_color").get<Color>();
    if (has_value("shadow_color"))
        fmt.shadow_color = json.at("shadow_color").get<Color>();
    if (has_value("alpha"))
        fmt.alpha = json.at("alpha").get<uint8_t>();
    // Language
    if (has_value("lng_tag"))
        fmt.lng_tag = json.at("lng_tag").get<std::string>();
    if (has_value("lng_script"))
        fmt.lng_script = json.at("lng_script").get<std::string>();
    if (has_value("lng_direction"))
        fmt.lng_direction = json.at("lng_direction").get<std::string>();
    if (has_value("word_dividers"))
        fmt.word_dividers = strToStr32(json.at("word_dividers").get<std::string>());
    if (has_value("tw_short_pauses"))
        fmt.tw_short_pauses = strToStr32(json.at("tw_short_pauses").get<std::string>());
    if (has_value("tw_long_pauses"))
        fmt.tw_long_pauses = strToStr32(json.at("tw_long_pauses").get<std::string>());
}

static void _parseFmt(std::stack<Format>& fmts, std::u32string const& data)
{
    auto const json = nlohmann::json::parse(data);

    if (json.empty() || json.is_null()) {
        if (fmts.size() > 1)
            fmts.pop();
        return;
    }

    fmts.push(fmts.top());
    Format& fmt = fmts.top();

    jsonToFmt(json, fmt);
}

void Area::parseStringU32(std::u32string const& str) try
{
    history.clear();
    std::vector<TextPart> parts;
    std::stack<Format> fmts;
    fmts.push(_format);
    size_t i = 0;
    while (i != str.size()) {
        size_t const opening_braces = str.find(U"{{", i);
        if (opening_braces == std::string::npos) {
            parts.emplace_back(str.substr(i), fmts.top());
            break;
        }
        else {
            // find closing braces
            size_t const closing_braces = str.find(U"}}", opening_braces + 2);
            if (closing_braces == std::string::npos) {
                parts.emplace_back(str.substr(i), fmts.top());
                break;
            }
            // add part if needed
            size_t diff = opening_braces - i;
            if (diff > 0)
                parts.emplace_back(str.substr(i, diff), fmts.top());

           _parseFmt(fmts, str.substr(opening_braces + 1, closing_braces - opening_braces));

            // skip to next sub strings
            i = closing_braces + 2;
        }
    }

    setTextParts(parts);
}
CATCH_AND_RETHROW_METHOD_EXC;

void Area::parseString(std::string const& str)
{
    parseStringU32(strToStr32(str));
}

void Area::setTextParts(std::vector<TextPart> const& text_parts, bool move_cursor)
{
    if (text_parts.empty()) {
        clear();
        return;
    }
    _buffers.resize(text_parts.size());
    
    for (size_t i = 0; i < text_parts.size(); i++) {
        auto const& part = text_parts.at(i);
        auto& buffer = _buffers.at(i);
        if (!buffer)
            buffer = std::make_unique<_internal::Buffer>(part);
        else
            buffer->set(part);
    }

    int const tmp = static_cast<int>(_glyph_count);
    _updateBufferInfos();
    int const diff = static_cast<int>(_glyph_count) - tmp;
    if (move_cursor) {
        if (_edit_cursor < _locked_cursor)
            _edit_cursor = _locked_cursor;
        _edit_cursor += static_cast<size_t>(diff);
    }
    _lock_selection = false;
    _locked_cursor = _edit_cursor;
    _tw_cursor = std::min(_tw_cursor, static_cast<float>(_glyph_count));
}

std::vector<TextPart> Area::getTextParts() const
{
    std::vector<TextPart> vec;
    vec.insert(vec.cbegin(), _buffer_infos->cbegin(), _buffer_infos->cend());
    return vec;
}

std::u32string Area::getStringU32() const
{
    return _buffer_infos->getString();
}

std::string Area::getString() const
{
    return str32ToStr(getStringU32());
}

static std::string fmtDiff(Format const& parent, Format const& child)
{
    nlohmann::json ret;

    if (parent.font != child.font)
        ret["font"] = child.font;
    if (parent.charsize != child.charsize)
        ret["charsize"] = child.charsize;
    if (parent.has_outline != child.has_outline)
        ret["has_outline"] = child.has_outline;
    if (parent.outline_size != child.outline_size)
        ret["outline_size"] = child.outline_size;
    if (parent.has_shadow != child.has_shadow)
        ret["has_shadow"] = child.has_shadow;
    if (parent.shadow_offset_x != child.shadow_offset_x)
        ret["shadow_offset_x"] = child.shadow_offset_x;
    if (parent.shadow_offset_y != child.shadow_offset_y)
        ret["shadow_offset_y"] = child.shadow_offset_y;
    if (parent.line_spacing != child.line_spacing)
        ret["line_spacing"] = child.line_spacing;
    if (parent.alignment != child.alignment)
        ret["alignment"] = child.alignment;
    if (parent.effect != child.effect)
        ret["effect"] = child.effect;
    if (parent.effect_offset != child.effect_offset)
        ret["effect_offset"] = child.effect_offset;
    if (parent.effect_offset != child.effect_offset)
        ret["effect_offset"] = child.effect_offset;
    if (parent.text_color != child.text_color)
        ret["text_color"] = child.text_color;
    if (parent.outline_color != child.outline_color)
        ret["outline_color"] = child.outline_color;
    if (parent.shadow_color != child.shadow_color)
        ret["shadow_color"] = child.shadow_color;
    if (parent.alpha != child.alpha)
        ret["alpha"] = child.alpha;
    if (parent.lng_tag != child.lng_tag)
        ret["lng_tag"] = child.lng_tag;
    if (parent.lng_script != child.lng_script)
        ret["lng_script"] = child.lng_script;
    if (parent.lng_direction != child.lng_direction)
        ret["lng_direction"] = child.lng_direction;
    if (parent.word_dividers != child.word_dividers)
        ret["word_dividers"] = child.word_dividers;
    if (parent.tw_short_pauses != child.tw_short_pauses)
        ret["tw_short_pauses"] = child.tw_short_pauses;
    if (parent.tw_long_pauses != child.tw_long_pauses)
        ret["tw_long_pauses"] = child.tw_long_pauses;

    return '{' + ret.dump() + '}';
}

std::u32string Area::getUnparsedStringU32() const
{
    std::vector<Format> fmts;
    fmts.emplace_back(_format);
    std::u32string ret;

    for (auto& ptr : _buffers) {
        auto const& buffer = *ptr;
        if (buffer.getString().empty())
            continue;
        Format const buffer_fmt = buffer.getFormat();
        if (buffer_fmt == fmts.back()) {
            ret.append(buffer.getString());
            continue;
        }
        bool done = false;
        for (size_t i = 1; i < fmts.size(); ++i) {
            Format const& fmt = fmts.at(fmts.size() - i - 1);
            if (buffer_fmt == fmt) {
                for (size_t j = 0; j < i; ++j) {
                    ret.append(U"{{}}");
                }
                ret.append(buffer.getString());
                fmts.resize(fmts.size() - i);
                done = true;
                break;
            }
        }
        if (done) continue;

        ret.append(strToStr32(fmtDiff(fmts.back(), buffer_fmt)));
        fmts.emplace_back(buffer_fmt);

        ret.append(buffer.getString());
    }

    return ret;
}

std::string Area::getUnparsedString() const
{
    return str32ToStr(getUnparsedStringU32());
}

void Area::updateAll()
{
    for (Shared area : getInstances()) {
        area->_drawIfNeeded();
        area->_last_update = std::chrono::steady_clock::now();
    }
}

void Area::cancelAll()
{
    for (Shared area : getInstances()) {
        (*area->_processing_pixels)->cancel();
    }
}

void Area::_subjectUpdate(Subject const& subject, int event_id)
{
    bool const resize = (*_current_pixels)->sizeDiff(*(*_processing_pixels));
    _current_pixels = _processing_pixels;
    _notifyObservers(resize ? Event::Resize : Event::Content);
}

void const* Area::pixelsGet() const try
{
    RGBA32::Vector const& pixels = (*_current_pixels)->getPixels();
    if (pixels.empty()) {
        return nullptr;
    }
    // Retrieve cropped dimensions of current pixels
    int w, h;
    (*_current_pixels)->getDimensions(w, h);
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
    _buffers.resize(1);
    _buffers.front()->set(TextPart(U"", _format));
    _updateBufferInfos();
    _edit_cursor = 0;
    _locked_cursor = 0;
    // Reset lines
    _updateLines();
    // Reset typewriter
    _tw_cursor = 0.f;
    // Reset cursor timer
    _edit_timer = std::chrono::nanoseconds(0);
}

void Area::pixelsGetDimensions(int& w, int& h) const noexcept
{
    (*_current_pixels)->getDimensions(w, h);
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
    _wrapping = false;
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
    if (tmp != _scrolling)
        _notifyObservers(Event::Content);
}

void Area::resetFocus()
{
    if (Shared area = getFocused(); area) {
        area->setFocus(false);
        _focused.reset();
    }
}

Area::Shared Area::getFocused() noexcept
{
    return _focused.lock();
}

void Area::setFocusable(bool focusable)
{
    if (_is_focusable == focusable)
        return;
    if (_is_focusable && isFocused()) {
        resetFocus();
    }
    _is_focusable = focusable;
}

void Area::setFocus(bool state)
{
    if (!_is_focusable)
        return;

    // Make this window focused
    if (state) {
        // Unfocus previous window if different
        if (Shared focused = getFocused(); focused && focused != shared_from_this()) {
            focused->setFocus(false);
        }
        _focused = weak_from_this();
        _edit_display_cursor = true;
        _edit_timer = std::chrono::nanoseconds(0);
        _draw = true;
    }
    // Unfocus this window
    else if (isFocused()) {
        _focused.reset();
        _edit_display_cursor = false;
        _draw = true;
    }
}

bool Area::isFocused() const noexcept
{
    Shared focused = getFocused();
    return focused && focused == shared_from_this();
}

void Area::cursorPlace(int x, int y) try
{
    setFocus(true);
    _edit_x = -1;
    if (_glyph_count == 0) {
        return;
    }
    y += _scrolling;

    _internal::Line::cit line = _lines.cbegin();
    FT_Vector pen;
    pen.x = (_buffer_infos->isLTR() ? _margin_v : (_w - _margin_v)) << 6;
    pen.y = _margin_h;
    for (; line != _lines.cend(); ++line) {
        pen.y += line->fullsize;
        if (pen.y > y) break;
    }
    if (line == _lines.cend()) {
        --line;
    }

    bool click_is_ltr = _buffer_infos->isLTR();
    bool i_is_ltr = click_is_ltr;
    pen.x += (line->x_offset(_buffer_infos->isLTR()) << 6) * (i_is_ltr ? 1 : -1);


    auto const select_text = [&]() {
        if (!_lock_selection) {
            _locked_cursor = _edit_cursor;
            lockSelection();
        }
    };
    // Do some magic
    for (size_t i = line->first_glyph; i < line->last_glyph; ++i) {
        // Get glyph info
        _internal::GlyphInfo const& glyph(_buffer_infos->getGlyph(i));

        // Check if text direction changed between previous glyphs
        if ((_buffer_infos->getBuffer(i).fmt.lng_direction == "ltr") != i_is_ltr) {
            // Check that click coordinates aren't on this specific glyph
            int clicked_x = (pen.x + glyph.pos.x_advance) >> 6;
            if (click_is_ltr ? (clicked_x > x) : (clicked_x < x)) {
                _edit_cursor = i;
                select_text();
                return;
            }
            // Replace pen to make up for direction changes
            line->replace_pen(pen, *_buffer_infos, i);
            // Update direction of cursor
            i_is_ltr = !i_is_ltr;
            // Update direction of click area if needed
            clicked_x = (pen.x - glyph.pos.x_advance) >> 6;
            if (click_is_ltr ? (clicked_x > x) : (clicked_x < x)) {
                click_is_ltr = !click_is_ltr;
            }
        }

        // Advance before computing X if RTL
        if (!i_is_ltr)
            pen.x -= glyph.pos.x_advance;
        // Compute X and test if it matches the click coordinates
        int const clicked_x = (pen.x + glyph.pos.x_advance / 2) >> 6;
        if (click_is_ltr ? (clicked_x > x) : (clicked_x < x)) {
            _edit_cursor = i;
            select_text();
            return;
        }
        // Advance after computing X if LTR
        if (i_is_ltr)
            pen.x += glyph.pos.x_advance;
    }
    _edit_cursor = line->last_glyph;
    select_text();
}
CATCH_AND_RETHROW_METHOD_EXC;

void Area::selectAll() noexcept
{
    _edit_cursor = _glyph_count;
    _locked_cursor = 0;
    _edit_x = -1;
    _draw = true;
}

void Area::formatSelection(nlohmann::json const& json)
{
    if (_edit_cursor == _locked_cursor)
        return;
    if (json.empty() || json.is_null())
        return;

    size_t first = std::min(_edit_cursor, _locked_cursor),
           last  = std::max(_edit_cursor, _locked_cursor);

    TextParts parts;

    size_t i = 0;
    for (; i < _buffers.size(); i++) {
        auto const& buffer = *_buffers.at(i);
        parts.emplace_back(buffer.getInfo());

        if (first < buffer.glyphCount()) {
            Format fmt = buffer.getFormat();
            jsonToFmt(json, fmt);
            parts.back().fmt = fmt;
            if (first != 0) {
                first = buffer.getClusterIndex(first);
                std::u32string const head = buffer.getString().substr(0, first),
                                     body = buffer.getString().substr(first);
                parts.back() = TextPart(head, buffer.getFormat());
                parts.emplace_back(body, fmt);
            }
            if (last < buffer.glyphCount()) {
                last = buffer.getClusterIndex(last);
                std::u32string const head = buffer.getString().substr(first, last - first),
                                     body = buffer.getString().substr(last);
                parts.back() = TextPart(head, fmt);
                parts.emplace_back(body, buffer.getFormat());
                break;
            }
            first = 0;
        }
        else
            first -= buffer.glyphCount();
        last -= buffer.glyphCount();
        if (last == 0)
            break;
    }
    if (i < _buffers.size() - 1)
        parts.insert(parts.cend(), _buffer_infos->cbegin() + i + 1, _buffer_infos->cend());

    history.add<AreaCommand>(AreaCommand::Type::Formatting, shared_from_this(), parts);
}

size_t Area::_move_cursor_line(_internal::Line const* line, int x)
{
    if (_edit_x < 0)
        _edit_x = x;
    else
        x = _edit_x;
    size_t cursor = line->first_glyph;
    size_t const glyph_count = _buffer_infos->glyphCount();
    bool line_is_ltr = _buffer_infos->isLTR();
    bool is_ltr = line_is_ltr;
    for (int new_x = (is_ltr ? (_margin_h + line->x_offset(_buffer_infos->isLTR())) :
        (_w - _margin_h - line->x_offset(_buffer_infos->isLTR()))) << 6;
        cursor < line->last_glyph && cursor < glyph_count;
        ++cursor)
    {
        int const offset = _buffer_infos->getGlyph(cursor).pos.x_advance;
        if ((_buffer_infos->getBuffer(cursor).fmt.lng_direction == "ltr") != is_ltr) {
            // Check that click coordinates aren't on this specific glyph
            int clicked_x = (x + offset) >> 6;
            if (line_is_ltr ? (clicked_x > x) : (clicked_x < x)) {
                break;
            }
            FT_Vector pen{ new_x, 0 };
            line->replace_pen(pen, *_buffer_infos, cursor);
            new_x = pen.x;
            is_ltr = !is_ltr;
            clicked_x = (pen.x - offset) >> 6;
            if (line_is_ltr ? (clicked_x > x) : (clicked_x < x)) {
                line_is_ltr = !line_is_ltr;
            }
        }
        if (line_is_ltr ? (((new_x + offset / 2) >> 6) > x) : (((new_x - offset / 2) >> 6) < x)) {
            break;
        }
        new_x += offset * (is_ltr ? 1 : -1);
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
    bool reset_edit_x = true;

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
        reset_edit_x = false;
        if (line == _lines.cend() - 1) break;
        ++line;
        _edit_cursor = _move_cursor_line(&(*line), x);
        break;

    case Move::Up:
        reset_edit_x = false;
        if (line == _lines.cbegin()) break;
        --line;
        _edit_cursor = _move_cursor_line(&(*line), x);
        break;

    case Move::CtrlRight:
        if (_edit_cursor >= _glyph_count) break;
        _edit_cursor = _ctrl_jump(*_buffer_infos, _edit_cursor, 1);
        break;

    case Move::CtrlLeft:
        if (_edit_cursor == 0) break;
        _edit_cursor = _ctrl_jump(*_buffer_infos, _edit_cursor, -1);
        break;

    case Move::Start:
        _edit_cursor = line->first_glyph;
        break;

    case Move::End:
        _edit_cursor = line->last_glyph;
        break;

    }
    if (!_lock_selection)
        _locked_cursor = _edit_cursor;

    _draw = true;
    if (reset_edit_x)
        _edit_x = -1;
}
CATCH_AND_RETHROW_METHOD_EXC;

std::optional<TextParts> Area::_cursorAddText(std::u32string str) const try
{
    if (str.empty()) {
        LOG_OBJ_METHOD_WRN("Empty string.");
        return std::nullopt;
    }
    if (_buffers.empty()) {
        throw_exc("No buffer was given beforehand.");
    }

    TextParts parts;
    if (auto opt_parts = _cursorDeleteText(Delete::Invalid); opt_parts) {
        parts = opt_parts.value();
        if (parts.empty())
            parts.emplace_back(U"", _format);
    }
    else {
        parts = TextParts(getTextParts());
    }

    if (_edit_cursor >= _glyph_count) {
        parts.back().str.append(str);
    }
    else {
        size_t cursor = std::min(_edit_cursor, _locked_cursor);
        for (size_t i = 0; i < parts.size(); ++i) {
            _internal::Buffer& buffer = *_buffers.at(i);
            if (buffer.glyphCount() > cursor) {
                TextPart& part = parts.at(i);
                part.str.insert(part.str.cbegin() + buffer.getClusterIndex(cursor), str.cbegin(), str.cend());
                break;
            }
            cursor -= buffer.glyphCount();
        }
    }
    parts.move_cursor = true;


    return parts;
}
CATCH_AND_RETHROW_METHOD_EXC;

std::tuple<size_t, size_t> Area::_cursorGetInfo() const
{
    if (_locked_cursor < _edit_cursor)
        return std::make_tuple(_locked_cursor, _edit_cursor - _locked_cursor);
    return std::make_tuple(_edit_cursor, _locked_cursor - _edit_cursor);
}

std::tuple<TextParts, TextParts> Area::_splitText(size_t cursor, size_t count) const
{
    TextParts kept, removed;

    size_t i = 0;
    for (; i < _buffers.size(); i++) {
        _internal::Buffer const& buffer = *_buffers.at(i);
        TextPart& part = kept.emplace_back(buffer.getInfo());
        size_t glyph_count = buffer.glyphCount();
        if (glyph_count > cursor) {
            size_t const first = buffer.getClusterIndex(cursor);
            size_t const last = buffer.getClusterIndex(cursor + count);
            auto const fit = part.str.cbegin() + first;
            auto const lit = part.str.cbegin() + last;
            removed.emplace_back(std::u32string(fit, lit), part.fmt);
            part.str.erase(fit, lit);
            count -= last - first;
            glyph_count -= std::min(buffer.glyphCount(), cursor + count) - cursor;
            if (part.str.empty())
                kept.pop_back();
            if (removed.back().str.empty())
                removed.pop_back();
            if (count == 0)
                break;
        }
        cursor -= glyph_count;
    }
    if (i < _buffers.size() - 1)
        kept.insert(kept.cend(), _buffer_infos->cbegin() + i + 1, _buffer_infos->cend());

    return std::make_tuple(std::move(kept), std::move(removed));
}

std::optional<TextParts> Area::_cursorDeleteText(Delete direction) const try
{
    auto [cursor, count] = _cursorGetInfo();
    size_t tmp;

    if (_locked_cursor != _edit_cursor) {
        direction = Delete::Invalid;
    }
    else switch (direction) {
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
        tmp = _ctrl_jump(*_buffer_infos, cursor, 1);
        count = tmp - cursor;
        break;

    case Delete::CtrlLeft:
        if (cursor == 0) break;
        tmp = _ctrl_jump(*_buffer_infos, cursor, -1);
        count = cursor - tmp;
        cursor = tmp;
        break;
    }

    if (count == 0)
        return std::nullopt;

    auto [kept, removed] = _splitText(cursor, count);

    if (direction == Delete::Right || direction == Delete::CtrlRight)
        kept.move_cursor = false;
    else
        kept.move_cursor = true;

    return kept;
}
CATCH_AND_RETHROW_METHOD_EXC;

std::optional<TextParts> Area::_cursorGetText() const
{
    if (_edit_cursor == _locked_cursor) {
        return std::nullopt;
    }

    auto [cursor, count] = _cursorGetInfo();
    auto [unselected, selected] = _splitText(cursor, count);

    return selected;
}


void Area::cursorMove(Move direction)
{
    Shared area = getFocused();
    if (area) {
        area->_cursorMove(direction);
    }
}

void Area::cursorAddText(std::u32string str)
{
    Shared area = getFocused();
    if (area) {
        auto parts = area->_cursorAddText(str);
        if (parts)
            history.add<AreaCommand>(AreaCommand::Type::Paste, area, parts.value());
    }
}

void Area::cursorAddText(std::string str)
{
    cursorAddText(strToStr32(str));
}

void Area::cursorAddChar(char32_t c)
{
    Shared area = getFocused();
    if (area) {
        auto parts = area->_cursorAddText(std::u32string(1, c));
        if (parts)
            history.add<AreaCommand>(AreaCommand::Type::Addition, area, parts.value());
    }
}

void Area::cursorAddChar(char c)
{
    cursorAddChar(static_cast<char32_t>(c));
}

void Area::cursorDeleteText(Delete direction)
{
    Shared area = getFocused();
    if (area) {
        auto parts = area->_cursorDeleteText(direction);
        if (parts)
            history.add<AreaCommand>(AreaCommand::Type::Deletion, area, parts.value());
    }
}

std::u32string Area::cursorGetText()
{
    Shared area = getFocused();
    if (area) {
        auto focused = area->_cursorGetText();
        if (focused) {
            std::u32string str;
            for (auto const& part : focused.value())
                str += part.str;
            return str;
        }
    }
    return std::u32string();
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

    FT_Vector pen;
    pen.y = _margin_h;
    for (_internal::Line::cit it = _lines.cbegin(); it <= line; ++it) {
        pen.y += it->fullsize;
    }

    bool is_ltr = _buffer_infos->isLTR();
    if (is_ltr)
        pen.x = (_margin_v + line->x_offset(_buffer_infos->isLTR())) << 6;
    else
        pen.x = (_w - _margin_v - line->x_offset(_buffer_infos->isLTR())) << 6;
    for (size_t n = line->first_glyph; n < _edit_cursor && n < _glyph_count; ++n) {
        auto const& buffer = _buffer_infos->getBuffer(n);
        if ((buffer.fmt.lng_direction == "ltr") != is_ltr) {
            is_ltr = !is_ltr;
            line->replace_pen(pen, *_buffer_infos, n);
        }
        pen.x += _buffer_infos->getGlyph(n).pos.x_advance * (is_ltr ? 1 : -1);
    }
    x = pen.x >> 6;
    y = pen.y;
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
    if (_wrapping) {
        _w = _margin_v * 2;
        _h = _margin_h * 2;
    }
    else if (_glyph_count > 0 && (_w <= 0 || _h <= 0)) {
        throw_exc("wrapping disabled but width and/or height <= 0");
    }
    // Reset _lines
    _lines.clear();
    _lines.emplace_back();
    _internal::Line::it line = _lines.begin();
    if (_buffer_infos->empty()) {
        line->alignment = _format.alignment;
        line->charsize = _format.charsize;
        line->fullsize = static_cast<int>(static_cast<float>(_format.charsize) * _format.line_spacing);
        return;
    }
    
    Alignment const main_alignment = _buffer_infos->front().fmt.alignment;
    line->alignment = main_alignment;
    size_t cursor = 0;

    size_t last_divider(0);
    int last_divider_x{ 0 };
    FT_Vector pen({ _margin_v << 6, _margin_h << 6 });

    bool add_line = false;
    
    while (cursor < _glyph_count) {
        // Retrieve glyph infos
        _internal::GlyphInfo const& glyph = _buffer_infos->getGlyph(cursor);
        _internal::BufferInfo const& buffer = _buffer_infos->getBuffer(cursor);

        // Add line if needed
        if (add_line) {
            _lines.emplace_back();
            line = _lines.end() - 1;
            line->first_glyph = cursor;
            line->scrolling = (line - 1)->scrolling;
            line->alignment = buffer.fmt.alignment;
            // Reset pen
            pen = { _margin_v << 6, _margin_h << 6 };
            add_line = false;
        }

        // Update sizes
        int const charsize = buffer.fmt.charsize;
        if (line->charsize < charsize) {
            line->charsize = charsize;
        }
        int const fullsize = static_cast<int>(static_cast<float>(charsize) *
            buffer.fmt.line_spacing);
        if (line->fullsize < fullsize) {
            line->fullsize = fullsize;
            line->y_offset = (fullsize - static_cast<int>(1.3f *
                static_cast<float>(charsize))) / 2;
        }
        // If glyph is a word divider, mark it as possible line break
        // Else, update alignment
        if (glyph.is_word_divider) {
            last_divider = cursor;
            last_divider_x = pen.x;
        }
        else if (line->alignment != buffer.fmt.alignment && buffer.fmt.alignment == main_alignment) {
            line->alignment = main_alignment;
        }
        // Update pen position
        if (!glyph.is_new_line) {
            pen.x += glyph.pos.x_advance;
            pen.y += glyph.pos.y_advance;
        }

        // If wrapping mode is disabled and the pen is out of bound, we should line break
        if (glyph.is_new_line
            || (!_wrapping && ((pen.x >> 6) < _margin_v || (pen.x >> 6) >= (_w - _margin_v)))
            || (_wrapping && _max_w && ((pen.x >> 6) < _margin_v || (pen.x >> 6) >= (_max_w - _margin_v)))
        ) {
            // If no word divider was found, hard break the line
            if (last_divider == 0) {
                if (cursor != 0)
                    --cursor;
                line->used_width = (pen.x - glyph.pos.x_advance) >> 6;
            }
            else {
                cursor = last_divider;
                line->used_width = last_divider_x >> 6;
            }

            line->last_glyph = cursor;
            line->scrolling += line->fullsize;
            line->used_width += _margin_v;
            if (_wrapping && _w < line->used_width) {
                _w = line->used_width;
            }
            last_divider = 0;
            last_divider_x = 0;
            add_line = true;
        }
        // Only increment cursor if not a line break
        ++cursor;
    }

    line->last_glyph = cursor;
    // Add line size if empty (for input visibility)
    if (line->first_glyph == line->last_glyph) {
        auto const& buffer = _buffer_infos->getBuffer(cursor);
        line->charsize = buffer.fmt.charsize;
        line->fullsize = static_cast<int>(static_cast<float>(line->charsize) *
            buffer.fmt.line_spacing);
        line->y_offset = (line->fullsize - static_cast<int>(1.3f *
            static_cast<float>(line->charsize))) / 2;
    }
    line->scrolling += line->fullsize;
    line->used_width = (pen.x >> 6) + _margin_v;
    if (_wrapping) {
        if (_w < (line->used_width)) {
            _w = line->used_width;
        }
        ++_w;
    }
    if (_w < _min_w)
        _w = _min_w;
    // Compute unused width of each line
    for (auto& line : _lines) {
        line.unused_width = _w - line.used_width;
        if (_wrapping) {
            _h += static_cast<int>(line.fullsize);
        }
    }
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
    if (!_buffers.empty()) {
        for (auto it = _buffers.cbegin() + 1; it != _buffers.cend(); ) {
            if ((*it)->glyphCount() == 0)
                it = _buffers.erase(it);
            else
                ++it;
        }
        if (_buffers.size() > 1 && _buffers.front()->glyphCount() == 0)
            _buffers.erase(_buffers.cbegin());
    }
    _buffer_infos->update(_buffers);
    _glyph_count = _buffer_infos->glyphCount();
    _updateLines();
}
CATCH_AND_RETHROW_METHOD_EXC;

// Draws current area if _draw is set to true
void Area::_drawIfNeeded()
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    auto const now = steady_clock::now();
    auto const diff = now - _last_update;

    // Determine if TypeWriter is needed
    if (_print_mode == PrintMode::Typewriter && static_cast<size_t>(_tw_cursor) < _glyph_count) {
        
        if (_tw_sleep != 0ns) {
            if (_tw_sleep < diff)
                _tw_sleep = 0ns;
            else
                _tw_sleep -= diff;
        }

        if (_tw_sleep == 0ns) {

            auto const ns_per_char = duration<float>(1) / _tw_cps;
            float new_cursor = _tw_cursor + diff / ns_per_char;
            if (static_cast<size_t>(new_cursor) > _glyph_count) {
                new_cursor = static_cast<float>(_glyph_count);
            }

            for (size_t first = static_cast<size_t>(_tw_cursor) + 1,
                    last = static_cast<size_t>(new_cursor);
                first <= last && first < _glyph_count-1;
                ++first)
            {
                char32_t const c = _buffer_infos->getChar(first);
                auto const& fmt = _buffer_infos->getBuffer(first).fmt;
                for (size_t i = 0;
                    i < fmt.tw_short_pauses.size() && i < fmt.tw_long_pauses.size();
                    i++)
                {
                    char32_t const
                        sh_c = i < fmt.tw_short_pauses.size() ? fmt.tw_short_pauses.at(i) : 0,
                        lo_c = i < fmt.tw_long_pauses.size() ? fmt.tw_long_pauses.at(i) : 0;

                    if ((c == sh_c || c == lo_c)
                        && std::iswspace(static_cast<wchar_t>(_buffer_infos->getChar(first + 1))))
                    {
                        _tw_cursor = static_cast<float>(first + 1);
                        _tw_sleep = ns_per_char * (lo_c == c ? 12 : 6);
                        _draw = true;
                        break;
                    }
                }
            }
        
            if (_tw_sleep == 0ns) {
                _tw_cursor = new_cursor;
                _draw = true;
            }
        }
    }
    // Determine if cursor needs to be drawn
    if (isFocused()) {
        _edit_timer += diff;
        if (_edit_timer >= 500ms) {
            _edit_timer %= 500ms;
            _edit_display_cursor = !_edit_display_cursor;
            _draw = true;
        }
    }
    // Determine if a function needs to be edited
    if (!_draw) {
        for (auto const& buffer : *_buffer_infos) {
            if (buffer.fmt.effect == Effect::Vibrate) {
                if (now - _last_vibrate_update >= 33ms) {
                    _last_vibrate_update = now;
                    _draw = true;
                    break;
                }
            }
            if ((buffer.fmt.effect != Effect::None && buffer.fmt.effect != Effect::Vibrate)
                || buffer.fmt.text_color.func == ColorFunc::Rainbow
                || (buffer.fmt.has_outline && buffer.fmt.outline_color.func == ColorFunc::Rainbow)
                || (buffer.fmt.has_shadow && buffer.fmt.shadow_color.func == ColorFunc::Rainbow))
            {
                _draw = true;
                break;
            }
        }
    }
    // Skip if drawing is not needed
    if (!_draw) {
        return;
    }
    // Skip if a draw call is already running
    if ((*_processing_pixels)->isRunning()) {
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
    if (_locked_cursor != _edit_cursor) {
        data.selected.first = _locked_cursor < _edit_cursor ? _locked_cursor : _edit_cursor;
        data.selected.last = _locked_cursor > _edit_cursor ? _locked_cursor : _edit_cursor;
        data.selected.state = true;
    }
    data.buffer_infos = *_buffer_infos;
    data.lines = _lines;
    data.bg_color = _bg_color;
    // Async draw
    (*_processing_pixels)->run(data);
    _draw = false;
}

SSS_TR_END;