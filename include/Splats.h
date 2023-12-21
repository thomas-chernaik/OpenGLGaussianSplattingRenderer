//
// Created by thomas on 11/12/23.
//
#include <GL/glew.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

#include "sort.h"



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
        void preprocess();

        //function to generate tile and depth based keys of splats
        void generateKeys();

        //function to sort the splats
        void sort();

        //function to draw the splats
        void draw(float* viewMatrix, float* projectionMatrix, float* lightPosition, float* lightColour, float* cameraPosition);



    private:
        //function to load splats from file
        void loadSplats(const std::string& filePath);

        //function to calculate if a splat is culled
        bool isCulled(int index, float* viewMatrix, float* projectionMatrix);

        int numSplats;
        int numSplatsPostCull;
        std::vector<float> means3D;
        std::vector<float> colours;
        std::vector<float> sphericalHarmonics;
        std::vector<float> opacities;
        std::vector<float> scales;
        float scaleModifier;
        std::vector<float> rotations;

        int workGroupSize;
        int numWorkGroups;



        //GPU buffers
        GLuint means3DBuffer;
        GLuint coloursBuffer;
        GLuint opacitiesBuffer;
        GLuint scalesBuffer;
        GLuint rotationsBuffer;
        GLuint indexBuffer;
        GLuint keyBuffer;
        GLuint intermediateBuffer;
        GLuint histogramBuffer;

        //not used for now
        GLuint sphericalHarmonicsBuffer;

        //shaders
        GLuint preProcessProgram;
        GLuint generateKeysProgram;
        GLuint sortProgram;
        GLuint histogramProgram;
        GLuint drawProgram;



};


#endif //DISS_SPLATS_H
