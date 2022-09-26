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

print("fmt")

fmt.charsize = 30
fmt.has_outline = true
fmt.has_shadow = true
fmt.outline_size = 1
--fmt.aligmnent = TR.Alignment.Center
fmt.effect = TR.Effect.Waves
fmt.effect_offset = 20
fmt.text_color.func = TR.ColorFunc.Rainbow

print("fmt2")

local fmt2 = TR.Fmt.new()
fmt2.lng_tag = "ar"
fmt2.lng_script = "Arab"
fmt2.lng_direction = "rtl"
fmt2.font = "LateefRegOT.ttf"
--fmt2.aligmnent = TR.Alignment.Right
fmt2.shadow_offset.x = -3
fmt2.effect_offset = -50

print("final settings")

area:setFmt(fmt, 0)
area:setFmt(fmt2, 1)
area.string = lorem_ipsum

print("Demo.lua end")