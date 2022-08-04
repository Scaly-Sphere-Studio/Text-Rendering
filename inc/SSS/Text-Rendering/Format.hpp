#pragma once

#include "_internal/_includes.hpp"

/** @file
 *  Defines SSS::TR::Format and subsequent classes.
 */

SSS_TR_BEGIN;

/** Structure defining text formats to be used in Area instances.
 *  @sa Style, Color, Language.
 */
struct Format {
    /** Sets the style of corresponding text.*/
    struct Style {
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
        FT_Vector shadow_offset{ 3, 3 };
        /** Spacing between lines.
         *  @default \c 1.5
         */
        float line_spacing{ 1.5f };
        /** */
        enum class Alignment {
            Left,
            Center,
            Right
        };
        /** */
        Alignment aligmnent{ Alignment::Left };
        /** */
        enum class Effect {
            None,
            Waves,
            //Vibrate,
            //Rotate,
        };
        /** */
        Effect effect{ Effect::None };
        /** */
        int effect_offset{ 4 };
    };

    /** Sets the color of corresponding text (& style features).
     *  The color can be plain, or use one of the defined functions in \c Func.
     */
    struct Color {
        /** Used in Config to determine the color at runtime.*/
        enum class Func {
            none,           /**< No function, the Config::plain color is used.*/
            rainbow,        /**< The color is determined by width ratio and time.*/
            rainbowFixed,   /**< Same as #rainbow, but not time-based.*/
        };
        /** Stores a "final color" in a single struct,
         *  whether it's plain or computed from a function.
         *  @default Plain white
         */
        struct Config {
            /** The plain color to be used if \c #func is set to \c Func::none.
             *  @default \c 0xFFFFFF <em>(plain white)</em>
             */
            RGB24 plain{ 0xFFFFFF };
            /** The way to determine the color of the text.
             *  @default Func::none
             */
            Func func{ Func::none };
        };
        /** Text color.
         *  @default \c 0xFFFFFF <em>(plain white)</em>
         *  @sa Config
         */
        Config text{ 0xFFFFFF };
        /** Text outline color, if any.
         *  @default \c 0x000000 <em>(plain black)</em>
         *  @sa Style::has_outline, Config
         */
        Config outline{ 0x000000 };
        /** Text shadow color, if any.
         *  @default \c 0x444444 <em>(plain gray)</em>
         *  @sa Style::has_shadow, Config
         */
        Config shadow{ 0x444444 };
        /** Text opacity.
         *  @default \c 255 <em>(fully opaque)</em>
         */
        uint8_t alpha{ 255 };
    };

    /** %Language related settings in corresponding text.*/
    struct Language {
    public:
        /** BCP 47 language tag, used in
         *  [Harfbuzz](https://harfbuzz.github.io/harfbuzz-hb-common.html#hb-language-t).
         *  @default \c "en"
         */
        std::string language{ "en" };
        /** ISO 15924 script, used in 
         *  [Harfbuzz](https://harfbuzz.github.io/harfbuzz-hb-common.html#hb-script-t).
         *  @default \c "Latn"
         */
        std::string script{ "Latn" };
        /** Writing direction, used in
         *  [Harfbuzz](https://harfbuzz.github.io/harfbuzz-hb-common.html#hb-direction-t).
         *  @default \c "ltr"
         */
        std::string direction{ "ltr" };
        /** [Word dividers](https://en.wikipedia.org/wiki/Word_divider),
         *  a blank space in most cases, but not always.
         *  Stored in a UTF32 string acting as a UTF32 vector.\n
         *  @default \c U" " <em>(blank space)</em>
         */
        std::u32string word_dividers{ U" " };
    };

    /** Font file name (needs to be initialised before processing).
     *  @default \c "arial.ttf"
     *  @sa loadFont(), addFontDir().
     */
    std::string font{ "arial.ttf" };
    /** See Style documentation.*/
    Style style;
    /** See Color documentation.*/
    Color color;
    /** See Language documentation.*/
    Language lng;
};

using Alignment = Format::Style::Alignment;
using Effect = Format::Style::Effect;
using ColorFunc = Format::Color::Func;

SSS_TR_END;