#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include <stdio.h>
#include <iostream>
#define WIDTH 1280
#define HEIGHT 720

GLuint vao;
GLuint program;
GLuint programQuad;
GLuint fbo[2];
GLuint color_texture[2];
static const GLenum draw_buffers[] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1
};

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
void fboInit() {

    glGenFramebuffers(2, &fbo[0]);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo[0]);

    glGenTextures(2, &color_texture[0]);

    glBindTexture(GL_TEXTURE_2D, color_texture[0]);
    glTexStorage2D(GL_TEXTURE_2D, 9, GL_RGBA16F, WIDTH, HEIGHT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture(GL_FRAMEBUFFER, draw_buffers[0], color_texture[0], 0);
    glDrawBuffers(1, &draw_buffers[0]);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo[1]);
    glBindTexture(GL_TEXTURE_2D, color_texture[1]);
    glTexStorage2D(GL_TEXTURE_2D, 9, GL_RGBA16F, WIDTH, HEIGHT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture(GL_FRAMEBUFFER, draw_buffers[1], color_texture[1], 0);
    
    glDrawBuffers(1, &draw_buffers[1]);
}

void startup() {
	//shaders initialization
    program = glCreateProgram();
    GLuint vs=loadShaderFromFile("vs.vsh",GL_VERTEX_SHADER,true);
    GLuint fs = loadShaderFromFile("fs.fsh", GL_FRAGMENT_SHADER, true);
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    programQuad = glCreateProgram();
    vs = loadShaderFromFile("vs.vsh", GL_VERTEX_SHADER, true);
    fs = loadShaderFromFile("quad.fsh", GL_FRAGMENT_SHADER, true);
    glAttachShader(programQuad, vs);
    glAttachShader(programQuad, fs);
    glLinkProgram(programQuad);
    glDeleteShader(vs);
    glDeleteShader(fs);

    //vao initialization
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //prepare for rendering
    glViewport(0, 0, WIDTH, HEIGHT);

    fboInit();
}

void render(double currentTime) {
    static int frame = 0;
    static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    glBindFramebuffer(GL_FRAMEBUFFER, fbo[frame%2]);
    glBindTexture(GL_TEXTURE_2D, color_texture[(frame + 1) % 2]);
    glUseProgram(program);

   // glClearBufferfv(GL_COLOR, 0, black);

    glUniform2f(0, WIDTH, HEIGHT);
    glUniform1f(1, currentTime);
    glUniform1i(2, frame);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, WIDTH, HEIGHT);
    glBindTexture(GL_TEXTURE_2D, color_texture[frame % 2]);
    glUseProgram(programQuad);
    glClearBufferfv(GL_COLOR, 0, black);

    glUniform2f(0, WIDTH, HEIGHT);
    glUniform1f(1, currentTime);
    glUniform1i(2, frame);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindTexture(GL_TEXTURE_2D, 0);


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

	do{
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