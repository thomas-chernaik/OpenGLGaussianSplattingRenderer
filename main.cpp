#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Splats.h"
#include "Camera.h"
int main()
{
    //intialise glfw
    if (!glfwInit()) {
        std::cerr << "Failed to initialise GLFW" << std::endl;
        return -1;
    }

    //set error callback
    glfwSetErrorCallback(errorCallback);

    //create window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Hello World", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }

    //make window current context
    glfwMakeContextCurrent(window);

    //initialise glew
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialise GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }
    //initialise timer stuff
    GLuint timerQuery;
    glGenQueries(1, &timerQuery);


    //initialise camera
    Camera camera(5.0f, 0.5f, -4.0f);
    //make the camera look down a bit
    camera.rotateDown(20.0f);
    //rotate camera 40 degrees
    camera.rotateRight(40.0f);
    camera.update();

    Splats splats("models/bike-small.ply", camera.getWidth(), camera.getHeight());
    GLuint startTime, endTime;
    glGenQueries(1, &startTime);
    glGenQueries(1, &endTime);
    //display window
    while (!glfwWindowShouldClose(window)) {
        //store start time with a GL_TIMESTAMP
        glQueryCounter(startTime, GL_TIMESTAMP);
        //resize window to camera size
        glfwSetWindowSize(window, camera.getWidth(), camera.getHeight());
        //set viewport
        glViewport(0, 0, camera.getWidth(), camera.getHeight());
        //clear window
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        splats.gpuRender(camera.getViewMatrix(), camera.getWidth(), camera.getHeight(), camera.getFocalX(),
                         camera.getFocalY(), camera.getTanFovy(), camera.getTanFovx(),
                         camera.getProjectionMatrix() * camera.getViewMatrix());
        //if get key c
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
            //cpu render
            splats.cpuRender(camera.getViewMatrix(), camera.getWidth(), camera.getHeight(), camera.getFocalX(),
                             camera.getFocalY(), camera.getTanFovy(), camera.getTanFovx(),
                             camera.getProjectionMatrix() * camera.getViewMatrix());
            return 0;
        }
        splats.display();
        //swap buffers
        glfwSwapBuffers(window);
        camera.getInput(window);

        //poll events
        glfwPollEvents();
        //check for escape key
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
        glQueryCounter(endTime, GL_TIMESTAMP);
        GLuint64 start, end;
        glGetQueryObjectui64v(startTime, GL_QUERY_RESULT, &start);
        glGetQueryObjectui64v(endTime, GL_QUERY_RESULT, &end);
        std::cout << "Frame took " << (end - start) / 1000000.0 << " milliseconds" << std::endl;
    }

    //close window
    glfwTerminate();
    return 0;
}
