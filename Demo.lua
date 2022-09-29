print("Demo.lua start")

local lorem_ipsum =
[[Lorem ipsum dolor sit amet,
consectetur adipiscing elit.
Pellentesque vitae velit ante.
Suspendisse nulla lacus,
tempor sit amet iaculis non,
scelerisque{{fmt: 1}} يرغب في الحب {{fmt: 0}}sed est.
Aenean pharetra ipsum sit amet sem lobortis,
a cursus felis semper.
Integer nec tortor ex.
Etiam quis consectetur turpis.
Proin ultrices bibendum imperdiet.
Suspendisse vitae fermentum ante,
eget cursus dolor.]]

local area = TR.Area.get(0)
local fmt = area:getFmt(0)

fmt.font = "opensans/OpenSans[wdth,wght].ttf"
fmt.charsize = 30
fmt.has_outline = true
fmt.has_shadow = true
fmt.outline_size = 1
fmt.alignment = TR.Alignment.Center
fmt.effect_offset = 50
area:setFmt(fmt, 0)

fmt.effect = TR.Effect.Waves
fmt.lng_tag = "ar"
fmt.lng_script = "Arab"
fmt.lng_direction = "rtl"
fmt.font = "LateefRegOT.ttf"
fmt.alignment = TR.Alignment.Right
fmt.shadow_offset.x = -3
fmt.text_color.func = TR.ColorFunc.Rainbow
area:setFmt(fmt, 1)
TR.addFontDir("C:/dev/fonts")

area.string = lorem_ipsum
area.clear_color = RGBA.new(0xFF888888)
area:setMargins(30, 10)
--area.print_mode = TR.PrintMode.Typewriter
area.TW_speed = 42
area.focus = true;

print("Demo.lua end")