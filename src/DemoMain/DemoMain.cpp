#define SSS_LUA
#include "Text-Rendering.hpp"
#include "opengl.cpp"

static std::string const arabic_lorem_ipsum =
"لكن لا بد أن أوضح لك أن كل هذه الأفكار المغلوطة حول\n"
"استنكار  النشوة وتمجيد الألم نشأت بالفعل،\n"
"وسأعرض لك التفاصيل لتكتشف حقيقة وأساس تلك السعادة البشرية،\n"
"فلا أحد يرفض أو يكره أو يتجنب الشعور بالسعادة،\n"
"ولكن بفضل هؤلاء الأشخاص الذين لا يدركون بأن السعادة لا بد أن نستشعرها\n"
"بصورة أكثر عقلانية ومنطقية فيعرضهم هذا لمواجهة الظروف الأليمة،\n"
"ونيل المنال ويتلذذ بالآلام،"
//"\u001F1\u001F يرغب في الحب \u001F0\u001F"
"\u001F1\u001F[ Lorem ipsum ]\u001F0\u001F"
"وأكرر بأنه لا يوجد من\n"
"الألم هو الألم ولكن نتيجة لظروف ما قد تكمن السعاده فيما نتحمله من كد .\n"
"و سأعرض مثال حي لهذا،\n"
"من منا لم يتحمل جهد بدني شاق إلا من أجل الحصول على ميزة أو\n"
"فائدة؟ ولكن من لديه الحق أن ينتقد شخص ما أراد أن يشعر بالسعادة التي لا\n"
"تشوبها عواقب أليمة أو آخر أراد أن يتجنب الألم الذي ربما تنجم عنه بعض ؟\n"
"علي الجانب الآخر نشجب ونستنكر هؤلاء الرجال المفتونون بنش";

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        using namespace SSS::TR;
        bool const ctrl = mods & GLFW_MOD_CONTROL;
        switch (key) {
        case GLFW_KEY_F1:
            Area::get(0)->setPrintMode(PrintMode::Instant);
            //Area::get(0)->setPrintMode(PrintMode::Typewriter);
            break;
        case GLFW_KEY_ESCAPE:
        case GLFW_KEY_KP_0:
        case GLFW_KEY_F5:
            glfwSetWindowShouldClose(window, true);
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
        case GLFW_KEY_W: {
            if (ctrl)
                Area::history.undo();
        }   break;
        case GLFW_KEY_Y: {
            if (ctrl)
                Area::history.redo();
        }   break;
        }
    }
}

// Character input callback
static void char_callback(GLFWwindow* ptr, unsigned int codepoint)
{
    std::u32string str(1, static_cast<char32_t>(codepoint));
    SSS::TR::Area::cursorAddText(str);
}

glm::mat4 VP;

// Resize callback
static void size_callback(GLFWwindow* ptr, int w, int h)
{
    glViewport(0, 0, w, h);
    auto const view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
    const float w2 = static_cast<float>(w) / 2.f,
        h2 = static_cast<float>(h) / 2.f;
    auto const proj = glm::ortho(-w2, w2, -h2, h2, 0.1f, 100.f);
    VP = proj * view;
}

int main() try
{
    using namespace SSS;

    //Log::TR::Fonts::get().glyph_load = true;

    // Lua
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::string);
    lua_setup(lua);
    TR::lua_setup_TR(lua);
    lua.unsafe_script_file("Demo.lua");
    TR::Area::Shared area = lua["area"];
    area->setFocus(true);

    // OpenGL
    WindowPtr window;
    GLuint ids[5];
    glm::mat4 scaling, MVP;
    load_opengl(window, ids, VP);
    glfwSetKeyCallback(window.get(), key_callback);
    glfwSetCharCallback(window.get(), char_callback);
    glfwSetWindowSizeCallback(window.get(), size_callback);

    auto [w, h] = area->getDimensions();
    glfwSetWindowSize(window.get(), w, h);

    FrameTimer fps;

    // Main loop
    while (!glfwWindowShouldClose(window.get())) {

        // Poll events
        glfwPollEvents();
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        // Bind needed things
        glUseProgram(ids[prog]);
        glBindVertexArray(ids[vao]);
        glBindTexture(GL_TEXTURE_2D, ids[tex]);

        bool const ctrl = glfwGetKey(window.get(), GLFW_KEY_LEFT_CONTROL) ||
                          glfwGetKey(window.get(), GLFW_KEY_RIGHT_CONTROL);
        if ((glfwGetKey(window.get(), GLFW_KEY_LEFT_CONTROL) ||
            glfwGetKey(window.get(), GLFW_KEY_RIGHT_CONTROL)) &&
            glfwGetKey(window.get(), GLFW_KEY_A))
            area->selectAll();

        bool const shift = glfwGetKey(window.get(), GLFW_KEY_LEFT_SHIFT) ||
                           glfwGetKey(window.get(), GLFW_KEY_RIGHT_SHIFT);
        if (shift)
            area->lockSelection();
        if (glfwGetMouseButton(window.get(), GLFW_MOUSE_BUTTON_1)) {
            double x, y;
            glfwGetCursorPos(window.get(), &x, &y);
            area->cursorPlace((int)x, (int)y);
        }
        else if (!shift)
            area->unlockSelection();

        if (glfwGetKey(window.get(), GLFW_KEY_F4) == GLFW_PRESS) {
            area->formatSelection(nlohmann::json{{ "text_color", 255 }});
        }

        // Update texture if needed
        area->update();
        if (area->pixelsWereChanged()) {
            int w, h;
            area->pixelsGetDimensions(w, h);
            scaling = glm::scale(glm::mat4(1), glm::vec3(w, h, 1));
            MVP = VP * scaling;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, area->pixelsGet());
            area->pixelsAreRetrieved();
        }

        glUniformMatrix4fv(glGetUniformLocation(ids[prog], "u_VP"), 1, false, &MVP[0][0]);

        // Draw & print
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glfwSwapBuffers(window.get());
        //if (fps.addFrame() && fps.get() != 60) {
        //    LOG_CTX_MSG("fps", fps.getFormatted());
        //}
    }

    //lua.stack_clear();
    //area.reset();
    glfwTerminate();
    TR::terminate();
}
CATCH_AND_LOG_FUNC_EXC;