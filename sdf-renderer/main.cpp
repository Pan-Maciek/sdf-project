#include "pch.h"

#define WIDTH 600
#define HEIGHT 400

GLuint vao;
GLuint program;

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

GLuint loadShaderFromFile(const char* filename, GLenum shader_type, bool check_errors)
{
    GLuint result = 0;
    FILE* fp;
    size_t filesize;
    char* data;

    fp = fopen(filename, "rb");

    if (!fp)
        return 0;

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    data = new char[filesize + 1];

    if (!data)
        return -1;

    fread(data, 1, filesize, fp);
    data[filesize] = 0;
    fclose(fp);

    result = glCreateShader(shader_type);

    if (!result)
        return -1;

    glShaderSource(result, 1, &data, NULL);

    delete[] data;

    glCompileShader(result);

    if (check_errors)
    {
        GLint status = 0;
        glGetShaderiv(result, GL_COMPILE_STATUS, &status);

        if (!status)
        {
            char buffer[4096];
            glGetShaderInfoLog(result, 4096, NULL, buffer);

            OutputDebugStringA(filename);
            OutputDebugStringA(":");
            OutputDebugStringA(buffer);
            OutputDebugStringA("\n");


            glDeleteShader(result);
        }
    }

    return result;

}

void startup() {
    //shaders initialization
    program = glCreateProgram();
    GLuint vs = loadShaderFromFile("vs.vsh", GL_VERTEX_SHADER, true);
    GLuint fs = loadShaderFromFile("fs.fsh", GL_FRAGMENT_SHADER, true);
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    //vao initialization
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //prepare for rendering
    glViewport(0, 0, WIDTH, HEIGHT);
    glUseProgram(program);
    glUniform2f(0, WIDTH, HEIGHT);
}

void render(double currentTime) {
    static int frame = 0;
    static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, black);

    glUniform1f(1, currentTime);
    glUniform1i(2, frame);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    frame++;
}

void shutdown() {
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

}

int main() {
    bool running = true;
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "SDF_Project", NULL, NULL);
    if (!window)
    {
        fprintf(stderr, "Failed to open window\n");
        return -1;
    }

    glfwMakeContextCurrent(window);

    gl3wInit();

    startup();

    do {
        render(glfwGetTime());

        glfwSwapBuffers(window);
        glfwPollEvents();

        running &= (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE);
        running &= (glfwWindowShouldClose(window) != GL_TRUE);
    } while (running);

    shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();



    return 0;
}
