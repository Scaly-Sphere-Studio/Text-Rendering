#define SSS_LUA
#include "SSS/Text-Rendering.hpp"
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
        }
    }
}

// Character input callback
static void char_callback(GLFWwindow* ptr, unsigned int codepoint)
{
    std::u32string str(1, static_cast<char32_t>(codepoint));
    SSS::TR::Area::cursorAddText(str);
}

int main() try
{
    using namespace SSS;

    //Log::TR::Fonts::get().glyph_load = true;

    auto const& area = TR::Area::create(700, 700);

    // Lua
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::string);
    lua_setup(lua);
    TR::lua_setup_TR(lua);
    lua.unsafe_script_file("Demo.lua");

    // OpenGL
    WindowPtr window;
    GLuint ids[5];
    glm::mat4 VP;
    load_opengl(window, ids, VP);
    glfwSetKeyCallback(window.get(), key_callback);
    glfwSetCharCallback(window.get(), char_callback);

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
        glUniformMatrix4fv(glGetUniformLocation(ids[prog], "u_VP"), 1, false, &VP[0][0]);

        // Update texture if needed
        area->update();
        if (area->pixelsWereChanged()) {
            int w, h;
            area->pixelsGetDimensions(w, h);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, area->pixelsGet());
            area->pixelsAreRetrieved();
        }

        // Draw & print
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glfwSwapBuffers(window.get());
        if (fps.addFrame() && fps.get() != 60) {
            LOG_CTX_MSG("fps", fps.getFormatted());
        }
    }

    glfwTerminate();
}
CATCH_AND_LOG_FUNC_EXC;