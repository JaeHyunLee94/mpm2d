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
struct Particle{
    Vec2 m_pos;
    Vec2 m_vel;
    Scalar mass;
    Mat2 m_F;
    Scalar J;

};
//openGL variable
GLFWwindow* window;
GLuint VAO;
GLuint VBO;
GLuint VBO_line;
GLuint shader_program_id;
const float line[]{
    0.0,0.0,
    1.0,0.0,
    1.0,1.0,
    0.0,1.0,

};


//simulation parameter
int grid_size;
Scalar dt;
Scalar gravity;
Scalar radius;
Scalar particle_mass;
Scalar dx;
Scalar inv_dx;
Vec2 center;
unsigned int particle_num;
std::vector<Particle> particles;
std::vector<std::vector<Vec3>> grid;




//debug function
GLenum debug_glCheckError(int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << " (" << line << ")" << std::endl;
    }
    return errorCode;
}

void logGrid(){

};

void logParticle(){

};

//opengl function
int glWindowInit(){
    glewExperimental = true; // Needed for core profile
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

// Open a window and create its OpenGL context
    window = glfwCreateWindow( 1024, 1024, "mpm2d 01", NULL, NULL);
    if( window == NULL ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // Initialize GLEW
    glewExperimental=true; // Needed in core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    return 0;

}
void glObjectInit(){
    glPointSize(1.5f);
    debug_glCheckError(102);
    glGenVertexArrays(1,&VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1,&VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(Particle)*particles.size(),particles.data(),GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(Particle),(void*)offsetof(Particle, m_pos));

    glGenBuffers(1,&VBO_line);
    glBindBuffer(GL_ARRAY_BUFFER,VBO_line);
    glBufferData(GL_ARRAY_BUFFER,sizeof(float)*8,line,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,(void*)0);

};
void shaderInit(){
    GLuint m_vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint m_fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    const char* m_vertex_shader_path = "../shader/VertexShader.glsl";
    const char* m_fragment_shader_path = "../shader/FragmentShader.glsl";
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
        return ;
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
        return ;
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
        return ;
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

    glm::vec3 eye(0.5,0.5,1);
    glm::vec3 lookat(0.5,0.5,0);
    glm::vec3 up(0,1,0);
    glm::mat4 m_view_matrix =glm::lookAt(eye,lookat,up);
    glm::mat4 m_proj_matrix =glm::perspective(glm::radians(60.f),1.0f,0.01f,100.0f);
    GLint loc_view= glGetUniformLocation(shader_program_id,"viewMat");
    glUniformMatrix4fv(loc_view,1,GL_FALSE,&m_view_matrix[0][0]);

    GLint loc_proj= glGetUniformLocation(shader_program_id,"projMat");
    glUniformMatrix4fv(loc_proj,1,GL_FALSE,&m_proj_matrix[0][0]);

};
// simulation function
void render(){

    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(Particle)*particles.size(),particles.data(),GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(Particle),(void*)offsetof(Particle, m_pos));
    glDrawArrays(GL_POINTS,0,particle_num);

    glBindBuffer(GL_ARRAY_BUFFER,VBO_line);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,(void*)0);
    glDrawArrays(GL_LINE_LOOP,0,4);


};
void simulationInit(){

    dt=1./60;
    grid_size=100;
    particle_num=8000;
    radius=0.05;
    particles.resize(particle_num);
    particle_mass=1.0;
    dx=1./grid_size;
    inv_dx=1./dx;
    center.x=0.5;
    center.y=0.7;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 9999);

    //particle init
    for(int i=0;i< particle_num;i++){
        int random_num=dis(gen);
        Scalar r = (radius/10000.0)*(Scalar)dis(gen);
        Scalar theta=(360.0/10000.0)*(Scalar)dis(gen);
        particles[i].m_pos.x = center.x + r*cos(theta);
        particles[i].m_pos.y = center.y + r*sin(theta);
        particles[i].m_vel.x=0;
        particles[i].m_vel.y=0;
        particles[i].m_F=Mat2(1.0);
        particles[i].J=1;
        particles[i].mass=particle_mass;


    }

    //grid init
    grid.resize(grid_size);
    for(auto& g: grid){
        g.resize(grid_size);
        for(int i=0;i<grid_size;i++){
            //vel + mass
            g[i]=Vec3(0,0,particle_mass);
        }
    }


};


//cubic B-spline
Scalar N(Scalar x){
    x=abs(x);
    if(x>=0 && x<1){
        return 0.5* pow(x,3)- pow(x,2)+2./3;
    }else if(x>=1 && x<2){
        return -(1./6)* pow(x,3)+ pow(x,2) - 2*x+4./3;
    }else{
        return 0;
    }
}

Scalar diff_N(Scalar x){


    Scalar ret=0;
    if(x>=0 && x<1){
        ret= 1.5* pow(abs(x),2)- 2*pow(abs(x),2);
    }else if(x>=1 && x<2){
        ret -0.5* pow(abs(x),2)+ 2*pow(abs(x),2) -2;
    }

    return x>0 ? ret : -ret;


}


//weight
Scalar W(Vec2 x_p,int grid_i,int grid_j){
    Scalar fx= inv_dx*(x_p.x-grid_i*dx);
    Scalar fy= inv_dx*(x_p.y-grid_j*dx);
    return N(fx)*N(fy);

}

Vec2 grad_W(Vec2 x_p,int grid_i,int grid_j){

    Scalar fx= inv_dx*(x_p.x-grid_i*dx);
    Scalar fy= inv_dx*(x_p.y-grid_j*dx);

    Vec2 ret(0);
    ret.x=N(fx)*inv_dx* diff_N(fx);
    ret.y=N(fy)*inv_dx* diff_N(fy);

    return ret;

}


void initGrid(){
    for(int i=0;i<grid_size;i++){
        for(int j=0;j<grid_size;j++){
            grid[i][j].x=0;
            grid[i][j].y=0;
            grid[i][j].z=0;
        }
    }

};
void p2g(){

    for(auto& p : particles){






    }

};
void updateGridVel(){



};
void gridCollision(){

};
void g2p(){

    for(int i=0;i<grid_size;i++){

        for(int j=0;j<grid_size;j++){



        }
    }

};
void updateParticle(){


};

void particleCollision(){

};

void step(){

    initGrid();
    p2g();
    updateGridVel();
    gridCollision();
    g2p();
    updateParticle();
    particleCollision();

}




int main(){


    glWindowInit();
    shaderInit();
    simulationInit();
    glObjectInit();

    do{

        glClear( GL_COLOR_BUFFER_BIT );
        glfwPollEvents();

        step();
        render();

        glfwSwapBuffers(window);



    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );


    return 0;
}

