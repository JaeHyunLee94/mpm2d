//
// Created by 이재현 on 2021/09/19.
//

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <Eigen/SVD>
#include <vector>
#include <fstream>
#include <random>

typedef glm::vec2 Vec2;
typedef glm::vec3 Vec3;
typedef glm::mat2 Mat2;
typedef double Scalar;

struct GridNode {
    Vec2 m_vel_i;
    Vec2 m_force_i;
    Scalar m_mass_i;
};
struct Particle {
    Vec2 m_pos_p;
    Vec2 m_vel_p;
    Vec3 m_force_P;
    Scalar m_mass_p;
    Mat2 m_F_p;

    Scalar m_J_p;

};
//openGL variable
GLFWwindow *window;
GLuint VAO;
GLuint VBO;
GLuint VBO_line;
GLuint shader_program_id;
const float line[]{
        0.0, 0.0,
        1.0, 0.0,
        1.0, 1.0,
        0.0, 1.0,

};


//simulation parameter
int grid_size;
Scalar dt;
Vec2 gravity;
Scalar initVolume;
Scalar radius;
Scalar particle_mass;
Scalar dx;
Scalar inv_dx;
Vec2 center;
unsigned int particle_num;
std::vector<Particle> particles;
std::vector<std::vector<GridNode>> grid;


//debug function
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

void logGrid() {

};

void logParticle() {

    int i = 0;
    for (auto &p : particles) {

        printf("%d th particle m_pos: (%f,%f), m_vel (%f,%f)\n", i, p.m_pos_p.x, p.m_pos_p.y,
               p.m_vel_p.x, p.m_vel_p.y);
        i++;
    }
};

//opengl function
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
    window = glfwCreateWindow(1024, 1024, "mpm2d 01", NULL, NULL);
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
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *) offsetof(Particle, m_pos_p));

    glGenBuffers(1, &VBO_line);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_line);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, line, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);

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

// simulation function
void render() {

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * particles.size(), particles.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *) offsetof(Particle, m_pos_p));
    glDrawArrays(GL_POINTS, 0, particle_num);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_line);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    glDrawArrays(GL_LINE_LOOP, 0, 4);


};

void simulationInit() {

    dt = 0.00001;
    grid_size = 128;
    particle_num = 8192;
    radius = 0.05;
    gravity = Vec2{0, -9.8};
    initVolume = 0.2f; //TODO
    particles.resize(particle_num);
    particle_mass = 1.0;
    dx = 1. / grid_size;
    inv_dx = 1. / dx;
    center.x = 0.5;
    center.y = 0.7;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 9999);

    //particle init
    for (int i = 0; i < particle_num; i++) {
        int random_num = dis(gen);
        Scalar r = (radius / 10000.0) * (Scalar) dis(gen);
        Scalar theta = (360.0 / 10000.0) * (Scalar) dis(gen);
        particles[i].m_pos_p.x = center.x + r * cos(theta);
        particles[i].m_pos_p.y = center.y + r * sin(theta);
        particles[i].m_vel_p.x = 0;
        particles[i].m_vel_p.y = 0;
        particles[i].m_F_p = Mat2(1.0);
        particles[i].m_J_p = 1;
        particles[i].m_mass_p = particle_mass;


    }

    //grid init
    grid.resize(grid_size);
    for (auto &g: grid) {
        g.resize(grid_size);
        for (int i = 0; i < grid_size; i++) {
            //vel + mass
            g[i].m_vel_i = Vec2(0);
            g[i].m_force_i = Vec2(0);
            g[i].m_mass_i = 0;
        }
    }


};


//cubic B-spline
Scalar N(Scalar x) {
    Scalar W;
    x = fabs(x);

    if (x < 1)
        W = (x * x * x / 2.0 - x * x + 2 / 3.0);

    else if (x < 2)
        W = (2 - x) * (2 - x) * (2 - x) / 6.0;

    else
        W = 0;

    return W;
}

Scalar diff_N(Scalar x) {


    Scalar dW;
    Scalar x_abs;
    x_abs = fabs(x);

    if (x_abs < 1)
        dW = 1.5 * x * x_abs - 2.0 * x;

    else if (x_abs < 2)
        dW = -x * x_abs / 2.0 + 2 * x - 2 * x / x_abs;

    else
        dW = 0;

    return dW;


}

//weight
Scalar W(Vec2 dist) {

    return N(dist.x) * N(dist.y);

}

Vec2 grad_W(Vec2 dist) {

    return Vec2(
            diff_N(dist[0]) * N(dist[1]),
            N(dist[0]) * diff_N(dist[1]));

}


void initGrid() {
    for (int i = 0; i < grid_size; i++) {
        for (int j = 0; j < grid_size; j++) {
            grid[i][j].m_vel_i = Vec2(0);
            grid[i][j].m_mass_i = 0;
            grid[i][j].m_force_i = Vec2(0);

        }
    }

};

void p2g() {

    for (auto &p : particles) {

        int base_x = static_cast<int> (std::floor(p.m_pos_p.x * inv_dx));
        int base_y = static_cast<int> (std::floor(p.m_pos_p.y * inv_dx));


        for (int i = -1; i < 3; i++) {
            for (int j = -1; j < 3; j++) {

                int coord_x = base_x + i;
                int coord_y = base_y + j;

                //printf("pos : %f,%f coord: %d,%d\n",p.m_pos_p.x,p.m_pos_p.y,coord_x,coord_y);

                //check boundary
                if (coord_x < 0 || coord_y < 0 || coord_x >= grid_size || coord_y >= grid_size) continue;
                Vec2 coord(coord_x, coord_y);
                Vec2 dist = (p.m_pos_p - coord * (float) dx) * (float) inv_dx;
                Scalar Weight = W(dist);

//                std::cout << Weight << "\n";
                grid[coord_x][coord_y].m_vel_i = p.m_vel_p * (float) (p.m_mass_p * Weight);
                grid[coord_x][coord_y].m_mass_i = p.m_mass_p * Weight;

                grid[coord_x][coord_y].m_vel_i /= grid[coord_x][coord_y].m_mass_i;


            }
        }


    }

};

void updateGridVel() {


    for (int i = 0; i < grid_size; i++) {
        for (int j = 0; j < grid_size; j++) {


            grid[i][j].m_vel_i += (gravity) * (float) dt;

        }

    }


};

void gridCollision() {

};

void g2p() {

    for (auto &p : particles) {

        int base_x = static_cast<int> (std::floor(p.m_pos_p.x * inv_dx));
        int base_y = static_cast<int> (std::floor(p.m_pos_p.y * inv_dx));


        for (int i = -1; i < 3; i++) {
            for (int j = -1; j < 3; j++) {

                int coord_x = base_x + i;
                int coord_y = base_y + j;
                //check boundary
                if (coord_x < 0 || coord_y < 0 || coord_x >= grid_size || coord_y >= grid_size) continue;
                Vec2 coord(coord_x, coord_y);
                Vec2 dist = (p.m_pos_p - coord * (float) dx) * (float) inv_dx;
                Scalar Weight = W(dist);


                p.m_vel_p += grid[coord_x][coord_y].m_vel_i * (float) Weight;


            }
        }


    }

};

void updateParticle() {
    for (auto &p : particles) {

        //explicit
        p.m_pos_p += p.m_vel_p * (float) dt;

    }


};

void particleCollision() {

};

void step() {

    initGrid();
    p2g();
    updateGridVel();
    gridCollision();
    g2p();
    updateParticle();
    particleCollision();
//
//    for (auto &p: particles) {
//        p.m_vel_p+=gravity*(float )dt;
//        p.m_pos_p+=p.m_vel_p*(float)dt;
//
//    }
}


int main() {

//TODO: cubic b spline 다른걸로
//TODO: p2g 말고 그냥 gravity update
    glWindowInit();
    shaderInit();
    simulationInit();
    glObjectInit();

    int frame = 0;
    do {

        glClear(GL_COLOR_BUFFER_BIT);
        glfwPollEvents();

        step();
        //logParticle();
        render();
        std::cout << "frame: " << frame << "\n";
        frame++;
        glfwSwapBuffers(window);


    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0);


    return 0;
}

