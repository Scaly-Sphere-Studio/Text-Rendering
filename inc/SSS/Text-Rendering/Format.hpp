#pragma once

#include "_internal/_includes.hpp"

__SSS_TR_BEGIN;

/** Structure defining different formats to be used in Area instances.
 *  The structure holds 4 variables
 */
struct Format {
    /** Sets the style of corresponding text.*/
    struct Style {
        /** Font size, in pt.
         *  Default value: \c 12.
         */
        int charsize{ 12 };
        /** Whether the text has an outline.
         *  Default value: \c false.
         *  @sa #outline_size.
         */
        bool has_outline{ false };
        /** Outline size, if any.
         *  Default value: \c 0.
         *  @sa #has_outline.
         */
        int outline_size{ 0 };
        /** Whether the text has a shadow.
         *  Default value: \c false.
         */
        bool has_shadow{ false };
        /** Spacing between lines.
         *  Default value: \c 1.5.
         */
        float line_spacing{ 1.5f };
    };

    /** Sets the color of corresponding text (& style features).
     *  The color can be plain, or use one of the defined functions in \c Func.
     */
    struct Color {
        /** Used in Config to determine the color at runtime.*/
        enum class Func {
            none,   /**< No function, the Config::plain color is used.*/
            rainbow /**< The color is determined by the width ratio*/
        };
        /** Holds both a plain color and a #Func variable.*/
        struct Config {
            /** The plain color to be used if \c #func is set to \c Func::plain.*/
            RGB24 plain;
            /** The way to determine the color of the text.*/
            Func func{ Func::none };
        };
        /** Text color.
         *  Default value: \c 0xFFFFFF, plain white.\n
         *  @sa Config
         */
        Config text{ 0xFFFFFF };
        /** Text outline color, if any.
         *  Default value: \c 0, plain black.\n
         *  @sa Style::has_outline, Config
         */
        Config outline{ 0x000000 };
        /** Text shadow color, if any.
         *  Default value: \c 0x444444, plain gray.\n
         *  @sa Style::has_shadow, Config
         */
        Config shadow{ 0x444444 };
        /** Text opacity.
         *  Default value: \c 255, fully opaque.
         */
        uint8_t alpha{ 255 };
    };

    /** %Language related settings in corresponding text.*/
    struct Language {
    public:
        /** BCP 47 language tag, used in
         *  [Harfbuzz](https://harfbuzz.github.io/harfbuzz-hb-common.html#hb-language-t).
         *  Default value: \c "en".
         */
        std::string language{ "en" };
        /** ISO 15924 script, used in 
         *  [Harfbuzz](https://harfbuzz.github.io/harfbuzz-hb-common.html#hb-script-t).
         *  Default value: \c "Latn".
         */
        std::string script{ "Latn" };
        /** Writing direction, used in
         *  [Harfbuzz](https://harfbuzz.github.io/harfbuzz-hb-common.html#hb-direction-t).
         *  Default value: \c "ltr".
         */
        std::string direction{ "ltr" };
        /** [Word dividers](https://en.wikipedia.org/wiki/Word_divider),
         *  a blank space in most cases, but not always.
         *  Default value: \c U" ".
         */
        std::u32string word_dividers{ U" " };
    };

    // --- Variables ---
    std::string font{ "arial.ttf" };   // Font
    Style style;        // Style
    Color color;        // Colors
    Language lng;       // Language
};

__SSS_TR_END;