#pragma once

#include <SSS/Text-Rendering/includes.h>

SSS_TR_BEGIN__

// FT_Library
class FT_Library_Ptr
    : public std::unique_ptr<FT_LibraryRec_, FT_Error(*)(FT_Library)> {
#ifndef NDEBUG
private:
    static constexpr bool log_constructor_ = true;
    static constexpr bool log_destructor_ = true;
#endif // NDEBUG

public:
    FT_Library_Ptr() noexcept : FT_Library_Ptr(nullptr) {};
    FT_Library_Ptr(FT_Library ptr) noexcept;
    ~FT_Library_Ptr() noexcept;
};

// FT_Face
class FT_Face_Ptr
    : public std::unique_ptr<FT_FaceRec_, FT_Error(*)(FT_Face)> {
#ifndef NDEBUG
private:
    static constexpr bool log_constructor_ = true;
    static constexpr bool log_destructor_ = true;
#endif // NDEBUG

public:
    FT_Face_Ptr() noexcept : FT_Face_Ptr(nullptr) {};
    FT_Face_Ptr(FT_Face ptr) noexcept;
    ~FT_Face_Ptr() noexcept;
};

// FT_Glyph
class FT_Glyph_Ptr
    : public std::unique_ptr<FT_GlyphRec_, void(*)(FT_Glyph)> {
#ifndef NDEBUG
private:
    static constexpr bool log_constructor_ = true;
    static constexpr bool log_destructor_ = true;
#endif // NDEBUG

public:
    FT_Glyph_Ptr() noexcept : FT_Glyph_Ptr(nullptr) {};
    FT_Glyph_Ptr(FT_Glyph ptr) noexcept;
    ~FT_Glyph_Ptr() noexcept;
};

// FT_BitmapGlyph
class FT_BitmapGlyph_Ptr
    : public std::unique_ptr<FT_BitmapGlyphRec_, void(*)(FT_BitmapGlyph)> {
#ifndef NDEBUG
private:
    static constexpr bool log_constructor_ = true;
    static constexpr bool log_destructor_ = true;
#endif // NDEBUG

public:
    FT_BitmapGlyph_Ptr() noexcept : FT_BitmapGlyph_Ptr(nullptr) {};
    FT_BitmapGlyph_Ptr(FT_BitmapGlyph ptr) noexcept;
    ~FT_BitmapGlyph_Ptr() noexcept;
};

// FT_Stroker
class FT_Stroker_Ptr
    : public std::unique_ptr<FT_StrokerRec_, void(*)(FT_Stroker)> {
#ifndef NDEBUG
private:
    static constexpr bool log_constructor_ = true;
    static constexpr bool log_destructor_ = true;
#endif // NDEBUG

public:
    FT_Stroker_Ptr() noexcept : FT_Stroker_Ptr(nullptr) {};
    FT_Stroker_Ptr(FT_Stroker ptr) noexcept;
    ~FT_Stroker_Ptr() noexcept;
};

// hb_font_t
class HB_Font_Ptr
    : public std::unique_ptr<hb_font_t, void(*)(hb_font_t*)> {
#ifndef NDEBUG
private:
    static constexpr bool log_constructor_ = true;
    static constexpr bool log_destructor_ = true;
#endif // NDEBUG

public:
    HB_Font_Ptr() noexcept : HB_Font_Ptr(nullptr) {};
    HB_Font_Ptr(hb_font_t *ptr) noexcept;
    ~HB_Font_Ptr() noexcept;
};

SSS_TR_END__