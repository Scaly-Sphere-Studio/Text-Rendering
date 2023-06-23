#ifndef SSS_TR_FORMAT_HPP
#define SSS_TR_FORMAT_HPP

#include "_includes.hpp"

/** @file
 *  Defines SSS::TR::Format and subsequent classes.
 */

SSS_TR_BEGIN;

/** */
enum class Alignment {
    Invalid = -1,
    Left,
    Center,
    Right
};

/** */
enum class Effect {
    Invalid = -1,
    None,
    Vibrate,
    Waves,
    FadingWaves,
};

/** Used in Color to determine the color at runtime.*/
enum class ColorFunc {
    Invalid = -1,
    None,           /**< No function, the Config::plain color is used.*/
    Rainbow,        /**< The color is determined by width ratio and time.*/
    RainbowFixed,   /**< Same as #rainbow, but not time-based.*/
};

struct SSS_TR_API Color : public RGB24 {
    using RGB24::RGB24;
    ColorFunc func{ ColorFunc::None };
    bool operator==(Color const& color) const;
};

// Ignore warning about STL exports as they're private members
#pragma warning(push, 2)
#pragma warning(disable: 4251)
#pragma warning(disable: 4275)

/** Structure defining text formats to be used in Area instances.
 *  @sa Color, ColorFunc, Alignment, Effect.
 */
struct SSS_TR_API Format {
    Format() noexcept;
    bool operator==(Format const&) const = default;

    // --- FONT ---

    /** Font file name (can be initialised before processing, but not mandatory).
     *  @default \c "arial.ttf"
     *  @sa loadFont(), addFontDir().
     */
    std::string font{ "arial.ttf" };

    // --- STYLE ---
    
    /** Font size, in pt.
     *  @default \c 12
     */
    int charsize{ 12 };
    /** Whether the text has an outline.
     *  @default \c false
     *  @sa #outline_size.
     */
    bool has_outline{ false };
    /** Outline size in pixels, if any.
     *  @default \c 2
     *  @sa #has_outline.
     */
    int outline_size{ 2 };
    /** Whether the text has a shadow.
     *  @default \c false
     */
    bool has_shadow{ false };
    /** Shadow offset, if any.
     *  @default <tt>(3, 3)</tt>
     *  @sa #has_shadow.
     */
    int shadow_offset_x{ 3 }, shadow_offset_y{ 3 };
    /** Spacing between lines.
     *  @default \c 1.5
     */
    float line_spacing{ 1.5f };
    /** */
    Alignment alignment{ Alignment::Left };
    /** */
    Effect effect{ Effect::None };
    /** */
    int effect_offset{ 4 };

    // --- COLOR ---
    
    /** Text color.
     *  @default \c 0xFFFFFF <em>(plain white)</em>
     *  @sa text_color_func
     */
    Color text_color{ 0xFFFFFF };
    /** Text outline color, if any.
     *  @default \c 0x000000 <em>(plain black)</em>
     *  @sa has_outline, text_color_func
     */
    Color outline_color{ 0x000000 };
    /** Text shadow color, if any.
     *  @default \c 0x444444 <em>(plain gray)</em>
     *  @sa has_shadow, shadow_color_func
     */
    Color shadow_color{ 0x444444 };
    /** Text opacity.
     *  @default \c 255 <em>(fully opaque)</em>
     */
    uint8_t alpha{ 255 };

    // --- LANGUAGE ---
    
    /** BCP 47 language tag, used in
    *  [Harfbuzz](https://harfbuzz.github.io/harfbuzz-hb-common.html#hb-language-t).
    *  @default \c "en"
    */
    std::string lng_tag{ "en" };
    /** ISO 15924 script, used in 
     *  [Harfbuzz](https://harfbuzz.github.io/harfbuzz-hb-common.html#hb-script-t).
     *  @default \c "Latn"
     */
    std::string lng_script{ "Latn" };
    /** Writing direction, used in
     *  [Harfbuzz](https://harfbuzz.github.io/harfbuzz-hb-common.html#hb-direction-t).
     *  @default \c "ltr"
     */
    std::string lng_direction{ "ltr" };
    /** [Word dividers](https://en.wikipedia.org/wiki/Word_divider),
     *  a blank space in most cases, but not always.
     *  Stored in a UTF32 string acting as a UTF32 vector.\n
     *  @default \c U" " <em>(blank space)</em>
     */
    std::u32string word_dividers{ U" " };
};

#pragma warning(pop)

SSS_TR_API extern Format default_fmt;

SSS_TR_END;

#endif // SSS_TR_FORMAT_HPP