//
// Created by thomas on 29/11/23.
//
#include "utils.h"
#include <cmath>
#include <vector>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifndef DISS_SORT_H
#define DISS_SORT_H

//function to create and link the shaders we will use later
void createAndLinkSortAndHistogramShaders(GLuint &histogramProgram, GLuint &sortProgram, GLuint &sumProgram);

//function to create and link the shader we will use later
void GPURadixSort(GLuint histogramProgram, GLuint prefixSumProgram, GLuint sortProgram, GLuint intermediateBuffer,
                  GLuint orderBuffer, GLuint histogramBuffer, int size, int workGroupCount, int workGroupSize,
                  GLuint buffer);


int PadBuffer(int size, int unitWidth);

#endif //DISS_SORT_H
