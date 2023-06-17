#include "Text-Rendering/Area.hpp"
#include "_internal/AreaInternals.hpp"
#include "Text-Rendering/Globals.hpp"
#include <nlohmann/json.hpp>

static void from_json(nlohmann::json const& j, FT_Vector& vec)
{
    if (j.contains("x"))
        j.at("x").get_to(vec.x);
    if (j.contains("y"))
        j.at("y").get_to(vec.y);
}

static void to_json(nlohmann::json& j, FT_Vector const& vec)
{
    j = nlohmann::json{ { "x", vec.x }, { "y", vec.y } };
}

SSS_TR_BEGIN;

static void from_json(nlohmann::json const& j, Color& color)
{
    if (j.is_number_unsigned()) {
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

std::map<uint32_t, Area::Ptr> Area::_instances{};
int Area::_default_margin_h{ 10 };
int Area::_default_margin_v{ 10 };
bool Area::_focused_state{ false };
uint32_t Area::_focused_id{ 0 };

    // --- Constructor, destructor & clear function ---

// Constructor, creates a default Buffer
Area::Area(uint32_t id) try
    :   _id(id),
        _buffer_infos(std::make_unique<_internal::BufferInfoVector>())
{
    for (auto& pixels : _pixels) {
        pixels.reset(new _internal::AreaPixels);
        if (!pixels)
            throw_exc("Couldn't allocate internal data");
    }
    _buffers.push_back(std::make_unique<_internal::Buffer>(Format()));
    _updateBufferInfos();
    if (Log::TR::Areas::query(Log::TR::Areas::get().life_state)) {
        char buff[256];
        sprintf_s(buff, "Created Area #%u.", _id);
        LOG_TR_MSG(buff);
    }
}
CATCH_AND_RETHROW_METHOD_EXC;

// Destructor, clears out buffer cache.
Area::~Area() noexcept
{
    if (Log::TR::Areas::query(Log::TR::Areas::get().life_state)) {
        char buff[256];
        sprintf_s(buff, "Deleted Area #%u.", _id);
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

Area& Area::create(uint32_t id) try
{
    Ptr& area = _instances[id];
    area.reset(new Area(id));
    return *area;
}
CATCH_AND_RETHROW_FUNC_EXC;

Area& Area::create()
{
    uint32_t id = 0;
    // Increment ID until no similar value is found
    while (_instances.count(id) != 0) {
        ++id;
    }
    return create(id);
}

Area& Area::create(int width, int height)
{
    Area& area = create();
    area.setDimensions(width, height);
    return area;
}

Area& Area::create(std::u32string const& str, Format fmt)
{
    Area& area = create();
    area.setFormat(fmt);
    area.parseStringU32(str);
    return area;
}

Area& Area::create(std::string const& str, Format fmt)
{
    return create(strToStr32(str), fmt);
}

void Area::remove(uint32_t id) try
{
    if (_instances.count(id) != 0) {
        _instances.erase(id);
    }
}
CATCH_AND_RETHROW_FUNC_EXC

void Area::clearAll() noexcept
{
    _instances.clear();
}

Area* Area::get(uint32_t id) noexcept
{
    if (_instances.count(id) == 0) {
        return nullptr;
    }
    return _instances.at(id).get();
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

void Area::_parseFmt(std::stack<Format>& fmts, std::u32string const& data)
{
    auto const json = nlohmann::json::parse(data);

    if (json.empty() || json.is_null()) {
        if (fmts.size() > 1)
            fmts.pop();
        return;
    }

    fmts.push(fmts.top());
    Format& fmt = fmts.top();

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
    if (has_value("shadow_offset"))
        fmt.shadow_offset = json.at("shadow_offset").get<FT_Vector>();
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
        fmt.alpha = json.at("alpha").get<float>();
    // Language
    if (has_value("lng_tag"))
        fmt.lng_tag = json.at("lng_tag").get<std::string>();
    if (has_value("lng_script"))
        fmt.lng_script = json.at("lng_script").get<std::string>();
    if (has_value("lng_direction"))
        fmt.lng_direction = json.at("lng_direction").get<std::string>();
    if (has_value("word_dividers"))
        fmt.word_dividers = json.at("word_dividers").get<std::u32string>();
}

void Area::parseStringU32(std::u32string const& str) try
{
    auto const new_buffer = [&](bool& is_first, Format const& opt, std::u32string const& str) {
        if (is_first)
            _buffers.back()->changeFormat(opt);
        else
            _buffers.push_back(std::make_unique<_internal::Buffer>(opt));
        _buffers.back()->changeString(str);
        is_first = false;
    };
    clear();
    bool is_first = true;
    std::stack<Format> fmts;
    fmts.push(_format);
    size_t i = 0;
    while (i != str.size()) {
        size_t const opening_braces = str.find(U"{{", i);
        if (opening_braces == std::string::npos) {
            // add buffer and load sub string
            new_buffer(is_first, fmts.top(), str.substr(i));
            break;
        }
        else {
            // find closing braces
            size_t const closing_braces = str.find(U"}}", opening_braces + 2);
            if (closing_braces == std::string::npos) {
                new_buffer(is_first, fmts.top(), str.substr(i));
                break;
            }
            // add buffer if needed
            size_t diff = opening_braces - i;
            if (diff > 0)
                new_buffer(is_first, fmts.top(), str.substr(i, diff));

           _parseFmt(fmts, str.substr(opening_braces + 1, closing_braces - opening_braces));

            // skip to next sub strings
            i = closing_braces + 2;
        }
    }
    size_t tmp = _glyph_count;
    _updateBufferInfos();
    _edit_cursor += _glyph_count - tmp;
    if (!_lock_selection)
        _locked_cursor = _edit_cursor;
}
CATCH_AND_RETHROW_METHOD_EXC;

void Area::parseString(std::string const& str)
{
    parseStringU32(strToStr32(str));
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
    if (parent.shadow_offset != child.shadow_offset)
        ret["shadow_offset"] = child.shadow_offset;
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

    return '{' + ret.dump() + '}';
}

std::string Area::getUnparsedString() const
{
    std::vector<Format> fmts;
    fmts.emplace_back(_format);
    std::string ret;

    for (auto& ptr : _buffers) {
        auto const& buffer = *ptr;
        if (buffer.getString().empty())
            continue;
        Format const buffer_fmt = buffer.getFormat();
        if (buffer_fmt == fmts.back()) {
            ret.append(str32ToStr(buffer.getString()));
            continue;
        }
        bool done = false;
        for (size_t i = 1; i < fmts.size(); ++i) {
            Format const& fmt = fmts.at(fmts.size() - i - 1);
            if (buffer_fmt == fmt) {
                for (size_t j = 0; j < i; ++j) {
                    ret.append("{{}}");
                }
                ret.append(str32ToStr(buffer.getString()));
                done = true;
                break;
            }
        }
        if (done) continue;

        ret.append(fmtDiff(fmts.back(), buffer_fmt));
        fmts.emplace_back(buffer_fmt);

        ret.append(str32ToStr(buffer.getString()));
    }

    return ret;
}

void Area::updateAll()
{
    for (auto const& pair : _instances) {
        if (pair.second)
            pair.second->update();
    }
}

void Area::notifyAll()
{
    for (auto const& pair : _instances) {
        if (pair.second && pair.second->pixelsWereChanged())
            pair.second->pixelsAreRetrieved();
    }
}

void Area::update()
{
    if ((*_processing_pixels)->isPending()) {
        _current_pixels = _processing_pixels;
        (*_processing_pixels)->setAsHandled();
        _changes_pending = true;
    }
    _drawIfNeeded();
    _last_update = std::chrono::steady_clock::now();
}

bool Area::hasRunningThread() const noexcept
{
    return (*_processing_pixels)->isRunning();
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
    _buffers.front()->changeString(U"");
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
    if (!_changes_pending) {
        _changes_pending = tmp != _scrolling;
    }
}

void Area::resetFocus()
{
    _focused_state = false;
    if (_instances.count(_focused_id) != 0) {
        _instances.at(_focused_id)->setFocus(false);
    }
}

Area* Area::getFocused() noexcept
{
    if (_focused_state && _instances.count(_focused_id) != 0) {
        return _instances.at(_focused_id).get();
    }
    return nullptr;
}

void Area::setFocusable(bool focusable)
{
    if (_is_focusable == focusable)
        return;
    if (_is_focusable && _focused_id == _id) {
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
        if (_focused_id != _id && _instances.count(_focused_id) != 0) {
            _instances.at(_focused_id)->setFocus(false);
        }
        _focused_id = _id;
        _focused_state = true;
        _edit_display_cursor = true;
        _edit_timer = std::chrono::nanoseconds(0);
        _draw = true;
    }
    // Unfocus this window
    else if (_focused_id == _id) {
        _focused_state = false;
        _edit_display_cursor = false;
        _draw = true;
    }
}

bool Area::isFocused() const noexcept
{
    return _focused_state && _focused_id == _id;
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
    _edit_display_cursor = true;
    _edit_timer = std::chrono::nanoseconds(0);
    if (reset_edit_x)
        _edit_x = -1;
}
CATCH_AND_RETHROW_METHOD_EXC;

void Area::_cursorAddText(std::u32string str) try
{
    if (str.empty()) {
        LOG_OBJ_METHOD_WRN("Empty string.");
        return;
    }
    if (_buffers.empty()) {
        throw_exc("No buffer was given beforehand.");
    }
    if (_locked_cursor != _edit_cursor) {
        _cursorDeleteText(Delete::Invalid);
    }
    size_t cursor = _edit_cursor;
    size_t size = 0;
    if (cursor >= _glyph_count) {
        _internal::Buffer& buffer = *_buffers.back();
        size_t const tmp = buffer.glyphCount();
        buffer.insertText(str, buffer.glyphCount());
        size = buffer.glyphCount() - tmp;
    }
    else {
        for (_internal::Buffer::Ptr const& ptr : _buffers) {
            _internal::Buffer& buffer = *ptr;
            if (buffer.glyphCount() > cursor) {
                size_t const tmp = buffer.glyphCount();
                buffer.insertText(str, cursor);
                size = buffer.glyphCount() - tmp;
                break;
            }
            cursor -= buffer.glyphCount();
        }
    }
    // Update lines as they need to be updated before moving cursor
    _updateBufferInfos();
    // Move cursors
    if (static_cast<size_t>(_tw_cursor) < _edit_cursor) {
        _tw_cursor += static_cast<float>(size);
    }
    _edit_cursor += size;
    _locked_cursor = _edit_cursor;
    _edit_display_cursor = true;
    _edit_timer = std::chrono::nanoseconds(0);
}
CATCH_AND_RETHROW_METHOD_EXC;

void Area::_cursorDeleteText(Delete direction) try
{
    size_t cursor = _edit_cursor;
    size_t count = 0;
    if (cursor > _glyph_count) {
        cursor = _glyph_count;
    }

    size_t tmp;
    if (_locked_cursor != _edit_cursor) {
        if (_locked_cursor < _edit_cursor) {
            count = _edit_cursor - _locked_cursor;
            _edit_cursor = _locked_cursor;
            cursor = _edit_cursor;
        }
        else {
            count = _locked_cursor - _edit_cursor;
        }
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
        return;

    size_t const size = count;
    for (_internal::Buffer::Ptr const& ptr : _buffers) {
        _internal::Buffer& buffer = *ptr;
        if (size_t const tmp = buffer.glyphCount(); tmp > cursor) {
            buffer.deleteText(cursor, count);
            count -= tmp - buffer.glyphCount();
            if (count == 0)
                break;
        }
        cursor -= buffer.glyphCount();
    }
    _updateBufferInfos();
    if (direction == Delete::Left || direction == Delete::CtrlLeft) {
        // Move cursor
        _edit_cursor -= size;
    }
    _locked_cursor = _edit_cursor;
    _edit_display_cursor = true;
    _edit_timer = std::chrono::nanoseconds(0);
}
CATCH_AND_RETHROW_METHOD_EXC;

void Area::cursorMove(Move direction)
{
    Area* area = getFocused();
    if (area) {
        area->_cursorMove(direction);
    }
}

void Area::cursorAddText(std::u32string str)
{
    Area* area = getFocused();
    if (area) {
        area->_cursorAddText(str);
    }
}

void Area::cursorAddText(std::string str)
{
    cursorAddText(strToStr32(str));
}

void Area::cursorDeleteText(Delete direction)
{
    Area* area = getFocused();
    if (area) {
        area->_cursorDeleteText(direction);
    }
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
    pen.y = _margin_h + _lines.cbegin()->charsize * 4 / 3;
    for (_internal::Line::cit it = _lines.cbegin() + 1; it <= line; ++it) {
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
    if (!_buffer_infos->empty()) {
        line->alignment = _buffer_infos->front().fmt.alignment;
    }
    size_t cursor = 0;

    size_t last_divider(0);
    int last_divider_x{ 0 };
    FT_Vector pen({ _margin_v << 6, _margin_h << 6 });
    
    while (cursor < _glyph_count) {
        // Retrieve glyph infos
        _internal::GlyphInfo const& glyph = _buffer_infos->getGlyph(cursor);
        _internal::BufferInfo const& buffer = _buffer_infos->getBuffer(cursor);

        // Update sizes
        int const charsize = buffer.fmt.charsize;
        if (line->charsize < charsize) {
            line->charsize = charsize;
        }
        int const fullsize = static_cast<int>(static_cast<float>(charsize) * buffer.fmt.line_spacing);
        if (line->fullsize < fullsize) {
            line->fullsize = fullsize;
        }
        // If glyph is a word divider, mark it as possible line break
        if (glyph.is_word_divider) {
            last_divider = cursor;
            last_divider_x = pen.x;
        }
        // Update pen position
        if (!glyph.is_new_line) {
            pen.x += glyph.pos.x_advance;
            pen.y += glyph.pos.y_advance;
        }

        // If wrapping mode is disabled and the pen is out of bound, we should line break
        if (glyph.is_new_line ||
            (!_wrapping && ((pen.x >> 6) < _margin_v || (pen.x >> 6) >= (_w - _margin_v))))
        {
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
            if (glyph.is_new_line)
                line->unused_width = _w - _margin_v - (pen.x >> 6);

            line->last_glyph = cursor;
            line->scrolling += line->fullsize;
            if (_wrapping && _w < (line->used_width + _margin_v)) {
                _w = line->used_width + _margin_v;
            }
            last_divider = 0;
            last_divider_x = 0;
            // Add line if needed
            _lines.emplace_back();
            line = _lines.end() - 1;
            line->first_glyph = cursor + 1;
            line->scrolling = (line - 1)->scrolling;
            line->alignment = buffer.fmt.alignment;
            // Reset pen
            pen = { _margin_v << 6, _margin_h << 6 };
        }
        // Only increment cursor if not a line break
        ++cursor;
    }

    line->last_glyph = cursor;
    // Add line size if empty (for input visibility)
    if (line->first_glyph == line->last_glyph) {
        auto const& buffer = _buffer_infos->getBuffer(cursor);
        line->charsize = buffer.fmt.charsize;
        line->fullsize = static_cast<int>(static_cast<float>(line->charsize)
            * buffer.fmt.line_spacing);
    }
    line->scrolling += line->fullsize;
    line->used_width = pen.x >> 6;
    if (_wrapping) {
        if (_w < (line->used_width + _margin_v)) {
            _w = line->used_width + _margin_v;
        }
        ++_w;
    }
    // Compute unused width of each line
    for (auto& line : _lines) {
        line.unused_width = _w - _margin_v - line.used_width;
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
    _buffer_infos->update(_buffers);
    _glyph_count = _buffer_infos->glyphCount();
    _updateLines();
}
CATCH_AND_RETHROW_METHOD_EXC;

// Draws current area if _draw is set to true
void Area::_drawIfNeeded()
{
    auto const now = std::chrono::steady_clock::now();
    auto const diff = now - _last_update;

    // Determine if TypeWriter is needed
    if (_print_mode == PrintMode::Typewriter && static_cast<size_t>(_tw_cursor) < _glyph_count) {
        auto const ns_per_char = std::chrono::duration<float>(1) / _tw_cps;
        _tw_cursor += diff / ns_per_char;
        if (static_cast<size_t>(_tw_cursor) > _glyph_count) {
            _tw_cursor = static_cast<float>(_glyph_count);
        }
        _draw = true;
    }
    // Determine if cursor needs to be drawn
    if (isFocused()) {
        _edit_timer += diff;
        using namespace std::chrono_literals;
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
                using namespace std::chrono_literals;
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