cmake_minimum_required(VERSION 3.10)
project("mpm2d")
set(CMAKE_CXX_STANDARD 14)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR}  ${GLUT_INCLUDE_DIRS})


### GLFW
message("processing GLFW...")
add_subdirectory(${PROJECT_SOURCE_DIR}/external/glfw)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
include_directories(${PROJECT_SOURCE_DIR}/external/glfw/include)

#### GLEW
message("processing GLEW...")
file(GLOB glew_src  "external/glew/include/GL/glew.h" "external/glew/include/GL/glew.c")
include_directories(${PROJECT_SOURCE_DIR}/external/glew/include)

### glm
message("include glm..")
include_directories(${PROJECT_SOURCE_DIR}/external/glm)

### Eigen library
message("include Eigen...")
include_directories(${PROJECT_SOURCE_DIR}/external/eigen-3.3.9/)


add_executable(mpm2d main.cpp ${glew_src})

target_link_libraries(mpm2d ${OPENGL_LIBRARIES})
target_link_libraries(mpm2d glfw ${GLFW_LIBRARIES})
