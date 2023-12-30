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
    Camera camera(0.0f, 0.0f, -10.0f);
    camera.update();

    Splats splats("models/point_cloud.ply");

    //render image
    //preprocess splats
    std::cout << "Preprocessing splats" << std::endl;

    splats.preprocess( camera.getProjectionMatrix() * camera.getViewMatrix(), camera.getRotationMatrix(), camera.getWidth(), camera.getHeight());

    //duplicate splats
    //start timer query
    glBeginQuery(GL_TIME_ELAPSED, timerQuery);

    std::cout << "Duplicating splats, generating keys" << std::endl;
    splats.duplicateKeys();
    //end timer query
    glEndQuery(GL_TIME_ELAPSED);
    //get time
    GLuint64 timeElapsed;
    glGetQueryObjectui64v(timerQuery, GL_QUERY_RESULT, &timeElapsed);
    std::cout << "Duplicating splats took " << timeElapsed / 1000000.0 << " milliseconds" << std::endl;

    //sort splats
    std::cout << "Sorting splats" << std::endl;
    splats.sort();

    //draw splats
    std::cout << "Drawing splats" << std::endl;
    splats.draw(nullptr, nullptr, nullptr, nullptr, nullptr);

    //close window
    glfwTerminate();
    return 0;
}
