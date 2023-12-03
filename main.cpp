#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "utils.h"
#include "sort.h"
#include <cmath>
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
    std::cout << "compiling shaders" << std::endl;
    createAndLinkSortShader();

    //create buffer
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);

    //create output buffer
    GLuint outputBuffer;
    glGenBuffers(1, &outputBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputBuffer);

    //create random numbers
    std::vector<float> randomNumbers = createRandomNumbersFloat(256*16);
    //fill the input buffer with random numbers
    glBufferData(GL_SHADER_STORAGE_BUFFER, randomNumbers.size() * sizeof(float), randomNumbers.data(), GL_STATIC_DRAW);
    //fill the output buffer with the same random numbers
    glBufferData(GL_SHADER_STORAGE_BUFFER, randomNumbers.size() * sizeof(float), randomNumbers.data(), GL_STATIC_DRAW);
    //run program
    GPURadixSort(buffer, randomNumbers.size());




    return 0;
}
