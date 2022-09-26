print("Demo.lua start")

local lorem_ipsum = [[
Lorem ipsum dolor sit amet,\n
consectetur adipiscing elit.\n
Pellentesque vitae velit ante.\n
Suspendisse nulla lacus,\n
tempor sit amet iaculis non,\n
scelerisque\u001F1\u001F[ يرغب في الحب ]\u001F0\u001Fsed est.\n
Aenean pharetra ipsum sit amet sem lobortis,\n
a cursus felis semper.\n
Integer nec tortor ex.\n
Etiam quis consectetur turpis.\n
Proin ultrices bibendum imperdiet.\n
Suspendisse vitae fermentum ante,\n
eget cursus dolor.
]]

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