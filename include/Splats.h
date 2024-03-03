//
// Created by thomas on 11/12/23.
//
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
        Splats(const std::string &filePath, int width, int height);
        //destructor
        ~Splats();

        //function to load the vectors to the GPU
        void loadToGPU(int width, int height);

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
        void draw(int width, int height, float tileWidth, float tileHeight);

        //function to display the splats
        void display();

        int numSplats;
        int numDuplicates;
        int numSplatsPostCull;
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

        void preprocessTemp(glm::mat4 viewMatrix, int width, int height, float focal_x, float focal_y, float tan_fov_x,
                            float tan_fov_y, glm::mat4 vpMatrix);

        //function to compute the 3D covariance matrix
        void computeCovarianceMatrices();
        glm::mat3x3 computeCovarianceMatrix(const glm::vec3 scale, const glm::vec4 rotation);
        //function to return the result of alphablending two colours
        glm::vec4 alphaBlend(glm::vec4 colour1, glm::vec4 colour2);




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

        GLuint depthBuffer;
        GLuint boundingRadiiBuffer;
        GLuint numDupedBuffer;
        GLuint splatKeysBuffer;


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
        GLuint binPrefixSumProgram;

        //simplified draw program
        GLuint simplifiedDrawProgram;


        //stuff for displaying
        GLuint vao;
        GLuint vbo;
        GLuint displayProgram;
        GLuint texture;

    //Debug functions
public:

    //function to print the splats's projected means
    void printProjectedMeans();
    //function to print the splat's projected means in order of indices
    void printProjectedMeansByIndex();

    void cpuRender(glm::mat4 viewMatrix, int width, int height, float focal_x, float focal_y, float tan_fov_x,
                   float tan_fov_y, glm::mat4 vpMatrix);
    void cpuProjectSplats(glm::mat4 vpMatrix, glm::mat3 rotationMatrix, int width, int height);
    void simplifiedDraw(glm::mat4 vpMatrix, glm::mat3 rotationMatrix, int width, int height);
    std::vector<glm::vec2> projectedMeans;
    std::vector<float> depthVector;
    std::vector<std::vector<glm::vec4>> image;
    std::vector<glm::vec3> conics;
    glm::vec3 get2DCovariance(glm::mat3 covarianceMatrix, glm::vec3 projectedMean, glm::mat3 rotationMatrix);
};


#endif //DISS_SPLATS_H
