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

    Splats splats("models/point_cloud.ply", camera.getWidth(), camera.getHeight());
    //splats.simplifiedDraw(camera.getProjectionMatrix() * camera.getViewMatrix(), camera.getRotationMatrix(), camera.getWidth(), camera.getHeight());
    //splats.printProjectedMeans();
    splats.cpuRender(camera.getViewMatrix(), camera.getWidth(), camera.getHeight(), camera.getFocalX(),
                     camera.getFocalY(), camera.getTanFovy(), camera.getTanFovx(),
                     camera.getProjectionMatrix() * camera.getViewMatrix());
    /*
    //render image
    //preprocess splats
    std::cout << "Preprocessing splats" << std::endl;

    splats.preprocess( camera.getProjectionMatrix() * camera.getViewMatrix(), camera.getRotationMatrix(), camera.getWidth(), camera.getHeight());
    //DEBUG: output the projected means

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
    std::cout << "Projected means" << std::endl;
    //splats.printProjectedMeansByIndex();
    //sort splats
    std::cout << "Sorting splats" << std::endl;
    splats.sort();
    //splats.printProjectedMeansByIndex();

    //compute bins
    std::cout << "Computing bins" << std::endl;
    splats.computeBins();


    //draw splats
    std::cout << "Drawing splats" << std::endl;
    glBeginQuery(GL_TIME_ELAPSED, timerQuery);
    splats.draw(camera.getWidth(), camera.getHeight());
    glEndQuery(GL_TIME_ELAPSED);
    glGetQueryObjectui64v(timerQuery, GL_QUERY_RESULT, &timeElapsed);
    std::cout << "Drawing splats took " << timeElapsed / 1000000.0 << " milliseconds" << std::endl;
*/
    GLuint startTime, endTime;
    glGenQueries(1, &startTime);
    glGenQueries(1, &endTime);
    //display window
    while (!glfwWindowShouldClose(window)) {
        //store start time with a GL_TIMESTAMP
        glQueryCounter(startTime, GL_TIMESTAMP);

//        glGenQueries(1, &startQuery);
//        glBeginQuery(GL_TIME_ELAPSED, startQuery);
        //resize window to camera size
        glfwSetWindowSize(window, camera.getWidth(), camera.getHeight());
        //set viewport
        glViewport(0, 0, camera.getWidth(), camera.getHeight());
        //clear window
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //display splats
        //splats.simplifiedDraw(camera.getProjectionMatrix() * camera.getViewMatrix(), camera.getRotationMatrix(), camera.getWidth(), camera.getHeight());

        splats.gpuRender(camera.getViewMatrix(), camera.getWidth(), camera.getHeight(), camera.getFocalX(),
                         camera.getFocalY(), camera.getTanFovy(), camera.getTanFovx(),
                         camera.getProjectionMatrix() * camera.getViewMatrix());
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

//        GLuint64 end;
//        glGetQueryObjectui64v(startQuery, GL_QUERY_RESULT, &end);
//        std::cout << "Frame took " << end / 1000000.0 << " milliseconds" << std::endl;
    }

    //close window
    glfwTerminate();
    return 0;
}
