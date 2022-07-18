#include "SSS/Text-Rendering.hpp"
#include <SSS/GL.hpp>

static uint32_t areaID = 0;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_KP_0 || key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, true);
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        using namespace SSS::TR;
        bool const ctrl = mods & GLFW_MOD_CONTROL;
        switch (key) {
        case GLFW_KEY_ENTER:
            Area::getMap().at(areaID)->cursorAddText("\n");
            break;
        case GLFW_KEY_LEFT:
            Area::getMap().at(areaID)->cursorMove(ctrl ? Move::CtrlLeft : Move::Left);
            break;
        case GLFW_KEY_RIGHT:
            Area::getMap().at(areaID)->cursorMove(ctrl ? Move::CtrlRight : Move::Right);
            break;
        case GLFW_KEY_DOWN:
            Area::getMap().at(areaID)->cursorMove(Move::Down);
            break;
        case GLFW_KEY_UP:
            Area::getMap().at(areaID)->cursorMove(Move::Up);
            break;
        case GLFW_KEY_HOME:
            Area::getMap().at(areaID)->cursorMove(Move::Start);
            break;
        case GLFW_KEY_END:
            Area::getMap().at(areaID)->cursorMove(Move::End);
            break;
        case GLFW_KEY_BACKSPACE:
            Area::getMap().at(areaID)->cursorDeleteText(ctrl ? Delete::CtrlLeft : Delete::Left);
            break;
        case GLFW_KEY_DELETE:
            Area::getMap().at(areaID)->cursorDeleteText(ctrl ? Delete::CtrlRight : Delete::Right);
            break;
        }
    }
}

void char_callback(GLFWwindow* window, unsigned int codepoint)
{
    std::u32string str(1, static_cast<char32_t>(codepoint));
    SSS::TR::Area::getMap().at(areaID)->cursorAddText(str);
}

int main() try
{
    using namespace SSS;

    Log::TR::Fonts::get().glyph_load = true;

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
    auto const& area = TR::Area::create(300, 300);
    areaID = area->getID();
    area->setClearColor(SSS::RGBA32(0xFF, 0, 0, 0x44));
    auto fmt = area->getFormat();
    fmt.style.charsize = 30;
    fmt.color.text.func = TR::Format::Color::Func::rainbow;
    area->setFormat(fmt);
    area->parseString("Lorem\nipsum dolor sit amet.");
    texture->setTextAreaID(area->getID());
    texture->setType(GL::Texture::Type::Text);

    // Camera
    camera->setPosition({ 0, 0, 3 });
    camera->setProjectionType(GL::Camera::Projection::OrthoFixed);

    // Plane
    plane->setTextureID(texture->getID());
    plane->scale(glm::vec3(300));
    plane_renderer->chunks.emplace_back();
    plane_renderer->chunks[0].reset_depth_before = true;
    plane_renderer->chunks[0].objects.push_back(0);

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