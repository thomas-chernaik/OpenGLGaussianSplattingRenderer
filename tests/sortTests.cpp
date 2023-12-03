//
// Created by thomas on 29/11/23.
//
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include "../utils.h"
#include "../sort.h"

//c++ implementation of the shader for debugging
void radixSort()
{
    //initialise two "buffers" (vectors) of random numbers
    std::vector<int> buffer = createRandomNumbersInt(256*16, 16);
    std::vector<int> outputBuffer = createRandomNumbersInt(256*16, 16);
    //initialise the "shared memory" (vector) for the local work group
    std::vector<int> sharedHistogram(256*16);
    for(int gl_LocalInvocationIndex=0; gl_LocalInvocationIndex < 256; gl_LocalInvocationIndex++)
    {
        int startIdx = gl_LocalInvocationIndex * 16;
        for(int i = startIdx; i < startIdx + 16; i++)
        {
            sharedHistogram[i] = 0;
        }
        int mask = 0x0000000F;
        //create the histogram for this "thread"
        int histogram[16];
        for(int i = 0; i < 16; i++)
        {
            histogram[i] = 0;
        }
        for(int i = startIdx; i < startIdx + 16; i++)
        {
            int index = buffer[i] & mask;
            histogram[index]++;
        }
        //write the histogram to the shared memory
        for(int i = startIdx; i < startIdx + 16; i++)
        {
            sharedHistogram[i] = histogram[i & mask];
        }
    }
    //"wait" for all threads to finish
    for(int gl_LocalInvocationIndex=0; gl_LocalInvocationIndex < 256; gl_LocalInvocationIndex++)
    {
        int startIdx = gl_LocalInvocationIndex * 16;
        //we need the local offset, from the start of the thread
        int offsets[16];
        //we need the global offset, from the start of the buffer
        int globalPrefixSum[16];
        for(int i = 0; i < 16; i++)
        {
            offsets[i] = 0;
            globalPrefixSum[i] = 0;
        }

        //compute the global prefix sum
        //for each thread
        int globalHistogram[16];
        for(int i = 0; i < 16; i++)
        {
            globalHistogram[i] = 0;
        }
        for(int i=0; i<256*16; i+=16)
        {
            for(int j=0; j<16; j++)
            {
                if(int(i/16) == gl_LocalInvocationIndex)
                {
                    offsets[j] = globalHistogram[j];
                }
                globalHistogram[j] += sharedHistogram[i+j];
            }
        }
        globalPrefixSum[0] = 0;
        for(int i = 1; i < 16; i++)
        {
            globalPrefixSum[i] = globalHistogram[i-1] + globalPrefixSum[i-1];
        }
        //write the output buffer
        for(int i = startIdx; i < startIdx + 16; i++)
        {
            int index = buffer[i] & 0x0000000F;
            outputBuffer[globalPrefixSum[index] + offsets[index]] = buffer[i];
            offsets[index]++;
        }

    }
    std::cout << "finished" << std::endl;
}

//create tests for the sort function
TEST(SortTest, SortTest) {
    //();
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
    createAndLinkSortShader();
    std::cout << "compiled shaders" << std::endl;

    //create buffer
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    //create output buffer
    GLuint outputBuffer;
    glGenBuffers(1, &outputBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputBuffer);
    //create random numbers
    std::vector<int> randomNumbers = createRandomNumbersInt(256*16, 16);
    //fill the input buffer with random numbers
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, randomNumbers.size() * sizeof(int), randomNumbers.data(), GL_STATIC_DRAW);
    int* bufferData = (int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    //fill the output buffer with the same random numbers
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, randomNumbers.size() * sizeof(int), randomNumbers.data(), GL_STATIC_DRAW);
    //int* outputBufferData = (int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    //run program
    GPURadixSort(buffer, outputBuffer, randomNumbers.size());
    //sort random numbers on cpu
    std::sort(randomNumbers.begin(), randomNumbers.end());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputBuffer);
    int* outputBufferData = (int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    //copy buffer data to vector
    std::vector<int> outputBufferVector(outputBufferData, outputBufferData + randomNumbers.size());
    //check buffer is sorted
    try{
        //get buffer data
        for (int i = 0; i < randomNumbers.size(); i++) {
            ASSERT_EQ(randomNumbers[i], outputBufferData[i]);
        }
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