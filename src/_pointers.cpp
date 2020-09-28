#include "SSS/Text-Rendering/_pointers.hpp"

SSS_TR_BEGIN__

    // --- FT_Library ---

// Constructor
FT_Library_Ptr::FT_Library_Ptr(FT_Library ptr) noexcept
    : std::unique_ptr<FT_LibraryRec_, FT_Error(*)(FT_Library)>(ptr, FT_Done_FreeType)
{
    LOG_CONSTRUCTOR__
}

// Destructor
FT_Library_Ptr::~FT_Library_Ptr() noexcept
{
    LOG_DESTRUCTOR__
}

    // --- FT_Face ---

// Constructor
FT_Face_Ptr::FT_Face_Ptr(FT_Face ptr) noexcept
    : std::unique_ptr<FT_FaceRec_, FT_Error(*)(FT_Face)>(ptr, FT_Done_Face)
{
    LOG_CONSTRUCTOR__
}

// Destructor
FT_Face_Ptr::~FT_Face_Ptr() noexcept
{
    LOG_DESTRUCTOR__
}

// Create new face
void FT_Face_Ptr::create(FT_Library_Ptr const& lib, std::string const& path) try
{
    FT_Face face;
    FT_Error error = FT_New_Face(lib.get(), path.c_str(), 0, &face);
    THROW_IF_FT_ERROR__("FT_New_Face()");
    reset(face);
}
CATCH_AND_RETHROW_METHOD_EXC__

    // --- FT_Glyph ---

// Constructor
FT_Glyph_Ptr::FT_Glyph_Ptr(FT_Glyph ptr) noexcept
    : std::unique_ptr<FT_GlyphRec_, void(*)(FT_Glyph)>(ptr, FT_Done_Glyph)
{
    LOG_CONSTRUCTOR__
}

// Destructor
FT_Glyph_Ptr::~FT_Glyph_Ptr() noexcept
{
    LOG_DESTRUCTOR__
}

    // --- FT_BitmapGlyph ---

// Constructor
FT_BitmapGlyph_Ptr::FT_BitmapGlyph_Ptr(FT_BitmapGlyph ptr) noexcept
    : std::unique_ptr<FT_BitmapGlyphRec_, void(*)(FT_BitmapGlyph)>(ptr,
        [](FT_BitmapGlyph bmp) noexcept { FT_Done_Glyph((FT_Glyph)bmp); })
{
    LOG_CONSTRUCTOR__
}

// Destructor
FT_BitmapGlyph_Ptr::~FT_BitmapGlyph_Ptr() noexcept
{
    LOG_DESTRUCTOR__
}

    // --- FT_Stroker ---

// Constructor
FT_Stroker_Ptr::FT_Stroker_Ptr(FT_Stroker ptr) noexcept
    : std::unique_ptr<FT_StrokerRec_, void(*)(FT_Stroker)>(ptr, FT_Stroker_Done)
{
    LOG_CONSTRUCTOR__
}

// Destructor
FT_Stroker_Ptr::~FT_Stroker_Ptr() noexcept
{
    LOG_DESTRUCTOR__
}

// Create new stroker
void FT_Stroker_Ptr::create(FT_Library_Ptr const& lib)
{
    FT_Stroker stroker;
    FT_Error error = FT_Stroker_New(lib.get(), &stroker);
    THROW_IF_FT_ERROR__("FT_Stroker_New()");
    reset(stroker);
}

    // --- hb_font_t ---

// Constructor
HB_Font_Ptr::HB_Font_Ptr(hb_font_t *ptr) noexcept
    : std::unique_ptr<hb_font_t, void(*)(hb_font_t*)>(ptr, hb_font_destroy)
{
    LOG_CONSTRUCTOR__
}

// Destructor
HB_Font_Ptr::~HB_Font_Ptr() noexcept
{
    LOG_DESTRUCTOR__
}

    // --- hb_buffer_t ---

// Constructor
HB_Buffer_Ptr::HB_Buffer_Ptr(hb_buffer_t* ptr) noexcept
    : std::unique_ptr<hb_buffer_t, void(*)(hb_buffer_t*)>(ptr, hb_buffer_destroy)
{
    LOG_CONSTRUCTOR__
}

// Destructor
HB_Buffer_Ptr::~HB_Buffer_Ptr() noexcept
{
    LOG_DESTRUCTOR__
}

SSS_TR_END__