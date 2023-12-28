//
// Created by thomas on 29/11/23.
//
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include "utils.h"

#ifndef UTILS_CPP
#define UTILS_CPP

std::string readShaderFile(const std::string& filePath) {
    std::ifstream fileStream(filePath);
    if (!fileStream.is_open()) {
        std::cerr << "Failed to open shader file: " << filePath << std::endl;
        return "";
    }

    std::ostringstream shaderCode;
    shaderCode << fileStream.rdbuf();
    return shaderCode.str();
}

void errorCallback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}

std::vector<double> createRandomNumbersDouble(int size, int maxNumber) {
    if(size < 1) {
        std::cerr << "Error: size must be greater than 0" << std::endl;
        return std::vector<double>();
    }
    std::vector<double> randomNumbers(size);
    for (int i = 0; i < size; i++) {
        randomNumbers[i] = (double)(rand() / maxNumber);
    }
    return randomNumbers;
}

//function to create random numbers to go in a buffer
//return a vector floats
std::vector<float> createRandomNumbersFloat(int size) {
    if(size < 1) {
        std::cerr << "Error: size must be greater than 0" << std::endl;
        return std::vector<float>();
    }
    std::vector<float> randomNumbers(size);
    for (int i = 0; i < size; i++) {
        randomNumbers[i] = (float)rand() / RAND_MAX;
    }
    return randomNumbers;
}
std::vector<int> createRandomNumbersInt(int size, int maxNumber) {
    std::vector<int> randomNumbers(size);
    std::random_device rd;
    std::mt19937 gen = std::mt19937 (rd());
    std::uniform_int_distribution<int> dis(0, maxNumber - 1);

    std::generate(randomNumbers.begin(), randomNumbers.end(), [&]() {
        return dis(gen);
    });
    //bit shift to make the number 64 bits
    //for(int i = 0; i < randomNumbers.size(); i++) {
    //    randomNumbers[i] = randomNumbers[i] << 32;
    //    randomNumbers[i] = randomNumbers[i] | dis(gen);
    //}
    return randomNumbers;
}
//function to test a buffer is sorted
void isSorted(float* buffer, int size) {
    for (int i = 0; i < size - 1; i++) {
        if (buffer[i] > buffer[i + 1]) {
            std::cerr << "Error: buffer is not sorted" << std::endl;
            return;
        }
    }
    std::cout << "Buffer is sorted" << std::endl;
}

//function to load and link a shader
GLuint loadAndLinkShader(std::string shaderName)
{
    std::cout << "compiling " << shaderName << " shader" << std::endl;
    //read shader file
    std::string shaderCode = readShaderFile("shaders/" + shaderName + ".glsl");

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
        return -1;
    }

    //create program and attach shader
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);

    //check program linked
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        // Linking failed, print error log
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cout << "Shader linking failed:\n" << infoLog << std::endl;
        glfwTerminate();
        return -1;
    }
    std::cout << "compiled and linked " << shaderName << " shader" << std::endl;
    return program;
}


#endif //DISS_UTILS_H
