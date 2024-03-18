//
// Created by thomas on 29/11/23.
//
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


int PadBuffer(int size, int unitWidth)
{
    //calculate the amount of padding needed
    if(size % unitWidth == 0)
    {
        return 0;
    }
    int padding = unitWidth - (size % unitWidth);

    return padding;
}

void GPURadixSort(GLuint histogramProgram, GLuint prefixSumProgram, GLuint sortProgram, GLuint intermediateBuffer,
                  GLuint orderBuffer, GLuint histogramBuffer, int size, int workGroupCount, int workGroupSize,
                  GLuint buffer)
{

    int numberOfSections = workGroupCount * workGroupSize;
    int paddingSize = numberOfSections;

    int paddedSize = size + PadBuffer(size, paddingSize);

    int sectionSize = paddedSize / numberOfSections;
    if (paddedSize % paddingSize != 0)
    {
        std::cerr << "Size must be a multiple of " << paddingSize << std::endl;
        return;
    }


    //run the sort
    for (int i = 0; i < 8; i++)
    {
        //generate the histograms
        glUseProgram(histogramProgram);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, orderBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, histogramBuffer);
        glUniform1i(glGetUniformLocation(histogramProgram, "sectionSize"), sectionSize);
        glUniform1i(glGetUniformLocation(histogramProgram, "numSections"), numberOfSections);
        glUniform1i(glGetUniformLocation(histogramProgram, "segmentReadOnly"), i);
        glUniform1i(glGetUniformLocation(histogramProgram, "bufferSize"), size);
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
        glUniform1i(glGetUniformLocation(histogramProgram, "bufferSize"), size);
        glDispatchCompute(workGroupCount, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        //swap the output and order buffers
        GLuint temp = orderBuffer;
        orderBuffer = intermediateBuffer;
        intermediateBuffer = temp;
    }
    //swap the output and order buffers
    GLuint temp = orderBuffer;
    orderBuffer = intermediateBuffer;
    intermediateBuffer = temp;

}

#endif //DISS_SORT_CPP
