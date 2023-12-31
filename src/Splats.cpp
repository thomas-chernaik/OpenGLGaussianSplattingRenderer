//
// Created by thomas on 11/12/23.
//

#include "Splats.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Splats::Splats(const std::string &filePath)
{
    loadShaders();
    std::cout << "setting up splats" << std::endl;
    loadSplats(filePath);
    computeCovarianceMatrices();
    loadToGPU();
    std::cout << "finished setting up splats" << std::endl;
}

Splats::~Splats()
{
    //delete the buffers
    glDeleteBuffers(1, &means3DBuffer);
    glDeleteBuffers(1, &coloursBuffer);
    glDeleteBuffers(1, &sphericalHarmonicsBuffer);
    glDeleteBuffers(1, &opacitiesBuffer);
    glDeleteBuffers(1, &indexBuffer);
    glDeleteBuffers(1, &keyBuffer);
    //delete the shaders
    glDeleteProgram(preProcessProgram);
    glDeleteProgram(generateKeysProgram);
    glDeleteProgram(sortProgram);
    glDeleteProgram(histogramProgram);
    glDeleteProgram(drawProgram);
    glDeleteProgram(displayProgram);
    //delete the vao and vbo
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

}

void Splats::loadToGPU()
{
    std::cout << "Loading splats to GPU" << std::endl;
    //create the buffers
    glGenBuffers(1, &means3DBuffer);
    glGenBuffers(1, &coloursBuffer);
    glGenBuffers(1, &sphericalHarmonicsBuffer);
    glGenBuffers(1, &opacitiesBuffer);
    glGenBuffers(1, &indexBuffer);
    glGenBuffers(1, &keyBuffer);
    glGenBuffers(1, &intermediateBuffer);
    glGenBuffers(1, &histogramBuffer);
    glGenBuffers(1, &CovarianceBuffer);
    glGenBuffers(1, &projectedMeansBuffer);
    glGenBuffers(1, &projectedCovarianceBuffer);
    //bind the buffers and load the data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, means3DBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, means3D.size() * sizeof(glm::vec3), means3D.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, coloursBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, colours.size() * sizeof(glm::vec3), colours.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphericalHarmonicsBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sphericalHarmonics.size() * sizeof(float), sphericalHarmonics.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, opacitiesBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, opacities.size() * sizeof(float), opacities.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, CovarianceBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numSplats * 6 * sizeof(float), covarianceMatrices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedMeansBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numSplats * 2 * sizeof(float), nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedCovarianceBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numSplats * 3 * sizeof(float), nullptr, GL_STATIC_DRAW);


    //These buffers can have duplicate keys, so we need to allocate enough space for the maximum number of keys
    //for now, we will allocate space for 2 * numSplats keys
    //TODO: choose a better number than 2 * numSplats, as this is probably too many
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numSplats * 2 * sizeof(int), nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, keyBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numSplats * 2 * 2 * sizeof(int), nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, intermediateBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numSplats  * 2 * sizeof(int), nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numSplats * 2 * sizeof(int), nullptr, GL_STATIC_DRAW);

    //create the texture to display
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    //set the texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //GL_NEAREST for no interpolation
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //set the texture to be the same size as the screen
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1920, 1080, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    //unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    //create the vao and vbo for displaying
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    //bind the vao and vbo
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    //a quad made of 2 triangles
    std::vector<glm::vec3> vertices = {
            {-1, -1, 0},
            {1, -1, 0},
            {1, 1, 0},
            {1, 1, 0},
            {-1, 1, 0},
            {-1, -1, 0}
    };
    //load the vertices into the vbo
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    //set the vertex attribute pointer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    //unbind the vao and vbo
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    std::cout << "Finished loading splats to GPU" << std::endl;

}

void Splats::loadShaders()
{
    //load the sort and histogram shaders
    createAndLinkSortAndHistogramShaders(histogramProgram, sortProgram, prefixSumProgram);
    //load the preprocess shader
    preProcessProgram = loadAndLinkShader("preprocess");
    //load the generate keys shader
    generateKeysProgram = loadAndLinkShader("duplicateKeys");
    //load the draw shader
    drawProgram = loadAndLinkShader("draw");
    //load the display shaders
    displayProgram = loadAndLinkShaders("renderTexture");

}

void Splats::loadSplats(const std::string &filePath)
{
    std::cout << "Loading splats from file" << std::endl;
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
        glm::vec3 meanVec(mean[0], mean[1], mean[2]);
        means3D.push_back(meanVec);
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
        glm::vec3 colourVec(colour[0], colour[1], colour[2]);
        colours.push_back(colourVec);
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
        glm::vec3 scaleVec(scale[0], scale[1], scale[2]);
        scales.push_back(scaleVec);
        //read the rotation
        float rotation[4];
        file.read((char *) rotation, sizeof(float) * 4);
        glm::vec4 rotationVec(rotation[0], rotation[1], rotation[2], rotation[3]);
        rotations.push_back(rotationVec);

    }
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
    std::cout << "Finished loading splats from file" << std::endl;
}

void Splats::preprocess(glm::mat4 vpMatrix, glm::mat3 rotationMatrix, int width, int height)
{
    /*the shader needs the following inputs
     * the buffer of means
     * the buffer of opacities
     * the buffer of precomputed 3D covariance matrices
     * the transform matrix (view matrix * projection matrix)
     * the rotation matrix as a 3x3 matrix
     * the number of splats
     * the screen width
     * the screen height
     *
     *
     * and needs buffers for the following outputs
     * the buffer of tiles touched & depths (keys)
     * the buffer of conic opacities (related to the 2D covariance matrices)
     * the buffer of the projected means
     */
    std::cout << "Preprocessing splats" << std::endl;
    //bind the shader
    glUseProgram(preProcessProgram);
    //bind the buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, means3DBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, opacitiesBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, CovarianceBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, keyBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, projectedCovarianceBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, projectedMeansBuffer);
    //set the uniforms
    glUniformMatrix4fv(glGetUniformLocation(preProcessProgram, "vpMatrix"), 1, GL_FALSE, &vpMatrix[0][0]);
    glUniformMatrix3fv(glGetUniformLocation(preProcessProgram, "rotationMatrix"), 1, GL_FALSE, &rotationMatrix[0][0]);
    glUniform1i(glGetUniformLocation(preProcessProgram, "numSplats"), numSplats);
    glUniform1i(glGetUniformLocation(preProcessProgram, "width"), width);
    glUniform1i(glGetUniformLocation(preProcessProgram, "height"), height);
    //run the shader
    glDispatchCompute(numSplats / 256, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    std::cout << "Finished preprocessing splats" << std::endl;
#ifdef DEBUG
    //print out the keys (buffer of vec2s)
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, keyBuffer);
    int numDupes = 0;
    int numCulled = 0;
    glm::vec2* keys = (glm::vec2*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    unsigned int allOnes = 0xFFFFFFFF;
    for(int i = 0; i < numSplats; i++)
    {
        //convert the 2 keys to unsigned ints, witht the same bit pattern
        unsigned int key1 = *(unsigned int*)&keys[i][0];
        unsigned int key2 = *(unsigned int*)&keys[i][1];
        //don't print out the keys that are all ones
        if(key1 != allOnes && key2 != allOnes)
        {
            //std::cout << "key1: " << std::bitset<32>(key1) << std::endl;
            //std::cout << "key2: " << std::bitset<32>(key2) << std::endl;
            for(uint i=1; i<3; i++) {
                if((key1 & (0xFF << i * 8)) != 0) {
                    numDupes++;
                    std::cout << "Key" << key1 << std::endl;
                }
            }
        }
        else
        {
            numCulled++;
        }
        //

    }
    std::cout << "num duplicates: " << numDupes << std::endl;
    std::cout << "num culled: " << numCulled << std::endl;
#endif


}

void Splats::duplicateKeys()
{
    //the generate keys shader needs the following inputs
    //the buffer of keys
    //the buffer of indices
    //the number of splats
    //the maximum number of keys we can have (the buffer size)

    //and needs buffers for the following outputs
    //the buffer of duplicate keys (the same as the buffer of keys)

    std::cout << "Generating keys" << std::endl;

    //bind the shader
    glUseProgram(generateKeysProgram);
    //bind the buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, keyBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, indexBuffer);
    //set the uniforms
    glUniform1i(0, numSplats);
    glUniform1i(glGetUniformLocation(generateKeysProgram, "maxNumKeys"), numSplats * 2);
    //set the atomic counter to 0
    GLuint numCulled, numDuplicates;
    int zero = 0;
    glGenBuffers(1, &numCulled);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, numCulled);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &numDuplicates);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, numDuplicates);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_DRAW);
    //bind the atomic counter to the shader
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, numCulled);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, numDuplicates);

    //run the shader
    glDispatchCompute(ceil(numSplats / 1024.f), 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);
    std::cout << "Finished generating keys" << std::endl;
    //print the atomic counters
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, numCulled);
    GLuint* numCulledData = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
    std::cout << "num culled: " << numCulledData[0] << std::endl;
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, numDuplicates);
    GLuint* numDuplicatesData = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
    std::cout << "num duplicates: " << numDuplicatesData[0] << std::endl;
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

    numSplatsPostCull = numSplats - numCulledData[0] + numDuplicatesData[0];
    std::cout << "num splats post cull: " << numSplatsPostCull << std::endl;
}

void Splats::sort()
{
    std::cout << "Sorting splats" << std::endl;
    double time = glfwGetTime();
    GPURadixSort2(histogramProgram, prefixSumProgram, sortProgram, keyBuffer, indexBuffer, intermediateBuffer, histogramBuffer,
                  numSplats * 2, 8, 256);
    std::cout << "Finished sorting splats" << std::endl;
    std::cout << "Time taken to sort: " << glfwGetTime() - time << std::endl;

}

void Splats::draw(float *viewMatrix, float *projectionMatrix, float *lightPosition, float *lightColour,
                  float *cameraPosition, int width, int height)
{
    //the draw function needs to following inputs:
    //the start and end of each bin for each tile (this can be found at the end of the histogram buffer and the numSplatsPostCull)
    //the buffer of projected means
    //the buffer of projected covariance matrices
    //the buffer of colours
    //the screen dimensions
#ifdef DEBUG
    //set the texture on the cpu to white
    std::vector<unsigned char> textureData(1920 * 1080 * 4, 255);
    //bind the texture
    glBindTexture(GL_TEXTURE_2D, texture);
    //load the image
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData.data());
    //unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
#endif

    //and needs a screen sized buffer for the output
    std::cout << "Drawing splats" << std::endl;
    //bind the shader
    glUseProgram(drawProgram);
    //bind the buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, histogramBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, projectedMeansBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, projectedCovarianceBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, coloursBuffer);
    //bind the output screen sized buffer (a image2D)
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    //set the uniforms
    glUniform1i(glGetUniformLocation(drawProgram, "numSplats"), numSplatsPostCull);
    glUniform1i(glGetUniformLocation(drawProgram, "screenWidth"), width);
    glUniform1i(glGetUniformLocation(drawProgram, "screenHeight"), height);

    //render the splats
    glDispatchCompute(width / 16, height / 16, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    std::cout << "Finished drawing splats" << std::endl;

}

void Splats::display()
{
    //the display function needs the following inputs:
    //the vbo
    //the texture to display

#ifdef DEBUG
    //load up an image from a file to texture
    //bind the texture
    glBindTexture(GL_TEXTURE_2D, texture);
    //load the image
    int w, h, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load("tex.png", &w, &h, &nrChannels, 0);
    std::vector<unsigned char> textureData(width * height * 4, 255);

    //check the image loaded correctly
    if(!data)
    {
        std::cerr << "Error: failed to load image" << std::endl;
        return;
    }
    GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
    //load the image into the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, format, GL_UNSIGNED_BYTE, textureData.data());
    //free the image data
    stbi_image_free(data);
    //unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
#endif


    //bind the shader
    glUseProgram(displayProgram);
    //bind the vao and vbo
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    //set the vertex attribute pointer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    //bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //GL_NEAREST for no interpolation
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); //GL_CLAMP_TO_BORDER for no wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    //set the uniforms
    glUniform1i(glGetUniformLocation(displayProgram, "tex"), 0);
    //draw the texture
    glDrawArrays(GL_TRIANGLES, 0, 6);
    //unbind the vao and vbo
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    //unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Splats::computeCovarianceMatrices()
{
    std::cout << "Computing covariance matrices" << std::endl;
    //compute the 3D covariance matrix for each splat
    for(int i = 0; i < numSplats; i++)
    {
        //get the scale and rotation of the splat
        glm::vec3 scale = scales[i];
        glm::vec4 rotation = rotations[i];
        //compute the 3D covariance matrix
        glm::mat3x3 covariance = computeCovarianceMatrix(scale, rotation);
        //store only the upper triangular part of the matrix, as it is symmetric
        covarianceMatrices.push_back({covariance[0][0], covariance[0][1], covariance[0][2], covariance[1][1], covariance[1][2],
                               covariance[2][2]});
    }
    std::cout << "Finished computing covariance matrices" << std::endl;
}

glm::mat3x3 Splats::computeCovarianceMatrix(const glm::vec3 scale, const glm::vec4 rotation)
{
    //create the scale matrix
    glm::mat3x3 scaleMatrix = glm::mat3x3(scale[0], 0, 0, 0, scale[1], 0, 0, 0, scale[2]);

    //normalise the rotation quaternion
    float length = rotation.length();
    glm::vec4 normalisedRotation = glm::vec4(rotation[0] / length, rotation[1] / length, rotation[2] / length, rotation[3] / length);
    //create the rotation matrix
    //https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation
    glm::mat3x3 rotationMatrix = glm::mat3x3( 1-2*(normalisedRotation[1]*normalisedRotation[1] + normalisedRotation[2]*normalisedRotation[2]), 2*(normalisedRotation[0]*normalisedRotation[1] - normalisedRotation[2]*normalisedRotation[3]), 2*(normalisedRotation[0]*normalisedRotation[2] + normalisedRotation[1]*normalisedRotation[3]),
                                              2*(normalisedRotation[0]*normalisedRotation[1] + normalisedRotation[2]*normalisedRotation[3]), 1-2*(normalisedRotation[0]*normalisedRotation[0] + normalisedRotation[2]*normalisedRotation[2]), 2*(normalisedRotation[1]*normalisedRotation[2] - normalisedRotation[0]*normalisedRotation[3]),
                                              2*(normalisedRotation[0]*normalisedRotation[2] - normalisedRotation[1]*normalisedRotation[3]), 2*(normalisedRotation[1]*normalisedRotation[2] + normalisedRotation[0]*normalisedRotation[3]), 1-2*(normalisedRotation[0]*normalisedRotation[0] + normalisedRotation[1]*normalisedRotation[1]));
    //compute the 3D covariance matrix from the transformation matrix
    //https://users.cs.utah.edu/~tch/CS4640F2019/resources/A%20geometric%20interpretation%20of%20the%20covariance%20matrix.pdf
    // sigma = T * T^T
    glm::mat3x3 transformationMatrix = rotationMatrix * scaleMatrix;
    glm::mat3x3 covarianceMatrix = transformationMatrix * glm::transpose(transformationMatrix);
    return covarianceMatrix;

}