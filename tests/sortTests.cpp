//
// Created by thomas on 29/11/23.
//
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <glm/vec2.hpp>
#include "utils.h"
#include "sort.h"

//c++ implementation of the shader for debugging
/*
void radixSort()
{
    int sizeOfBuffer = pow(2,30);
    int numThreads = 256;
    //initialise two "buffers" (vectors) of random numbers
    std::vector<int> buffer = createRandomNumbersInt(sizeOfBuffer, 500000);
    std::vector<int> outputBuffer = createRandomNumbersInt(sizeOfBuffer, 16);
    //initialise the "global memory" (vector) for the local work group
    std::vector<int> globalHistograms(numThreads * 16);

    int numSegments = numThreads;
    int segmentSize = ceil(sizeOfBuffer / numSegments);
    int keySize = 16;
    for(int segment = 0; segment < 8; segment++)
    {
        for (int gl_LocalInvocationIndex = 0; gl_LocalInvocationIndex < numThreads; gl_LocalInvocationIndex++)
        {
            int startIdx = gl_LocalInvocationIndex * segmentSize;
            int startIdx2 = gl_LocalInvocationIndex * keySize;
            int mask = 0x0000000F << (segment * 4);

            //create the histogram for values between startIdx and startIdx + segmentSize
            int histogram[16];
            for (int i = 0; i < 16; i++)
            {
                histogram[i] = 0;
            }
            for (int i = startIdx; i < startIdx + segmentSize; i++)
            {
                int key = (buffer[i] & mask) >> (segment * 4);
                histogram[key]++;
            }

            //write the histogram to global memory, from startIdx2 to startIdx2 + keySize
            for (int i = 0; i < 16; i++)
            {
                globalHistograms[startIdx2 + i] = histogram[i];
            }

            //wait for all threads to finish writing to global memory
            //barrier();
        }
        for (int gl_LocalInvocationIndex = 0; gl_LocalInvocationIndex < numThreads; gl_LocalInvocationIndex++)
        {
            int startIdx = gl_LocalInvocationIndex * segmentSize;
            int startIdx2 = gl_LocalInvocationIndex * keySize;
            int mask = 0x0000000F << (segment * 4);

            int offsets[keySize];
            int globalHistogram[16];
            //sum up all the histograms from all the threads
            for (int i = 0; i < 16; i++)
            {
                globalHistogram[i] = 0;
            }
            for (int i = 0; i < numSegments; i++)
            {
                for (int j = 0; j < 16; j++)
                {
                    //if we are at our thread, store the offset
                    if (i == gl_LocalInvocationIndex)
                    {
                        offsets[j] = globalHistogram[j];
                    }
                    globalHistogram[j] += globalHistograms[i * keySize + j];
                }
            }

            //now calculate the prefix sum of the global histogram
            int prefixSum[16];
            prefixSum[0] = 0;
            for (int i = 1; i < 16; i++)
            {
                prefixSum[i] = prefixSum[i - 1] + globalHistogram[i - 1];
            }

            //now write the sorted values to the output buffer
            for (int i = startIdx; i < startIdx + segmentSize; i++)
            {
                int key = (buffer[i] & mask) >> (segment * 4);
                int sortedIdx = prefixSum[key] + offsets[key];
                outputBuffer[sortedIdx] = buffer[i];
                offsets[key]++;
            }
        }
        //swap the buffers
        buffer = outputBuffer;
    }
    std::cout << "finished" << std::endl;
    //check buffer is sorted
    try{
        //get buffer data
        for (int i = 0; i < buffer.size() - 1; i++) {
            if (outputBuffer[i] > outputBuffer[i + 1]) {
                std::cerr << "Error: buffer is not sorted" << std::endl;
                return;
            }
        }
        std::cout << "Buffer is sorted" << std::endl;
    } catch (std::exception& e) {
        std::cerr << "Error: buffer is not sorted" << std::endl;
        return;
    }
}*/

void sortVec2(std::vector<glm::vec2> list)
{
    //cpu sort to check
    std::sort(list.begin(), list.end(), [](glm::vec2 a, glm::vec2 b) {
        return a.y < b.y;
    });}

//create tests for the sort function
TEST(SortTest, SortTest) {
    //start timer
    double startTime = glfwGetTime();
    //radixSort();
    std::cout << "CPU sort took " << glfwGetTime() - startTime << " seconds" << std::endl;

    //intialise glfw
    if (!glfwInit()) {
        std::cerr << "Failed to initialise GLFW" << std::endl;
        //fail test
        FAIL();
    }

    //set error callback
    glfwSetErrorCallback(errorCallback);

    //create window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Hello World", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        FAIL();
    }

    //make window current context
    glfwMakeContextCurrent(window);

    //initialise glew
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialise GLEW" << std::endl;
        glfwTerminate();
        FAIL();
    }

    std::cout << "compiling shaders" << std::endl;
    //createAndLinkSortShader();
    GLuint histogramProgram, sortProgram, sumProgram, paddingProgram;
    createAndLinkSortAndHistogramShaders(histogramProgram, sortProgram, sumProgram);
    std::cout << "compiled shaders" << std::endl;

    //create buffer
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    //create output buffer
    GLuint intermediateBuffer;
    glGenBuffers(1, &intermediateBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, intermediateBuffer);
    //create order buffer
    GLuint orderBuffer;
    glGenBuffers(1, &orderBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, orderBuffer);
    //create random numbers

    std::vector<float> randomNumbers = createRandomNumbersFloat(32*16*10000-7);
    //print the max number
    //std::cout << "max number: " << *std::max_element(randomNumbers.begin(), randomNumbers.end()) << std::endl;
    //fill the input buffer with random numbers
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, randomNumbers.size() * sizeof(float), randomNumbers.data(), GL_STATIC_DRAW);
    int* bufferData = (int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);

    //fill the order buffer with ascending numbers
    //generate ascending numbers
    std::vector<int> ascendingNumbers(randomNumbers.size());
    for(int i = 0; i < randomNumbers.size(); i++) {
        ascendingNumbers[i] = i;
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, orderBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ascendingNumbers.size() * sizeof(int), ascendingNumbers.data(), GL_STATIC_DRAW);
    //fill the output buffer with the same ascending numbers
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, intermediateBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ascendingNumbers.size() * sizeof(int), nullptr, GL_STATIC_DRAW);

    //create the histogram buffer to use later
    int histogramSize = 16 * 8*256 + 16;
    GLuint histogramBuffer;
    glGenBuffers(1, &histogramBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, histogramSize * sizeof(int), nullptr, GL_STATIC_DRAW);



    //int* outputBufferData = (int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    //run program
    //GPURadixSort(buffer, intermediateBuffer, randomNumbers.size());
    int size = randomNumbers.size();
    double currentTime = glfwGetTime();
    GPURadixSort(histogramProgram, sumProgram, sortProgram, intermediateBuffer, orderBuffer, histogramBuffer,
                 size, 16, 32, buffer);
    glFinish();
    std::cout << "GPU sort took " << glfwGetTime() - currentTime << " seconds" << std::endl;

//    //deep copy random numbers
    std::vector<float> randomNumbersCopy(randomNumbers);

    //sort random numbers on cpu
    currentTime = glfwGetTime();
    //cpu sort to check
    std::sort(randomNumbers.begin(), randomNumbers.end(), [](float a, float b) {
        return a < b;
    });
    std::cout << "CPU sort took " << glfwGetTime() - currentTime << " seconds" << std::endl;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, orderBuffer);
    int* outputBufferData = (int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    //copy buffer data to vector
    std::vector<int> outputBufferVector(outputBufferData, outputBufferData + size);
    //find the number 44
    //std::cout << "number 44 is at index " << std::find(outputBufferVector.begin(), outputBufferVector.end(), 44) - outputBufferVector.begin() << std::endl;
    //check buffer is sorted
    try{
        int errors = 0;
        //get buffer data
        for (int i = 1; i < size; i++) {
            ASSERT_GE(randomNumbersCopy[outputBufferVector[i]], randomNumbersCopy[outputBufferVector[i - 1]]);
           ASSERT_EQ(randomNumbersCopy[outputBufferVector[i]], randomNumbers[i]);

        }
        std::cout << "Successfully sorted " << randomNumbers.size() << " numbers" << std::endl;
    } catch (std::exception& e) {
        //fail test
        FAIL();

    }
    //pass test
    SUCCEED();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}