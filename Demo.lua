print("Demo.lua start")

local lorem_ipsum =
[[Lorem ipsum dolor sit amet,
consectetur adipiscing elit.
Pellentesque vitae velit ante.
Suspendisse nulla lacus,
tempor sit amet iaculis non,
scelerisque.{{
    "charsize": 50,
    "text_color": "Rainbow",
    "effect": "",
    "alignment": "Right",
    "shadow_offset": {"x": -3},
    "lng_tag": "ar",
    "lng_script": "Arab",
    "lng_direction": "rtl"
}} يرغب في الحب {{}} sed est.
Aenean pharetra ipsum sit amet sem lobortis,
a cursus felis semper.
Integer nec tortor ex.
Etiam quis consectetur turpis.
Proin ultrices bibendum imperdiet.
Suspendisse vitae fermentum ante,
eget cursus dolor.]]

local demo_str = 
[[{{"effect":"Waves", "effect_offset": 50}}Versatile,{{}} {{"effect":"Vibrate", "effect_offset": 1, "font": "CALIBRIB.TTF", "outline_size": 3}}robust,{{}} and {{"outline_size": 1}}{{"font": "SEGOEPR.TTF", "has_shadow": true}}optimised
{{}}{{"font": "INKFREE.TTF", "outline_size": 4, "charsize": 60}} Text Rendering {{}}
{{"line_spacing": 1.25}}for video games and applications!]]

TR.addFontDir("C:/dev/fonts")

TR.default_fmt.font = "CALIBRI.TTF"
TR.default_fmt.charsize = 30
TR.default_fmt.has_outline = true
TR.default_fmt.has_shadow = true
TR.default_fmt.outline_size = 2
TR.default_fmt.line_spacing = 1.5
TR.default_fmt.outline_color.rgb = RGB.new(212, 164, 159).rgb
TR.default_fmt.text_color.rgb = RGB.new(230, 192, 188).rgb
TR.default_fmt.shadow_color.rgb = RGB.new(194, 121, 114).rgb
TR.default_fmt.alignment = TR.Alignment.Center
--TR.default_fmt.effect = TR.Effect.Waves
--TR.default_fmt.effect_offset = 50
--TR.default_fmt.tw_short_pauses = ""

area = TR.Area.new(1230, 680)

area.margin_h = 0
area.clear_color = RGBA.new(244, 234, 233, 255)
area.wrapping = true
--area.print_mode = TR.PrintMode.Typewriter
area.TW_speed = 20
area.focusable = true;
area.focus = true;
area.string = lorem_ipsum
area.wrapping = false


print("Demo.lua end")