//
// Created by 이재현 on 2021/09/19.
//
#include "glHeader.h"

int main() {

    glWindowInit();
    shaderInit();
    simulationInit();
    glObjectInit();

    do {

        glClear(GL_COLOR_BUFFER_BIT);
        glfwPollEvents();
        //main simulation step

        step();
        render();

        std::cout << "frame: " << frame << "\n";
        frame++;

        glfwSwapBuffers(window);

#if RECORD_VIDEO
        //Make sure ffmpeg has installed and added to Environment variable.
        glReadPixels(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
        fwrite(buffer, sizeof(int) * WINDOW_WIDTH * WINDOW_HEIGHT, 1, ffmpeg);
#endif

    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 && frame <END_FRAME);
#if RECORD_VIDEO
    _pclose(ffmpeg);
#endif

    return 0;
}

