//
// Created by thomas on 11/12/23.
//
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <stdlib.h>
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
        Splats(const std::string& filePath);
        //destructor
        ~Splats();

        //function to load the vectors to the GPU
        void loadToGPU();

        //function to load the shaders
        void loadShaders();

        //function to preprocess the splats
        void preprocess(glm::mat4 vpMatrix, glm::mat3 rotationMatrix, int width, int height);

        //function to generate tile and depth based keys of splats
        void duplicateKeys();



        //function to sort the splats
        void sort();

        //function to compute the bins of the splats
        void computeBins();

        //function to draw the splats
        void draw(float *viewMatrix, float *projectionMatrix, float *lightPosition, float *lightColour,
                  float *cameraPosition, int width, int height);

        //function to display the splats
        void display();



    private:
        //function to load splats from file
        void loadSplats(const std::string& filePath);

        //function to compute the 3D covariance matrix
        void computeCovarianceMatrices();
        glm::mat3x3 computeCovarianceMatrix(const glm::vec3 scale, const glm::vec4 rotation);


        int numSplats;
        int numSplatsPostCull;
        std::vector<glm::vec3> means3D;
        std::vector<glm::vec3> colours;
        std::vector<float> sphericalHarmonics;
        std::vector<float> opacities;
        std::vector<glm::vec3> scales;
        std::vector<glm::vec4> rotations;
        std::vector<std::array<float, 6>> covarianceMatrices;

        int workGroupSize;
        int numWorkGroups;



        //GPU buffers
        GLuint means3DBuffer;
        GLuint coloursBuffer;
        GLuint opacitiesBuffer;
        GLuint CovarianceBuffer;
        GLuint projectedMeansBuffer;
        GLuint projectedCovarianceBuffer;
        GLuint indexBuffer;
        GLuint keyBuffer;
        GLuint intermediateBuffer;
        GLuint histogramBuffer;
        GLuint binsBuffer;


        //not used for now
        GLuint sphericalHarmonicsBuffer;

        //shaders
        GLuint preProcessProgram;
        GLuint generateKeysProgram;
        GLuint sortProgram;
        GLuint histogramProgram;
        GLuint prefixSumProgram;
        GLuint drawProgram;
        GLuint binProgram;

        //stuff for displaying
        GLuint vao;
        GLuint vbo;
        GLuint displayProgram;
        GLuint texture;



};


#endif //DISS_SPLATS_H
