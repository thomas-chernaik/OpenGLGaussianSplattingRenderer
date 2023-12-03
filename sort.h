//
// Created by thomas on 29/11/23.
//
#include "utils.h"
#include <cmath>
#include <vector>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifndef DISS_SORT_H
#define DISS_SORT_H
GLuint program;

//function to create and link the shader we will use later
void createAndLinkSortShader()
{
    std::cout << "compiling sorting shader" << std::endl;
    //read shader file
    std::string shaderCode = readShaderFile("sort.glsl");

    //create shader
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    const char *shaderCodePtr = shaderCode.c_str();
    glShaderSource(shader, 1, &shaderCodePtr, nullptr);
    glCompileShader(shader);

    //check shader compiled
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        // Compilation failed, print error log
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cout << "Shader compilation failed:\n" << infoLog << std::endl;
        glfwTerminate();
        return;
    }

    //create program and attach shader
    program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);

    //check program linked
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        std::cerr << "Failed to link program" << std::endl;
        glfwTerminate();
        return;
    }
    std::cout << "compiled and linked sorting shader" << std::endl;
}

void GPURadixSort(GLuint buffer, GLuint outputBuffer, int size)
{


    //get number of work groups
    int workGroupCount = ceil(size / 128.0f);
    int currentTime = glfwGetTime();

    //run shader
    glUseProgram(program);
    //bind buffer
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, outputBuffer);
    glDispatchCompute(1, 1, 1);
    //wait for shader to finish
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    int timeTaken = glfwGetTime() - currentTime;
    //print time taken
    std::cout << "Time taken: " << timeTaken << " seconds" << std::endl;

}

#endif //DISS_SORT_H
