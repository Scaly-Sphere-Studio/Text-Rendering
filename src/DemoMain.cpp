#include "SSS/Text-Rendering.hpp"
#include <SSS/GL.hpp>

static uint32_t areaID = 0;

static std::string const lorem_ipsum =
"Lorem ipsum dolor sit amet,\n"
"consectetur adipiscing elit.\n"
"Pellentesque vitae velit ante.\n"
"Suspendisse nulla lacus,\n"
"tempor sit amet iaculis non,\n"
"scelerisque sed est.\n"
"Aenean pharetra ipsum sit amet sem lobortis,\n"
"a cursus felis semper.\n"
"Integer nec tortor ex.\n"
"Etiam quis consectetur turpis.\n"
"Proin ultrices bibendum imperdiet.\n"
"Suspendisse vitae fermentum ante,\n"
"eget cursus dolor.";

static std::string const lorem_ipsum2 =
"Lorem ipsum dolor sit amet,\n"
"consectetur adipiscing elit.\n"
"Nam ornare arcu turpis,\n"
"at suscipit purus rhoncus eu.\n"
"Donec vitae diam eget ante imperdiet mollis.\n"
"Maecenas id est sit amet sapien eleifend gravida.\n"
"Curabitur sit amet felis condimentum.\n"
"varius augue eu,\n"
"maximus nunc.\n"
"Curabitur quis laoreet tellus.\n"
"Donec urna arcu,\n"
"egestas vitae commodo euismod,\n"
"sodales vitae nunc.";

static std::string const arabic_lorem_ipsum =
"لكن لا بد أن أوضح لك أن كل هذه الأفكار المغلوطة حول\n"
"استنكار  النشوة وتمجيد الألم نشأت بالفعل،\n"
"وسأعرض لك التفاصيل لتكتشف حقيقة وأساس تلك السعادة البشرية،\n"
"فلا أحد يرفض أو يكره أو يتجنب الشعور بالسعادة،\n"
"ولكن بفضل هؤلاء الأشخاص الذين لا يدركون بأن السعادة لا بد أن نستشعرها\n"
"بصورة أكثر عقلانية ومنطقية فيعرضهم هذا لمواجهة الظروف الأليمة،\n"
"وأكرر بأنه لا يوجد من يرغب في الحب ونيل المنال ويتلذذ بالآلام،\n"
"الألم هو الألم ولكن نتيجة لظروف ما قد تكمن السعاده فيما نتحمله من كد .\n"
"و سأعرض مثال حي لهذا،\n"
"من منا لم يتحمل جهد بدني شاق إلا من أجل الحصول على ميزة أو\n"
"فائدة؟ ولكن من لديه الحق أن ينتقد شخص ما أراد أن يشعر بالسعادة التي لا\n"
"تشوبها عواقب أليمة أو آخر أراد أن يتجنب الألم الذي ربما تنجم عنه بعض ؟\n"
"علي الجانب الآخر نشجب ونستنكر هؤلاء الرجال المفتونون بنش";

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS && (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_KP_0)) {
        glfwSetWindowShouldClose(window, true);
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        using namespace SSS::TR;
        switch (key) {
        case GLFW_KEY_KP_ADD: {
            auto const& area = Area::getMap().at(areaID);
            area->setMarginV(area->getMarginV() + 1);
        }   break;
        case GLFW_KEY_KP_SUBTRACT: {
            auto const& area = Area::getMap().at(areaID);
            area->setMarginV(area->getMarginV() - 1);
        }   break;
        }
    }
}

int main() try
{
    using namespace SSS;

    //Log::TR::Fonts::get().glyph_load = true;
    //Log::GL::Window::get().fps = true;

    // Create Window
    GL::Window::CreateArgs args;
    args.title = "SSS/Text-Rendering - Demo Window";
    args.w = static_cast<int>(600);
    args.h = static_cast<int>(600);
    //args.maximized = true;
    GL::Window::Shared window = GL::Window::create(args);

    // Set context
    GL::Context const context(window);

    // Finish setting up window
    glClearColor(0.3f, 0.3f, 0.3f, 0.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    window->setVSYNC(true);
    window->setCallback(glfwSetKeyCallback, key_callback);

    // SSS/GL objects

    // Create objects
    auto const& texture = GL::Texture::create();
    auto const& camera = GL::Camera::create();
    auto const& plane = GL::Plane::create();
    auto const& plane_renderer = GL::Renderer::create<GL::PlaneRenderer>();

    // Text
    TR::Format fmt;
    fmt.style.charsize = 30;
    fmt.style.has_outline = true;
    fmt.style.has_shadow = true;
    fmt.style.outline_size = 1;
    fmt.style.aligmnent = TR::Alignment::Right;

    //fmt.color.text.func = TR::ColorFunc::rainbow;

    //fmt.lng.direction = "rtl";
    //fmt.lng.script = "Arab";
    //fmt.lng.language = "ar";
    //fmt.font = "LateefRegOT.ttf";
    //TR::addFontDir("C:/dev/fonts");

    auto const& area = TR::Area::create(lorem_ipsum, fmt);
    area->setClearColor(0xFF888888);
    //area->setPrintMode(TR::Area::PrintMode::Typewriter);
    //area->setTypeWriterSpeed(60);
    areaID = area->getID();

    texture->setTextAreaID(area->getID());
    texture->setType(GL::Texture::Type::Text);

    // Camera
    camera->setPosition({ 0, 0, 3 });
    camera->setProjectionType(GL::Camera::Projection::OrthoFixed);

    // Plane
    plane->setHitbox(GL::Plane::Hitbox::Full);
    plane->setTextureID(texture->getID());
    int w, h;
    area->getDimensions(w, h);
    glfwSetWindowSize(window->getGLFWwindow(), w, h);
    plane->scale(glm::vec3(static_cast<float>(h)));
    plane->translate(glm::vec3(-w / 2 - 20, 0, 0));

    auto const& plane2 = GL::Plane::create();
    plane2->setHitbox(GL::Plane::Hitbox::Full);
    fmt.style.aligmnent = TR::Alignment::Left;
    auto const& area2 = TR::Area::create(lorem_ipsum2, fmt);
    area2->setClearColor(0xFF888888);
    auto const& texture2 = GL::Texture::create();
    texture2->setTextAreaID(area2->getID());
    texture2->setType(GL::Texture::Type::Text);
    plane2->setTextureID(texture2->getID());
    plane2->scale(glm::vec3(static_cast<float>(h)));
    plane2->translate(glm::vec3(w / 2 + 20, 0, 0));
    
    auto& chunks = plane_renderer->castAs<GL::PlaneRenderer>().chunks;
    chunks.emplace_back(camera, true);
    chunks.back().planes.emplace_back(plane);
    chunks.back().planes.emplace_back(plane2);

    // Main loop
    while (!window->shouldClose()) {
        // Poll events, threads, etc
        GL::pollEverything();
        // Draw renderers
        window->drawObjects();
        // Swap buffers
        window->printFrame();
    }
}
CATCH_AND_LOG_FUNC_EXC;