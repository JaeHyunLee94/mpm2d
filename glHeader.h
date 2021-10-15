//
// Created by test on 2021-10-15.
//

#ifndef MPM2D_GLHEADER_H
#define MPM2D_GLHEADER_H
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <Eigen/SVD>
#include <Eigen/Dense>
#include <vector>
#include <fstream>
#include <random>
#include "simHeader.h"

#define RECORD_VIDEO true

GLFWwindow *window;
GLuint VAO;
GLuint VBO;
GLuint VBO_line;
GLuint shader_program_id;
unsigned int frame = 0;
const unsigned int WINDOW_WIDTH=1024;
const unsigned int WINDOW_HEIGHT=1024;
const unsigned int END_FRAME=3000;

const double line[]{
        0.0, 0.0,
        1.0, 0.0,
        1.0, 1.0,
        0.0, 1.0,
};

#if RECORD_VIDEO
//Make sure ffmpeg has installed and added to Environment variable.
const static int FPS = 60;
std::string str_cmd = "ffmpeg -r " + std::to_string(FPS) + " -f rawvideo -pix_fmt rgba -s "
                      + std::to_string(WINDOW_WIDTH) + "x" + std::to_string(WINDOW_HEIGHT)
                      + " -i - -threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip out/movie.mp4";
const char *cmd = str_cmd.c_str();
FILE *ffmpeg = _popen(cmd, "wb");
int *buffer = new int[WINDOW_WIDTH * WINDOW_HEIGHT];
#endif

GLenum debug_glCheckError(int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                error = "STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                error = "STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
        std::cout << error << " | " << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
int glWindowInit() {
    glewExperimental = true; // Needed for core profile
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

// Open a window and create its OpenGL context
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "mpm2d 01", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr,
                "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // Initialize GLEW
    glewExperimental = true; // Needed in core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    return 0;

}
void glObjectInit() {
    glPointSize(1.5f);
    debug_glCheckError(102);
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * particles.size(), particles.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_DOUBLE, GL_FALSE, sizeof(Particle), (void *) offsetof(Particle, m_pos_p));

    glGenBuffers(1, &VBO_line);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_line);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Scalar) * 8, line, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_DOUBLE, GL_FALSE, 0, (void *) 0);

};
void shaderInit() {
    GLuint m_vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint m_fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    const char *m_vertex_shader_path = "../shader/VertexShader.glsl";
    const char *m_fragment_shader_path = "../shader/FragmentShader.glsl";
    std::string m_vertex_shader_code;
    std::string m_fragment_shader_code;

    std::ifstream VertexShaderStream(m_vertex_shader_path, std::ios::in);
    if (VertexShaderStream.is_open()) {
        std::stringstream sstr;
        sstr << VertexShaderStream.rdbuf();
        m_vertex_shader_code = sstr.str();
        VertexShaderStream.close();
    } else {

        printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n",
               m_vertex_shader_path);
        getchar();
        return;
    }
    // Read the Fragment Shader code from the file

    std::ifstream FragmentShaderStream(m_fragment_shader_path, std::ios::in);
    if (FragmentShaderStream.is_open()) {
        std::stringstream sstr;
        sstr << FragmentShaderStream.rdbuf();
        m_fragment_shader_code = sstr.str();
        FragmentShaderStream.close();
    } else {
        printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n",
               m_fragment_shader_path);
        getchar();
        return;
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;
    // Compile Vertex Shader
    std::cout << "Compiling Vertex shader : " << m_vertex_shader_path << "\n";

    char const *VertexSourcePointer = m_vertex_shader_code.c_str();
    glShaderSource(m_vertex_shader_id, 1, &VertexSourcePointer, NULL);
    glCompileShader(m_vertex_shader_id);

    // Check Vertex Shader
    glGetShaderiv(m_vertex_shader_id, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(m_vertex_shader_id, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(m_vertex_shader_id, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        printf("%s\n", &VertexShaderErrorMessage[0]);
        return;
    }

    // Compile Fragment Shader
    std::cout << "Compiling Fragment shader : " << m_fragment_shader_path << "\n";
    char const *FragmentSourcePointer = m_fragment_shader_code.c_str();
    glShaderSource(m_fragment_shader_id, 1, &FragmentSourcePointer, NULL);
    glCompileShader(m_fragment_shader_id);

    // Check Fragment Shader
    glGetShaderiv(m_fragment_shader_id, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(m_fragment_shader_id, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(m_fragment_shader_id, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        printf("%s\n", &FragmentShaderErrorMessage[0]);
        return;
    }

    std::cout << "Shader Successfully compiled\n";


    shader_program_id = glCreateProgram();
    // Link the program
    std::cout << "Linking program...\n";


    glAttachShader(shader_program_id, m_vertex_shader_id);
    glAttachShader(shader_program_id, m_fragment_shader_id);
    glLinkProgram(shader_program_id);

    // Check the program
    glGetProgramiv(shader_program_id, GL_LINK_STATUS, &Result);
    glGetProgramiv(shader_program_id, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(shader_program_id, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        printf("%s\n", &ProgramErrorMessage[0]);
        return;
    }


    glDetachShader(shader_program_id, m_vertex_shader_id);
    glDetachShader(shader_program_id, m_fragment_shader_id);

    glDeleteShader(m_vertex_shader_id);
    glDeleteShader(m_fragment_shader_id);
    std::cout << "Shader Program successfully made\n";
    glUseProgram(shader_program_id);

    glm::vec3 eye(0.5, 0.5, 1);
    glm::vec3 lookat(0.5, 0.5, 0);
    glm::vec3 up(0, 1, 0);
    glm::mat4 m_view_matrix = glm::lookAt(eye, lookat, up);
    glm::mat4 m_proj_matrix = glm::perspective(glm::radians(60.f), 1.0f, 0.01f, 100.0f);
    GLint loc_view = glGetUniformLocation(shader_program_id, "viewMat");
    glUniformMatrix4fv(loc_view, 1, GL_FALSE, &m_view_matrix[0][0]);

    GLint loc_proj = glGetUniformLocation(shader_program_id, "projMat");
    glUniformMatrix4fv(loc_proj, 1, GL_FALSE, &m_proj_matrix[0][0]);

};
void render() {

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * particles.size(), particles.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_DOUBLE, GL_FALSE, sizeof(Particle), (void *) offsetof(Particle, m_pos_p));
    glDrawArrays(GL_POINTS, 0, particle_num);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_line);
    glVertexAttribPointer(0, 2, GL_DOUBLE, GL_FALSE, 0, (void *) 0);
    glDrawArrays(GL_LINE_LOOP, 0, 4);


};

#endif //MPM2D_GLHEADER_H
