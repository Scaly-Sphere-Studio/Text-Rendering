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


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_KP_0) {
        glfwSetWindowShouldClose(window, true);
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        using namespace SSS::TR;
        bool const ctrl = mods & GLFW_MOD_CONTROL;
        switch (key) {
        case GLFW_KEY_ESCAPE:
            Area::resetFocus();
            break;
        case GLFW_KEY_ENTER:
            Area::cursorAddText("\n");
            break;
        case GLFW_KEY_LEFT:
            Area::cursorMove(ctrl ? Move::CtrlLeft : Move::Left);
            break;
        case GLFW_KEY_RIGHT:
            Area::cursorMove(ctrl ? Move::CtrlRight : Move::Right);
            break;
        case GLFW_KEY_DOWN:
            Area::cursorMove(Move::Down);
            break;
        case GLFW_KEY_UP:
            Area::cursorMove(Move::Up);
            break;
        case GLFW_KEY_HOME:
            Area::cursorMove(Move::Start);
            break;
        case GLFW_KEY_END:
            Area::cursorMove(Move::End);
            break;
        case GLFW_KEY_BACKSPACE:
            Area::cursorDeleteText(ctrl ? Delete::CtrlLeft : Delete::Left);
            break;
        case GLFW_KEY_DELETE:
            Area::cursorDeleteText(ctrl ? Delete::CtrlRight : Delete::Right);
            break;
        case GLFW_KEY_KP_ADD:
            Area::getMap().at(areaID)->setMarginV(Area::getMap().at(areaID)->getMarginV() + 1);
            break;
        case GLFW_KEY_KP_SUBTRACT:
            Area::getMap().at(areaID)->setMarginV(Area::getMap().at(areaID)->getMarginV() - 1);
            break;
        }
    }
}

void char_callback(GLFWwindow* window, unsigned int codepoint)
{
    std::u32string str(1, static_cast<char32_t>(codepoint));
    SSS::TR::Area::cursorAddText(str);
}

void func(SSS::GL::Window::Shared win, SSS::GL::Plane::Ptr const& plane,
    int button, int action, int mod)
{
    if (action == GLFW_PRESS) {
        auto const& objects = win->getObjects();
        auto const& tex = objects.textures.at(plane->getTextureID());
        if (button == GLFW_MOUSE_BUTTON_1) {
            int x, y;
            plane->getRelativeCoords(x, y);
            tex->getTextArea()->cursorPlace(x, y);
        }
    }
}

int main() try
{
    using namespace SSS;

    SSS::GL::Plane::on_click_funcs = { { 1, func } };

    //Log::TR::Fonts::get().glyph_load = true;
    //Log::GL::Window::get().fps = true;

    // Create Window
    GL::Window::CreateArgs args;
    args.title = "SSS/Text-Rendering - Demo Window";
    args.w = static_cast<int>(600);
    args.h = static_cast<int>(600);
    GL::Window::Shared window = GL::Window::create(args);

    // Set context
    GL::Context const context(window);

    // Finish setting up window
    glClearColor(0.3f, 0.3f, 0.3f, 0.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    window->setVSYNC(true);
    window->setCallback(glfwSetKeyCallback, key_callback);
    window->setCallback(glfwSetCharCallback, char_callback);

    // SSS/GL objects

    // Create objects
    auto const& texture = GL::Texture::create();
    auto const& camera = GL::Camera::create();
    auto const& plane = GL::Plane::create();
    auto const& plane_renderer = GL::Plane::Renderer::create();

    // Text
    TR::Format fmt;
    fmt.style.charsize = 30;
    fmt.style.has_shadow = true;
    fmt.style.has_outline = true;
    fmt.style.outline_size = 1;

    TR::Area::setDefaultMargins(30, 30);
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
    plane->setOnClickFuncID(1);
    plane->setTextureID(texture->getID());
    int w, h;
    area->getDimensions(w, h);
    plane->scale(glm::vec3(static_cast<float>(h) * 0.99f));
    plane->translate(glm::vec3(-w / 2 - 20, 0, 0));

    auto const& plane2 = GL::Plane::create();
    plane2->setHitbox(GL::Plane::Hitbox::Full);
    plane2->setOnClickFuncID(1);
    TR::Area::setDefaultMargins(40, 70);
    auto const& area2 = TR::Area::create(lorem_ipsum2, fmt);
    area2->setClearColor(0xFF888888);
    auto const& texture2 = GL::Texture::create();
    texture2->setTextAreaID(area2->getID());
    texture2->setType(GL::Texture::Type::Text);
    plane2->setTextureID(texture2->getID());
    plane2->scale(glm::vec3(static_cast<float>(h) * 0.99f));
    plane2->translate(glm::vec3(w / 2 + 20, 0, 0));
    
    plane_renderer->chunks.emplace_back();
    plane_renderer->chunks[0].reset_depth_before = true;
    plane_renderer->chunks[0].objects.push_back(plane->getID());
    plane_renderer->chunks[0].objects.push_back(plane2->getID());

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