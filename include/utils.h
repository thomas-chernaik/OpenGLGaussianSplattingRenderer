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
#include <glm/vec2.hpp>

#ifndef UTILS_H
#define UTILS_H

std::string readShaderFile(const std::string& filePath);

void errorCallback(int error, const char* description);
std::vector<double> createRandomNumbersDouble(int size, int maxNumber);


//function to create random numbers to go in a buffer
//return a vector floats
std::vector<float> createRandomNumbersFloat(int size);

std::vector<int> createRandomNumbersInt(int size, int maxNumber);

std::vector<glm::uvec2> createRandomNumbersVec2(int size, int maxNumber);


//function to test a buffer is sorted;

//function to load and link a shader
GLuint loadAndLinkShader(std::string shaderName);

GLuint loadAndLinkShaders(std::string shaderName);



#endif //DISS_UTILS_H
