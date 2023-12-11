//
// Created by thomas on 29/11/23.
//
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#ifndef DISS_UTILS_H
#define DISS_UTILS_H

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
std::vector<uint64_t> createRandomNumbersInt(int size, uint64_t maxNumber) {
    if(size < 1) {
        std::cerr << "Error: size must be greater than 0" << std::endl;
        return std::vector<uint64_t>();
    }
    std::vector<uint64_t> randomNumbers(size);
    for (int i = 0; i < size; i++) {
        randomNumbers[i] = static_cast<uint64_t>(rand() / 10000000.f * maxNumber);
    }
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


#endif //DISS_UTILS_H
