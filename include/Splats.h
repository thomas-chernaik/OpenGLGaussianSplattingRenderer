//
// Created by thomas on 11/12/23.
//
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <array>
#include <bitset>


#include "sort.h"
#include "utils.h"




#ifndef DISS_SPLATS_H
#define DISS_SPLATS_H


class Splats
{
    public:
        //constructor with file path
        Splats(const std::string &filePath, int width, int height);
        //destructor
        ~Splats();

        //function to load the vectors to the GPU
        void loadToGPU(int width, int height);

        //function to load the shaders
        void loadShaders();

        //function to sort the splats
        void sort();

        //function to compute the bins of the splats
        void computeBins();

        //function to draw the splats
        void draw(int width, int height, float tileWidth, float tileHeight);

        //function to display the splats
        void display();

        int numSplats{};
        int numDuplicates{};
        std::vector<glm::vec4> means3D;
        std::vector<glm::vec4> colours;
        std::vector<float> sphericalHarmonics;
        std::vector<float> opacities;
        std::vector<glm::vec3> scales;
        std::vector<glm::vec4> rotations;
        std::vector<float> covarianceMatrices;



    private:
        //function to load splats from file
        void loadSplats(const std::string& filePath);

        void preprocess(glm::mat4 viewMatrix, int width, int height, float focal_x, float focal_y, float tan_fov_x,
                        float tan_fov_y, glm::mat4 vpMatrix);

        //function to compute the 3D covariance matrix
        void computeCovarianceMatrices();
        static glm::mat3x3 computeCovarianceMatrix(glm::vec3 scale, glm::vec4 rotation);
        //function to return the result of alpha blending two colours
        static glm::vec4 alphaBlend(glm::vec4 colour1, glm::vec4 colour2);





        //GPU buffers
        GLuint means3DBuffer{};
        GLuint coloursBuffer{};
        GLuint opacitiesBuffer{};
        GLuint CovarianceBuffer{};
        GLuint projectedMeansBuffer{};
        GLuint projectedCovarianceBuffer{};
        GLuint indexBuffer{};
        GLuint intermediateBuffer{};
        GLuint histogramBuffer{};
        GLuint binsBuffer{};

        GLuint depthBuffer{};
        GLuint numDupedBuffer{};
        GLuint splatKeysBuffer{};


        //shaders
        GLuint preProcessProgram{};
        GLuint sortProgram{};
        GLuint histogramProgram{};
        GLuint prefixSumProgram{};
        GLuint drawProgram{};
        GLuint binProgram{};
        GLuint binPrefixSumProgram{};

        //stuff for displaying
        GLuint vao{};
        GLuint vbo{};
        GLuint displayProgram{};
        GLuint texture{};

    //Debug functions
public:


    void cpuRender(glm::mat4 viewMatrix, int width, int height, float focal_x, float focal_y, float tan_fov_x,
                   float tan_fov_y, glm::mat4 vpMatrix);
    void gpuRender(glm::mat4 viewMatrix, int width, int height, float focal_x, float focal_y, float tan_fov_x,
                   float tan_fov_y, glm::mat4 vpMatrix);
};


#endif //DISS_SPLATS_H
