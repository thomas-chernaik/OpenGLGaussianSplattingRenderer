#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
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

    //render image
    //preprocess splats
    std::cout << "Preprocessing splats" << std::endl;
    splats.preprocess();
    //sort splats
    std::cout << "Sorting splats" << std::endl;
    splats.sort();
    //count tile sizes
    std::cout << "Counting tile sizes" << std::endl;
    splats.countTileSizes();
    //draw splats
    std::cout << "Drawing splats" << std::endl;
    splats.draw(nullptr, nullptr, nullptr, nullptr, nullptr);

    //close window
    glfwTerminate();
    return 0;
}
