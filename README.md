<p align="center">
<img src="webp/demo.webp">
</p>

# Text-Rendering

Lightweight, versatile text rendering utilities for games and applications.
--- 

## Overview

This library provides Unicode-aware text layout and rendering with support for outlining, shadows, inline formatting, typing effects, RTL scripts and language/script hints.

Inline formatting within strings uses JSON objects wrapped in `{{...}}`. An empty `{{}}` resets to the area's base format.

```cpp
area->parseString(
    R"({{"effect":"Waves","effect_offset":50}}Versatile text{{}} )"
    R"(and {{"outline_size":3}}outlined{{}})"
);
```

## Requirements

- Windows (development tested with Visual Studio)
- A C++ toolchain (MSVC) and CMake/Visual Studio solution in the repo
- vcpkg overlay ports are supported via the provided scripts

See the [vcpkg_scripts](vcpkg_scripts) folder for helper install scripts.

## Demo

- Primary demo script: [Demo.lua](Demo.lua)
- Native demo runner: [src/DemoMain/DemoMain.cpp](src/DemoMain/DemoMain.cpp)

The demo shows how to configure default formats, load font directories, create `Area` instances and use inline JSON tags to change formatting on-the-fly.

Basic Lua usage (from Demo.lua):

```lua
TR.addFontDir("C:/dev/fonts")
TR.default_fmt.font = "CALIBRI.TTF"
TR.default_fmt.charsize = 30
TR.default_fmt.has_outline = true
TR.default_fmt.has_shadow = true

area = TR.Area.new(1230, 680)
area.wrapping = true
area.string = "Hello, world!"
```

Inline formatting within strings uses JSON objects wrapped in `{{...}}`. Example:

```lua
area.string = '{{"effect":"Waves","effect_offset":50}}Versatile text{{}} and {{"outline_size":3}}outlined{{}}'
```


## `Format` reference

`Format` describes text appearance and layout. See [inc/Text-Rendering/Format.hpp](inc/Text-Rendering/Format.hpp) for full definitions.

A global `SSS::TR::default_fmt` instance exists; modify it to change defaults for all subsequently created areas.

### Font

| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `font` | `string` | `"arial.ttf"` | Font filename, resolved via `addFontDir()` |

### Style

| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `charsize` | `int` | `12` | Font size in points |
| `has_outline` | `bool` | `false` | Enable outline rendering |
| `outline_size` | `int` | `2` | Outline thickness in pixels (requires `has_outline`) |
| `has_shadow` | `bool` | `false` | Enable drop shadow |
| `shadow_offset_x` | `int` | `3` | Horizontal shadow offset in pixels (requires `has_shadow`) |
| `shadow_offset_y` | `int` | `3` | Vertical shadow offset in pixels (requires `has_shadow`) |
| `line_spacing` | `float` | `1.5` | Line spacing multiplier |
| `alignment` | `Alignment` | `Left` | `Left`, `Center`, or `Right` |
| `effect` | `Effect` | `None` | Animated effect — see [Text Effects](#text-effects) |
| `effect_offset` | `int` | `4` | Effect magnitude; meaning depends on effect type. `0` does not disable the effect — use `effect = None` for that |
| `effect_speed` | `int` | `4` | Milliseconds between animation frame updates; larger = slower |

### Color

Each color field is of type `Color`, which extends `RGB24` and adds a `ColorFunc` for animated coloring.

| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `text_color` | `Color` | `0xFFFFFF` (white) | Main glyph color |
| `outline_color` | `Color` | `0x000000` (black) | Outline color (requires `has_outline`) |
| `shadow_color` | `Color` | `0x444444` (gray) | Shadow color (requires `has_shadow`) |
| `alpha` | `uint8_t` | `255` | Global text opacity — `0` = transparent, `255` = fully opaque |
| `clear_color` | `Color` | `0x00000000` (transparent) | Background fill drawn behind the text |

`ColorFunc` values (set per `Color` field via its `.func` member):

| Value | Behavior |
|-------|----------|
| `None` | Use the color's plain RGB value |
| `Rainbow` | Hue cycles across the text width and over time |
| `RainbowFixed` | Hue cycles across the text width, not time-based |

```cpp
SSS::TR::Format fmt;
fmt.text_color.func = SSS::TR::ColorFunc::Rainbow;
```

### Language & shaping

| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `lng_tag` | `string` | `"en"` | BCP-47 language tag (passed to HarfBuzz) |
| `lng_script` | `string` | `"Latn"` | ISO 15924 script (passed to HarfBuzz) |
| `lng_direction` | `string` | `"ltr"` | `"ltr"` or `"rtl"` |
| `word_dividers` | `u32string` | `U" "` | Characters treated as word boundaries |
| `tw_short_pauses` | `u32string` | `U",;:"` | Typewriter short-pause characters |
| `tw_long_pauses` | `u32string` | `U".!?"` | Typewriter long-pause characters |

## Text Effects

Set via `fmt.effect` or the inline `"effect"` JSON key.

> **Note:** setting `effect_offset = 0` disable the animation

| Effect | Description | `effect_offset` meaning |
|--------|-------------|------------------------|
| `None` | Static text | — |
| `Vibrate` | Small randomized per-character displacement each frame | Displacement radius (px) |
| `Waves` | Sinusoidal vertical displacement across characters | Wave amplitude (px) |
| `FadingWaves` | Waves combined with per-character alpha modulation | Wave amplitude (px) |

### Effects options 

| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `effect_offset` | `int` | `4` | Amplitude of the effect in px |
| `effect_speed` | `int` | `50` | time in ms between animation frame, longer is slower. Doesn't affect the `Vibrate` effect |


```cpp
// Global effect on all areas
SSS::TR::default_fmt.effect        = SSS::TR::Effect::Waves;
SSS::TR::default_fmt.effect_offset = 8;
SSS::TR::default_fmt.effect_speed  = 30; // ms between frames — larger is slower

// Inline effect on a substring only
area->parseString(R"({{"effect":"FadingWaves","effect_offset":12}}Hello!{{}})");
```

## Inline Formatting Quick Reference

Inside `parseString()`, any `{{...}}` block accepts JSON keys matching `Format` field names. An empty `{{}}` resets back to the area's base format.

| JSON key | Accepted values | Example |
|----------|-----------------|---------|
| `"charsize"` | integer | `{{"charsize":32}}` |
| `"has_outline"` | `true` / `false` | `{{"has_outline":true}}` |
| `"outline_size"` | integer | `{{"outline_size":4}}` |
| `"has_shadow"` | `true` / `false` | `{{"has_shadow":true}}` |
| `"effect"` | `"None"` `"Vibrate"` `"Waves"` `"FadingWaves"` | `{{"effect":"Waves"}}` |
| `"effect_offset"` | integer | `{{"effect_offset":8}}` |
| `"alignment"` | `"Left"` `"Center"` `"Right"` | `{{"alignment":"Center"}}` |
| `"font"` | font filename string | `{{"font":"impact.ttf"}}` |

```cpp
area->parseString(
    R"({{"charsize":32,"effect":"Waves"}}BIG WAVY{{}} )"
    R"({{"has_outline":true,"outline_size":4}}outlined{{}} )"
    R"({{"has_shadow":true,"shadow_offset_x":5}}shadowed{{}})"
);
```


## Using the Demo to Explore Effects

1. Build and run the demo as described above.
2. Open [Demo.lua](Demo.lua) and tweak `TR.default_fmt` and `area` properties.
3. Modify inline formatting tags in `demo_str` to try `effect`, `outline_size`, `font`, `has_shadow`, and color overrides.

## Contributing

Feel free to open issues or submit PRs. See the code under `src/` and the public headers under `inc/Text-Rendering/` for implementation details.

----

Screenshot / animated demo: above.

## Title example

```json
{{"effect":"Waves", "effect_offset": 50}}Versatile,{{}} {{"effect":"Vibrate", "effect_offset": 1, "font": "CALIBRIB.TTF", "outline_size": 3}}robust,{{}} and {{"outline_size": 1}}{{"font": "SEGOEPR.TTF", "has_shadow": true}}optimised
{{}}{{"font": "INKFREE.TTF", "outline_size": 4, "charsize": 60}} Text Rendering {{}}
{{"line_spacing": 1.25}}for video games and applications!
```