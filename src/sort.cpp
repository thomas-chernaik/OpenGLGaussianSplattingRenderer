//
// Created by thomas on 29/11/23.
//
#include <cmath>
#include <vector>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "utils.h"
#include "sort.h"


#ifndef DISS_SORT_CPP
#define DISS_SORT_CPP

//function to create and link the shaders we will use later
void createAndLinkSortAndHistogramShaders(GLuint &histogramProgram, GLuint &sortProgram, GLuint &sumProgram)
{
    std::cout << "compiling sorting shaders" << std::endl;
    //read shader file for histogram
    std::string histogramShaderCode = readShaderFile("shaders/generateHistograms.glsl");

    //create histogram shader
    GLuint histogramShader = glCreateShader(GL_COMPUTE_SHADER);
    const char *histogramShaderCodePtr = histogramShaderCode.c_str();
    glShaderSource(histogramShader, 1, &histogramShaderCodePtr, nullptr);
    glCompileShader(histogramShader);

    //check histogram shader compiled
    GLint success;
    glGetShaderiv(histogramShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        // Compilation failed, print error log
        char infoLog[512];
        glGetShaderInfoLog(histogramShader, 512, nullptr, infoLog);
        std::cout << "Shader compilation failed:\n" << infoLog << std::endl;
        glfwTerminate();
        return;
    }

    //create histogram program and attach shader
    histogramProgram = glCreateProgram();
    glAttachShader(histogramProgram, histogramShader);
    glLinkProgram(histogramProgram);

    //check histogram program linked
    glGetProgramiv(histogramProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        std::cerr << "Failed to link histogram program" << std::endl;
        glfwTerminate();
        return;
    }

    //read shader file for sort
    std::string sortShaderCode = readShaderFile("shaders/scan.glsl");

    //create sort shader
    GLuint sortShader = glCreateShader(GL_COMPUTE_SHADER);
    const char *sortShaderCodePtr = sortShaderCode.c_str();
    glShaderSource(sortShader, 1, &sortShaderCodePtr, nullptr);
    glCompileShader(sortShader);

    //check sort shader compiled
    glGetShaderiv(sortShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        // Compilation failed, print error log
        char infoLog[512];
        glGetShaderInfoLog(sortShader, 512, nullptr, infoLog);
        std::cout << "Shader compilation failed:\n" << infoLog << std::endl;
        glfwTerminate();
        return;
    }

    //create sort program and attach shader
    sortProgram = glCreateProgram();
    glAttachShader(sortProgram, sortShader);
    glLinkProgram(sortProgram);

    //check sort program linked
    glGetProgramiv(sortProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        std::cerr << "Failed to link sort program" << std::endl;
        glfwTerminate();
        return;
    }

    //read shader file for sum
    std::string sumShaderCode = readShaderFile("shaders/computePrefixSum.glsl");

    //create sum shader
    GLuint sumShader = glCreateShader(GL_COMPUTE_SHADER);
    const char *sumShaderCodePtr = sumShaderCode.c_str();
    glShaderSource(sumShader, 1, &sumShaderCodePtr, nullptr);
    glCompileShader(sumShader);

    //check sum shader compiled
    glGetShaderiv(sumShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        // Compilation failed, print error log
        char infoLog[512];
        glGetShaderInfoLog(sumShader, 512, nullptr, infoLog);
        std::cout << "Shader compilation failed:\n" << infoLog << std::endl;
        glfwTerminate();
        return;
    }

    //create sum program and attach shader
    sumProgram = glCreateProgram();
    glAttachShader(sumProgram, sumShader);
    glLinkProgram(sumProgram);

    //check sum program linked
    glGetProgramiv(sumProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        std::cerr << "Failed to link sum program" << std::endl;
        glfwTerminate();
        return;
    }

    std::cout << "compiled and linked sorting shaders" << std::endl;
}


//function to create and link the shader we will use later
void createAndLinkSortShader(GLuint &program)
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

void GPURadixSort2(GLuint histogramProgram, GLuint prefixSumProgram, GLuint sortProgram, GLuint buffer,
                   GLuint intermediateBuffer, GLuint orderBuffer, GLuint histogramBuffer, int size, int workGroupCount,
                   int workGroupSize)
{
    //create query object
    GLuint startTimeQuery, endTimeQuery;
    glGenQueries(1, &startTimeQuery);
    glGenQueries(1, &endTimeQuery);
    GLint64 startTime, endTime;
    int numberOfSections = workGroupCount * workGroupSize;
    int sectionSize = size / numberOfSections;

    //start the timer
    glQueryCounter(startTimeQuery, GL_TIMESTAMP);

    //run the sort
    for(int i=0; i<7; i++)
    {
        //generate the histograms
        glUseProgram(histogramProgram);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, orderBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, histogramBuffer);
        glUniform1i(glGetUniformLocation(histogramProgram, "sectionSize"), sectionSize);
        glUniform1i(glGetUniformLocation(histogramProgram, "numSections"), numberOfSections);
        glUniform1i(glGetUniformLocation(histogramProgram, "segmentReadOnly"), i);
        glDispatchCompute(workGroupCount, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        //calculate the prefix sum
        glUseProgram(prefixSumProgram);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, histogramBuffer);
        glUniform1i(glGetUniformLocation(prefixSumProgram, "numSections"), numberOfSections);
        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


        //scan the histograms
        glUseProgram(sortProgram);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, intermediateBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, histogramBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, orderBuffer);
        glUniform1i(glGetUniformLocation(sortProgram, "sectionSize"), sectionSize);
        glUniform1i(glGetUniformLocation(sortProgram, "numSections"), numberOfSections);
        glUniform1i(glGetUniformLocation(sortProgram, "segmentReadOnly"), i);
        glDispatchCompute(workGroupCount, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        //swap the output and order buffers
        GLuint temp = orderBuffer;
        orderBuffer = intermediateBuffer;
        intermediateBuffer = temp;
    }
    //intermediateBuffer;
    //stop the timer
    glQueryCounter(endTimeQuery, GL_TIMESTAMP);

    //get time taken
    glGetQueryObjecti64v(startTimeQuery, GL_QUERY_RESULT, &startTime);
    glGetQueryObjecti64v(endTimeQuery, GL_QUERY_RESULT, &endTime);

    double timeTaken = (endTime - startTime) / 1000000000.f;
    std::cout << "GPU sort took " << timeTaken << " seconds" << std::endl;
}

void GPURadixSort(GLuint program, GLuint buffer, GLuint outputBuffer, int size)
{
    //create query object
    GLuint startTimeQuery, endTimeQuery;
    glGenQueries(1, &startTimeQuery);
    glGenQueries(1, &endTimeQuery);
    GLint64 startTime, endTime;

    //measure GPU time




    //get number of work groups
    int workGroupCount = 4;
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
