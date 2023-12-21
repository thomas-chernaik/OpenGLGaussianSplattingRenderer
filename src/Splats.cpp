//
// Created by thomas on 11/12/23.
//

#include "Splats.h"

Splats::Splats(const std::string &filePath)
{
    loadSplats(filePath);
}

Splats::~Splats()
{

}

void Splats::loadToGPU()
{
    //create the buffers
    glGenBuffers(1, &means3DBuffer);
    glGenBuffers(1, &coloursBuffer);
    glGenBuffers(1, &sphericalHarmonicsBuffer);
    glGenBuffers(1, &opacitiesBuffer);
    glGenBuffers(1, &scalesBuffer);
    glGenBuffers(1, &rotationsBuffer);
    glGenBuffers(1, &indexBuffer);
    glGenBuffers(1, &keyBuffer);
    //bind the buffers
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, means3DBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, coloursBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphericalHarmonicsBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, opacitiesBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, scalesBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rotationsBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, keyBuffer);
    //load the data into the buffers
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * means3D.size(), means3D.data(), GL_STATIC_DRAW);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * colours.size(), colours.data(), GL_STATIC_DRAW);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * sphericalHarmonics.size(), sphericalHarmonics.data(), GL_STATIC_DRAW);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * opacities.size(), opacities.data(), GL_STATIC_DRAW);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * scales.size(), scales.data(), GL_STATIC_DRAW);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * rotations.size(), rotations.data(), GL_STATIC_DRAW);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * numSplats, nullptr, GL_STATIC_DRAW);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * numSplats, nullptr, GL_STATIC_DRAW);
}

void Splats::loadShaders()
{
    //load the sort and histogram shaders
    createAndLinkSortAndHistogramShaders(histogramProgram, sortProgram);
    //load the preprocess shader
    preProcessProgram = createShaderProgram("shaders/preprocess");
}

void Splats::loadSplats(const std::string &filePath)
{
    //the file is stored in ply format
    //the number of splats is in the 3rd line after element vertex
    //the data is stored as follows
    /*property float x
        property float y
        property float z - the mean of the splat
        property float nx
        property float ny
        property float nz - the normal of the splat, not sure what this is used for
        property float f_dc_0
        property float f_dc_1
        property float f_dc_2 - the spherical harmonics of the splat with the diffuse colour
        property float f_rest_0
        property float f_rest_1
        property float f_rest_2
        property float f_rest_3
        property float f_rest_4
        property float f_rest_5
        property float f_rest_6
        property float f_rest_7
        property float f_rest_8
        property float f_rest_9
        property float f_rest_10
        property float f_rest_11
        property float f_rest_12
        property float f_rest_13
        property float f_rest_14
        property float f_rest_15
        property float f_rest_16
        property float f_rest_17
        property float f_rest_18
        property float f_rest_19
        property float f_rest_20
        property float f_rest_21
        property float f_rest_22
        property float f_rest_23
        property float f_rest_24
        property float f_rest_25
        property float f_rest_26
        property float f_rest_27
        property float f_rest_28
        property float f_rest_29
        property float f_rest_30
        property float f_rest_31
        property float f_rest_32
        property float f_rest_33
        property float f_rest_34
        property float f_rest_35
        property float f_rest_36
        property float f_rest_37
        property float f_rest_38
        property float f_rest_39
        property float f_rest_40
        property float f_rest_41
        property float f_rest_42
        property float f_rest_43
        property float f_rest_44 - the precomputed spherical harmonics of the splat (copilot written)
        property float opacity - the opacity of the splat
        property float scale_0
        property float scale_1
        property float scale_2 - the scale of the splat
        property float rot_0
        property float rot_1
        property float rot_2
        property float rot_3 - the rotation of the splat
     */
    //open the file at filePath
    std::fstream file(filePath, std::ios::in | std::ios::binary);
    if(!file.is_open())
    {
        std::cerr << "Error: failed to open file " << filePath << std::endl;
        return;
    }
    //read through the file until the 3rd line after element vertex
    std::string line;
    for(int i = 0; i < 2; i++)
    {
        std::getline(file, line);
    }
    //get the number of splats
    std::getline(file, line);
    std::stringstream ss(line);
    std::string element;
    ss >> element >> element >> numSplats;
    std::cout << "num splats: " << numSplats << std::endl;
    //read through the file until the end_header line
    while(line != "end_header")
    {
        std::getline(file, line);
    }
    std::vector<float> allValues;
    std::vector<float> normals;
    //this is from https://github.com/graphdeco-inria/diff-gaussian-rasterization/blob/main/cuda_rasterizer/auxiliary.h
    float SH_C0 = 0.28209479177387814f;
    int count = 0;
    //read the splats into the vectors
    //the splats are stored as floats, in little endian format
    for(int i = 0; i < numSplats; i++)
    {
        //read the mean
        float mean[3];
        file.read((char *) mean, sizeof(float) * 3);
        means3D.push_back(mean[0]);
        means3D.push_back(mean[1]);
        means3D.push_back(mean[2]);
        //read out the normal
        float normal[3];
        file.read((char *) normal, sizeof(float) * 3);
        normals.push_back(normal[0]);
        normals.push_back(normal[1]);
        normals.push_back(normal[2]);

        //read the colour
        //(0.5 + SH_C0 * rawVertex['f_dc_0']) * 255;
        float colour[3];
        file.read((char *) colour, sizeof(float) * 3);
        for(int i=0; i<3; i++)
        {
            colour[i] = (0.5 + (SH_C0 * colour[i])) * 255.f;
            //colour[i] = exp(colour[i]);
        }
        colours.push_back(colour[0]);
        colours.push_back(colour[1]);
        colours.push_back(colour[2]);
        //read the spherical harmonics (44 floats)
        float sphericalHarmonic[45];
        file.read((char *) sphericalHarmonic, sizeof(float) * 45);
        for(int j = 0; j < 45; j++)
        {
            sphericalHarmonics.push_back(sphericalHarmonic[j]);
        }
        //read the opacity
        float opacity;
        file.read((char *) &opacity, sizeof(float));
        //(1 / (1 + Math.exp(-rawVertex['opacity']))) * 255;
        opacity = (1/(1+exp(-opacity))) * 255;
        //increase count if opacity less than 1
        if(opacity < 1)
        {
            count++;
        }
        opacities.push_back(opacity);
        //read the scale
        float scale[3];
        file.read((char *) scale, sizeof(float) * 3);
        scales.push_back(scale[0]);
        scales.push_back(scale[1]);
        scales.push_back(scale[2]);
        //read the rotation
        float rotation[4];
        file.read((char *) rotation, sizeof(float) * 4);
        rotations.push_back(rotation[0]);
        rotations.push_back(rotation[1]);
        rotations.push_back(rotation[2]);
        rotations.push_back(rotation[3]);

    }
    //print the range of the opacities
    std::cout << "opacity range: " << *std::min_element(opacities.begin(), opacities.end()) << " - " << *std::max_element(opacities.begin(), opacities.end()) << std::endl;
    //print the number of splats with opacity less than 1
    std::cout << "num splats with opacity less than 1: " << count << std::endl;
    //print the range of the colours
    std::cout << "colour range: " << *std::min_element(colours.begin(), colours.end()) << " - " << *std::max_element(colours.begin(), colours.end()) << std::endl;
    //print the number of colours outside 0-255
    int count2 = 0;
    for(int i = 0; i < colours.size(); i++)
    {
        if(colours[i] < 0 || colours[i] > 255)
        {
            count2++;
        }
    }
    std::cout << "num colours outside 0-255: " << count2 << std::endl;
    //print the range of the rotations
    std::cout << "rotation range: " << *std::min_element(rotations.begin(), rotations.end()) << " - " << *std::max_element(rotations.begin(), rotations.end()) << std::endl;
    //print the range of the scales
    std::cout << "scale range: " << *std::min_element(scales.begin(), scales.end()) << " - " << *std::max_element(scales.begin(), scales.end()) << std::endl;
    //print the range of the means
    std::cout << "mean range: " << *std::min_element(means3D.begin(), means3D.end()) << " - " << *std::max_element(means3D.begin(), means3D.end()) << std::endl;
    char c;
    file.read(&c, sizeof(char));
    //check we have reached the end of the file
    if(!file.eof())
    {
        std::cerr << "Error: failed to read all splats from file" << std::endl;
        return;
    }
    //close the file
    file.close();
}