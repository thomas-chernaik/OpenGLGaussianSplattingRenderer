#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "sort.h"
#include "Splats.h"
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
    Splats splats("models/point_cloud.ply");
    //load splats into buffers
    splats.loadToGPU();
    //load shaders
    splats.loadShaders();
    //preprocess splats
    splats.preprocess();
    //generate keys
    splats.generateKeys();
    //sort splats
    splats.sort();
    //draw splats
    splats.draw(nullptr, nullptr, nullptr, nullptr, nullptr);




    return 0;
}
