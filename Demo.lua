print("Demo.lua start")

local lorem_ipsum =
[[Lorem ipsum dolor sit amet,
consectetur adipiscing elit.
Pellentesque vitae velit ante.
Suspendisse nulla lacus,
tempor sit amet iaculis non,
scelerisque{{
    "text_color": "Rainbow",
    "effect": "Waves",
    "alignment": "Right",
    "shadow_offset": {"x": -3},
    "lng_tag": "ar",
    "lng_script": "Arab",
    "lng_direction": "rtl"
}} يرغب في الحب {{}}sed est.
Aenean pharetra ipsum sit amet sem lobortis,
a cursus felis semper.
Integer nec tortor ex.
Etiam quis consectetur turpis.
Proin ultrices bibendum imperdiet.
Suspendisse vitae fermentum ante,
eget cursus dolor.]]

TR.addFontDir("C:/dev/fonts")

--TR.default_fmt.font = "opensans/OpenSans[wdth,wght].ttf"
TR.default_fmt.charsize = 30
TR.default_fmt.has_outline = true
TR.default_fmt.outline_size = 1
TR.default_fmt.alignment = TR.Alignment.Center
TR.default_fmt.effect_offset = 50

area = TR.Area.new()
local fmt = area:getFmt()
fmt.has_shadow = true
area:setFmt(fmt)

area.margin_h = 30
area.string = lorem_ipsum
area.clear_color = RGBA.new(0xFF888888)
area.wrapping = true
--area.print_mode = TR.PrintMode.Typewriter
area.TW_speed = 42
area.focusable = true;
area.focus = true;


print("Demo.lua end")