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
    //create query object
    GLuint startTimeQuery, endTimeQuery;
    glGenQueries(1, &startTimeQuery);
    glGenQueries(1, &endTimeQuery);
    GLint64 startTime, endTime;

    //measure GPU time




    //get number of work groups
    int workGroupCount = 1;
    //work out the section size and number of sections
    int workGroupSize = 256;
    int numberOfSections = workGroupCount * workGroupSize;
    int sectionSize = size / numberOfSections;
    //use the shader program
    glUseProgram(program);

    //bind input and output buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, outputBuffer);

    //create and bind the shared histogram buffer, of length 16 * number of sections
    int histogramSize = 16 * numberOfSections;
    GLuint histogramBuffer;
    glGenBuffers(1, &histogramBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, histogramSize     * sizeof(int), nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, histogramBuffer);

    //set the uniform variables
    glUniform1i(glGetUniformLocation(program, "sectionSize"), sectionSize);
    glUniform1i(glGetUniformLocation(program, "numSections"), numberOfSections);


    glQueryCounter(startTimeQuery, GL_TIMESTAMP);
    glDispatchCompute(workGroupCount, 1, 1);
    //wait for shader to finish
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glQueryCounter(endTimeQuery, GL_TIMESTAMP);

    //get time taken
    glGetQueryObjecti64v(startTimeQuery, GL_QUERY_RESULT, &startTime);
    glGetQueryObjecti64v(endTimeQuery, GL_QUERY_RESULT, &endTime);

    double timeTaken = (endTime - startTime) / 1000000000.f;


    //print time taken. Do not round down to 0
    std::cout << "GPU sort took " << timeTaken << " seconds" << std::endl;

}

#endif //DISS_SORT_H
