#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

//#define SSS_LUA
#include "SSS/Text-Rendering.hpp"

using WindowPtr = SSS::C_Ptr<GLFWwindow, void(*)(GLFWwindow*), glfwDestroyWindow>;

static void compileShader(GLuint shader_id, std::string const& shader_code)
{
    // Compile shader
    char const* c_str = shader_code.c_str();
    glShaderSource(shader_id, 1, &c_str, NULL);
    glCompileShader(shader_id);
    // Throw if failed
    GLint res;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &res);
    if (res != GL_TRUE) {
        // Get error message
        int log_length;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
        std::vector<char> msg(log_length + 1);
        glGetShaderInfoLog(shader_id, log_length, NULL, &msg[0]);
        // Throw
        SSS::throw_exc(FUNC_MSG(CONTEXT_MSG("Could not compile shader", &msg[0])));
    }
}

static GLuint loadShaders(std::string const& vertex_data, std::string const& fragment_data)
{
    // Create the shaders
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    if (vertex_shader_id == 0) {
        SSS::throw_exc(CONTEXT_MSG("Could not create vertex shader", glGetError()));
    }
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    if (fragment_shader_id == 0) {
        SSS::throw_exc(CONTEXT_MSG("Could not create fragment shader", glGetError()));
    }
    // Compile the shaders
    compileShader(vertex_shader_id, vertex_data);
    compileShader(fragment_shader_id, fragment_data);

    // Link the program
    GLuint program_id = glCreateProgram();
    if (program_id == 0) {
        SSS::throw_exc(CONTEXT_MSG("Could not create program", glGetError()));
    }
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);
    // Throw if failed
    GLint res;
    glGetProgramiv(program_id, GL_LINK_STATUS, &res);
    if (res != GL_TRUE) {
        // Get error message
        int log_length;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
        std::vector<char> msg(log_length + 1);
        glGetProgramInfoLog(program_id, log_length, NULL, &msg[0]);
        // Throw
        SSS::throw_exc(FUNC_MSG(CONTEXT_MSG("Could not link program", &msg[0])));
    }

    // Free shaders
    glDetachShader(program_id, vertex_shader_id);
    glDetachShader(program_id, fragment_shader_id);
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Return newly created & linked program
    return program_id;
}

enum GLUID {
    prog,
    vao,
    vbo,
    ibo,
    tex,
};

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

inline void load_opengl(WindowPtr& window, GLuint ids[5], glm::mat4& VP) try
{
    using namespace SSS;

    // Create Window
    glfwInit();
    window.reset(glfwCreateWindow(800, 800, "SSS/Text-Rendering - Demo Window", nullptr, nullptr));
    // Throw if an error occured
    if (!window) {
        const char* msg;
        glfwGetError(&msg);
        throw_exc(msg);
    }
    glfwSetKeyCallback(window.get(), key_callback);
    glfwSetCharCallback(window.get(), char_callback);
    
    // Set context
    glfwMakeContextCurrent(window.get());

    // Init glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        SSS::throw_exc("Failed to initialize GLAD");
    }

    // Finish setting up window
    glViewport(0, 0, 800, 800);
    glClearColor(0.3f, 0.3f, 0.3f, 0.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Shaders
    std::string const vertex =
        "#version 330 core\n"
        "layout(location = 0) in vec3 a_Pos;\n"
        "layout(location = 1) in vec2 a_UV;\n"

        "uniform mat4 u_VP;\n"

        "out vec2 UV;\n"
        "flat out int instanceID;\n"

        "void main()\n"
        "{\n"
        "    gl_Position = u_VP * vec4(a_Pos, 1);\n"
        "    UV = a_UV;\n"
        "    instanceID = gl_InstanceID;\n"
        "}";

    std::string const fragment =
        "#version 330 core\n"
        "out vec4 FragColor;\n"

        "in vec2 UV;\n"
        "flat in int instanceID;\n"

        "uniform sampler2D u_Tex;\n"

        "void main()\n"
        "{\n"
        "    FragColor = texture(u_Tex, UV);\n"
        "}";
    ids[prog] = loadShaders(vertex, fragment);

    // VAO
    glGenVertexArrays(1, &ids[vao]);
    glBindVertexArray(ids[vao]);

    // VBO
    glGenBuffers(1, &ids[vbo]);
    glBindBuffer(GL_ARRAY_BUFFER, ids[vbo]);
    constexpr float vertices[] = {
        // positions            // texture coords (1 - y)
        -350.f,  350.f, 0.0f,   0.f, 1.f - 1.f,   // top left
        -350.f, -350.f, 0.0f,   0.f, 1.f - 0.f,   // bottom left
         350.f, -350.f, 0.0f,   1.f, 1.f - 0.f,   // bottom right
         350.f,  350.f, 0.0f,   1.f, 1.f - 1.f    // top right
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // IBO
    glGenBuffers(1, &ids[ibo]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ids[ibo]);
    constexpr unsigned int indices[] = {
        0, 1, 2,  // first triangle
        2, 3, 0   // second triangle
    };
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Texture
    glGenTextures(1, &ids[tex]);
    glBindTexture(GL_TEXTURE_2D, ids[tex]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Cam
    auto const view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    auto const proj = glm::ortho(-400.f, 400.f, -400.f, 400.f, 0.1f, 100.f);
    VP = proj * view;
}
CATCH_AND_LOG_FUNC_EXC;