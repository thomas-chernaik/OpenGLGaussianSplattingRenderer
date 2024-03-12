//
// Created by thomas on 02/02/24.
//

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <gtest/gtest.h>
#include <vector>
#include "Splats.h"


// Function to load up the bins for the model
std::vector<glm::vec3> loadBins(const std::string& filePath)
{
    // Create a vector to store the bins
    std::vector<glm::vec3> bins;
    // Open the file
    std::ifstream file(filePath);
    // the file is stored as binary floats
    float x, y, z;
    // Read the file
    while (file.read(reinterpret_cast<char*>(&x), sizeof(float)))
    {
        file.read(reinterpret_cast<char*>(&y), sizeof(float));
        file.read(reinterpret_cast<char*>(&z), sizeof(float));
        bins.push_back(glm::vec3(x, y, z));
    }
    return bins;
}

std::vector<float> loadBinsf(const std::string& filePath)
{
    // Create a vector to store the bins
    std::vector<float> bins;
    // Open the file
    std::ifstream file(filePath);
    // the file is stored as binary floats
    float x;
    // Read the file
    while (file.read(reinterpret_cast<char*>(&x), sizeof(float)))
    {
        bins.push_back(x);
    }
    return bins;
}
std::vector<glm::vec4> loadBins4(const std::string& filePath)
{
    // Create a vector to store the bins
    std::vector<glm::vec4> bins;
    // Open the file
    std::ifstream file(filePath);
    // the file is stored as binary floats
    float x, y, z, w;
    // Read the file
    while (file.read(reinterpret_cast<char *>(&x), sizeof(float)))
    {
        file.read(reinterpret_cast<char *>(&y), sizeof(float));
        file.read(reinterpret_cast<char *>(&z), sizeof(float));
        file.read(reinterpret_cast<char *>(&w), sizeof(float));
        bins.push_back(glm::vec4(x, y, z, w));
    }
    return bins;
}

TEST(SplatsTest, LoadFile)
{
    Splats splats("models/point_cloud.ply", 1000, 1000);
    ASSERT_EQ(splats.numSplats, 3616103);
    std::vector<glm::vec3> means = loadBins("models/test/means.bin");
    // Compare to the means of the splats
    for (int i = 0; i < splats.numSplats; i++)
    {
        for(int j=0; j<3; j++)
        {
            ASSERT_FLOAT_EQ(splats.means3D[i][j], means[i][j]);
        }
    }
    std::vector<float> opacities = loadBinsf("models/test/opacity.bin");
    // Compare to the opacities of the splats
    for (int i = 0; i < splats.numSplats; i++)
    {
        ASSERT_FLOAT_EQ(splats.opacities[i], opacities[i]);
    }
    std::vector<glm::vec3> scales = loadBins("models/test/scale.bin");
    // Compare to the scales of the splats
    for (int i = 0; i < splats.numSplats; i++)
    {
        for(int j=0; j<3; j++)
        {
            ASSERT_FLOAT_EQ(splats.scales[i][j], scales[i][j]);
        }
    }
    std::vector<glm::vec4> rotations = loadBins4("models/test/rot.bin");
    // Compare to the rotations of the splats
    for (int i = 0; i < splats.numSplats; i++)
    {
        for(int j=0; j<4; j++)
        {
            ASSERT_FLOAT_EQ(splats.rotations[i][j], rotations[i][j]);
        }
    }
}

TEST(SplatsTest, LoadSimplePly)
{
    Splats splats("models/testSingleItem.ply", 100, 100);
    ASSERT_EQ(splats.numSplats, 1);
}