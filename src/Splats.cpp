//
// Created by thomas on 11/12/23.
//

#include "Splats.h"

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

Splats::Splats(const std::string &filePath, int width, int height)
{

    loadShaders();
    std::cout << "setting up splats" << std::endl;
    loadSplats(filePath);
    computeCovarianceMatrices();
    loadToGPU(width, height);

    glFinish();
    std::cout << "finished setting up splats" << std::endl;
}

Splats::~Splats()
{
    //delete the buffers
    glDeleteBuffers(1, &means3DBuffer);
    glDeleteBuffers(1, &coloursBuffer);
    glDeleteBuffers(1, &opacitiesBuffer);
    glDeleteBuffers(1, &indexBuffer);
    glDeleteBuffers(1, &intermediateBuffer);
    glDeleteBuffers(1, &histogramBuffer);
    glDeleteBuffers(1, &CovarianceBuffer);
    glDeleteBuffers(1, &projectedMeansBuffer);
    glDeleteBuffers(1, &projectedCovarianceBuffer);
    glDeleteBuffers(1, &binsBuffer);
    glDeleteBuffers(1, &depthBuffer);
    glDeleteBuffers(1, &numDupedBuffer);
    glDeleteBuffers(1, &splatKeysBuffer);
    //delete the texture
    glDeleteTextures(1, &texture);
    //delete the shaders
    glDeleteProgram(preProcessProgram);
    glDeleteProgram(sortProgram);
    glDeleteProgram(histogramProgram);
    glDeleteProgram(drawProgram);
    glDeleteProgram(displayProgram);
    glDeleteProgram(binProgram);
    glDeleteProgram(prefixSumProgram);
    glDeleteProgram(binPrefixSumProgram);
    //delete the vao and vbo
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

}

void Splats::loadToGPU(int width, int height)
{
    std::cout << "Loading splats to GPU" << std::endl;
    //create the buffers
    glGenBuffers(1, &means3DBuffer);
    glGenBuffers(1, &coloursBuffer);
    glGenBuffers(1, &opacitiesBuffer);
    glGenBuffers(1, &indexBuffer);
    glGenBuffers(1, &intermediateBuffer);
    glGenBuffers(1, &histogramBuffer);
    glGenBuffers(1, &CovarianceBuffer);
    glGenBuffers(1, &projectedMeansBuffer);
    glGenBuffers(1, &projectedCovarianceBuffer);
    glGenBuffers(1, &depthBuffer);
    glGenBuffers(1, &numDupedBuffer);
    glGenBuffers(1, &splatKeysBuffer);
    //bind the buffers and load the data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, means3DBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(means3D.size() * sizeof(glm::vec4)), means3D.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, coloursBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(colours.size() * sizeof(glm::vec4)), colours.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, opacitiesBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(opacities.size() * sizeof(float)), opacities.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, CovarianceBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(numSplats * 6 * sizeof(float)), covarianceMatrices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedMeansBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(numSplats * 2 * sizeof(float)), nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedCovarianceBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(numSplats * 4 * sizeof(float)), nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, depthBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(numSplats * 2 * sizeof(float)), nullptr, GL_STATIC_DRAW);


    //These buffers can have duplicate keys, so we need to allocate enough space for the maximum number of keys
    //for now, we will allocate space for 2 * numSplats keys
    //TODO: choose a better number than 2 * numSplats, as this is probably too many
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(numSplats * 2 * sizeof(int)), nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, intermediateBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(numSplats * 2 * sizeof(int)), nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(numSplats * 2 * sizeof(int)), nullptr, GL_STATIC_DRAW);

    //bin buffer (buffer of uints, size 256)
    glGenBuffers(1, &binsBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, binsBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 256 * sizeof(int), nullptr, GL_STATIC_DRAW);
    //num culled buffer (buffer of uints, size 1)
    glGenBuffers(1, &numDupedBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, numDupedBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int), nullptr, GL_STATIC_DRAW);
    //splat keys buffer (buffer of vec2s, size numSplats * 2)
    glGenBuffers(1, &splatKeysBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, splatKeysBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(numSplats * 2 * sizeof(glm::vec2)), nullptr, GL_STATIC_DRAW);


    //create the texture to display
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    //set the texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //GL_NEAREST for no interpolation
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //set the texture to be the same size as the screen
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
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
            {1,  -1, 0},
            {1,  1,  0},
            {1,  1,  0},
            {-1, 1,  0},
            {-1, -1, 0}
    };
    //load the vertices into the vbo
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3)), vertices.data(), GL_STATIC_DRAW);
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
    //load the draw shader
    drawProgram = loadAndLinkShader("draw");
    //load the display shaders
    displayProgram = loadAndLinkShaders("renderTexture");
    //load the bin shader
    binProgram = loadAndLinkShader("countBins");
    //load the bin prefix sum shader
    binPrefixSumProgram = loadAndLinkShader("prefixBins");


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
    if (!file.is_open())
    {
        std::cerr << "Error: failed to open file " << filePath << std::endl;
        return;
    }
    //read through the file until the 3rd line after element vertex
    std::string line;
    for (int i = 0; i < 2; i++)
    {
        std::getline(file, line);
    }
    //get the number of splats
    std::getline(file, line);
    std::stringstream ss(line);
    std::string element;
    ss >> element >> element >> numSplats;
    //numSplats = 300000;
    std::cout << "num splats: " << numSplats << std::endl;
    //read through the file until the end_header line
    while (line != "end_header")
    {
        std::getline(file, line);
    }
    //assign memory for the vectors
    means3D.reserve(numSplats);
    colours.reserve(numSplats);
    opacities.reserve(numSplats);
    scales.reserve(numSplats);
    rotations.reserve(numSplats);
    //this is from https://github.com/graphdeco-inria/diff-gaussian-rasterization/blob/main/cuda_rasterizer/auxiliary.h
    float SH_C0 = 0.28209479177387814f;
    //read the splats into the vectors
    //the splats are stored as floats, in little endian format
    for (int i = 0; i < numSplats; i++)
    {
        //read the mean
        float mean[3];
        file.read((char *) mean, sizeof(float) * 3);
        glm::vec4 meanVec(mean[0], mean[1], mean[2], 1.f);
        means3D.push_back(meanVec);
        //read out the normal
        float normal[3];
        file.read((char *) normal, sizeof(float) * 3);

        //read the colour
        //(0.5 + SH_C0 * rawVertex['f_dc_0']) * 255;
        float colour[3];
        file.read((char *) colour, sizeof(float) * 3);
        for (float & c : colour)
        {
            c = (0.5f + (SH_C0 * c)) * 255.f;
            //colour[i] = exp(colour[i]);
        }
        glm::vec4 colourVec(colour[0], colour[1], colour[2], 1.f);
        colours.push_back(colourVec);
        //read the spherical harmonics (44 floats)
        float sphericalHarmonic[45];
        file.read((char *) sphericalHarmonic, sizeof(float) * 45);
        //read the opacity
        float opacity;
        file.read((char *) &opacity, sizeof(float));
        //(1 / (1 + Math.exp(-rawVertex['opacity']))) * 255;
        opacity = (1 / (1 + exp(-opacity)));

        opacities.push_back(opacity);
        //read the scale
        float scale[3];
        file.read((char *) scale, sizeof(float) * 3);
        // do exp(scale) to get the scale
        for (float & s : scale)
        {
            s = exp(s);
        }
        glm::vec3 scaleVec(scale[0], scale[1], scale[2]);
        scales.push_back(scaleVec);
        //read the rotation
        float rotation[4];
        file.read((char *) rotation, sizeof(float) * 4);
        // normalise the rotation
        float length = sqrt(rotation[0] * rotation[0] + rotation[1] * rotation[1] + rotation[2] * rotation[2] +
                            rotation[3] * rotation[3]);
        rotation[0] /= length;
        rotation[1] /= length;
        rotation[2] /= length;
        rotation[3] /= length;
        glm::vec4 rotationVec(rotation[0], rotation[1], rotation[2], rotation[3]);
        rotations.push_back(rotationVec);
    }
    char c;
    file.read(&c, sizeof(char));
    //check we have reached the end of the file
    if (!file.eof())
    {
        std::cerr << "Error: failed to read all splats from file" << std::endl;
        return;
    }
    //close the file
    file.close();
    std::cout << "Finished loading splats from file" << std::endl;
}

void Splats::sort()
{
    int size = numSplats + numDuplicates;
    int workGroupSize = 32;
    int workGroupCount = 16;
    //pad the buffer
    GPURadixSort(histogramProgram, prefixSumProgram, sortProgram, intermediateBuffer, indexBuffer,
                 histogramBuffer, size, workGroupCount, workGroupSize, depthBuffer);
}

void Splats::draw(int width, int height, float tileWidth, float tileHeight)
{
    //bind the shader
    glUseProgram(drawProgram);
    //bind the buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, binsBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, projectedMeansBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, projectedCovarianceBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, coloursBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, indexBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, splatKeysBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, depthBuffer);
    //bind the output screen sized buffer (a image2D)
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    //set the uniforms
    glUniform1i(glGetUniformLocation(drawProgram, "numSplats"), numSplats+numDuplicates);
    glUniform1i(glGetUniformLocation(drawProgram, "screenWidth"), width);
    glUniform1i(glGetUniformLocation(drawProgram, "screenHeight"), height);
    glUniform1f(glGetUniformLocation(drawProgram, "tileWidth"), tileWidth);
    glUniform1f(glGetUniformLocation(drawProgram, "tileHeight"), tileHeight);

    //render the splats
    glDispatchCompute(width / 32, height / 32, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

}

void Splats::display()
{


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
    //reserve memory for the covariance matrices
    covarianceMatrices.reserve(numSplats * 6);
    //compute the 3D covariance matrix for each splat
    for (int i = 0; i < numSplats; i++)
    {
        //get the scale and rotation of the splat
        glm::vec3 scale = scales[i];
        glm::vec4 rotation = rotations[i];
        //compute the 3D covariance matrix
        glm::mat3x3 covariance = computeCovarianceMatrix(scale, rotation);

        //commpute the eignenvalues and eigenvectors of the covariance matrix to check the results
        //https://www.geometrictools.com/Documentation/RobustEigenSymmetric3x3.pdf
        covarianceMatrices.push_back(covariance[0][0]);
        covarianceMatrices.push_back(covariance[0][1]);
        covarianceMatrices.push_back(covariance[0][2]);
        covarianceMatrices.push_back(covariance[1][1]);
        covarianceMatrices.push_back(covariance[1][2]);
        covarianceMatrices.push_back(covariance[2][2]);
    }
    std::cout << "Finished computing covariance matrices" << std::endl;
}

glm::mat3x3 Splats::computeCovarianceMatrix(const glm::vec3 scale, const glm::vec4 rotation)
{
    //create the scale matrix
    glm::mat3x3 scaleMatrix = glm::mat3x3(scale[0], 0, 0,
                                          0, scale[1], 0,
                                          0, 0, scale[2]);


    float r = rotation.x;
    float x = rotation.y;
    float y = rotation.z;
    float z = rotation.w;

    // Compute rotation matrix from quaternion
    glm::mat3 rotationMatrix = glm::mat3(
            1.f - 2.f * (y * y + z * z), 2.f * (x * y - r * z), 2.f * (x * z + r * y),
            2.f * (x * y + r * z), 1.f - 2.f * (x * x + z * z), 2.f * (y * z - r * x),
            2.f * (x * z - r * y), 2.f * (y * z + r * x), 1.f - 2.f * (x * x + y * y)
    );

    //rotate the rotation matrix 90 degrees around an axis
    //https://en.wikipedia.org/wiki/Rotation_matrix
    glm::mat3x3 rotationMatrixX = glm::mat3x3(1, 0, 0,
                                              0, cos(90), -sin(90),
                                              0, sin(90), cos(90));
    glm::mat3x3 rotationMatrixY = glm::mat3x3(cos(90), 0, sin(90),
                                              0, 1, 0,
                                              -sin(90), 0, cos(90));
    glm::mat3x3 rotationMatrixZ = glm::mat3x3(cos(90), -sin(90), 0,
                                              sin(90), cos(90), 0,
                                              0, 0, 1);
    //rotationMatrix = rotationMatrix * rotationMatrix;
    //compute the 3D covariance matrix from the transformation matrix
    //https://users.cs.utah.edu/~tch/CS4640F2019/resources/A%20geometric%20interpretation%20of%20the%20covariance%20matrix.pdf
    // sigma = T * T^T
    glm::mat3x3 transformationMatrix = scaleMatrix * rotationMatrix;
    glm::mat3x3 covarianceMatrix = glm::transpose(transformationMatrix) * transformationMatrix;
    return covarianceMatrix;

}

void Splats::computeBins()
{
    //the bin shader needs the following inputs:
    //the buffer of keys
    //and the output:
    //the atomic counter buffer of bins
    //bind the shader
    glUseProgram(binProgram);
    //bind the buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, depthBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, binsBuffer);
    //initialise the bins to 0
    std::vector<GLuint> zero(256, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, binsBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 256 * sizeof(GLuint), zero.data(), GL_DYNAMIC_DRAW);

    //set the length of the array of keys
    glUniform1i(glGetUniformLocation(binProgram, "length"), numSplats + numDuplicates);

    //run the shader
    glDispatchCompute(1+((numSplats+numDuplicates) / 256), 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    //run the prefix sum on the bins
    //bind the shader
    glUseProgram(binPrefixSumProgram);
    //bind the buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, binsBuffer);
    //run the shader
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT );
}



void saveImage(const std::vector<std::vector<glm::vec4>> &image, const std::string &filename)
{
    int width = static_cast<int>(image.size());
    int height = static_cast<int>(image[0].size());

    std::vector<unsigned char> pixels(width * height * 4);

    for (int h = 0; h < height; ++h)
    {
        for (int w = 0; w < width; ++w)
        {
            glm::vec4 color = image[w][h];
            //clamp the colour to between 0 and 255
            color = glm::clamp(color, glm::vec4(0, 0, 0, 0), glm::vec4(255, 255, 255, 255));

            int index = (h * width + w) * 4;
            pixels[index + 0] = static_cast<unsigned char>(color.r);
            pixels[index + 1] = static_cast<unsigned char>(color.g);
            pixels[index + 2] = static_cast<unsigned char>(color.b);
            pixels[index + 3] = static_cast<unsigned char>(color.a * 255);
        }
    }

    stbi_write_png(filename.c_str(), width, height, 4, pixels.data(), width * 4);
}

void Splats::preprocess(glm::mat4 viewMatrix, int width, int height, float focal_x, float focal_y, float tan_fov_x,
                        float tan_fov_y, glm::mat4 vpMatrix)
{
    //bind the shader
    glUseProgram(preProcessProgram);
    //bind the buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, means3DBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, CovarianceBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, opacitiesBuffer);

    //set the uniforms
    glUniformMatrix4fv(0, 1, GL_FALSE, &viewMatrix[0][0]);
    glUniform1ui(1, width);
    glUniform1ui(2, height);
    glUniform1f(3, focal_x);
    glUniform1f(4, focal_y);
    glUniform1f(5, tan_fov_x);
    glUniform1f(6, tan_fov_y);
    glUniformMatrix4fv(7, 1, GL_FALSE, &vpMatrix[0][0]);
    glUniform1ui(8, numSplats);
    //set the atomic counter
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 9, numDupedBuffer);
    //set the atomic counter to 0
    GLuint zero = 0;
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, numDupedBuffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_DRAW);

    //bind the output buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, projectedMeansBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, depthBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, projectedCovarianceBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, indexBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, splatKeysBuffer);
    //run the shader
    glDispatchCompute(numSplats, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);
    //get the number of splats duplicated
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, numDupedBuffer);
    auto *numDuped = (GLuint *) glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
    numDuplicates = static_cast<int>(numDuped[0]);
    numDuplicates = std::min(numDuplicates, numSplats);
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

}

void Splats::gpuRender(glm::mat4 viewMatrix, int width, int height, float focal_x, float focal_y, float tan_fov_x,
                       float tan_fov_y, glm::mat4 vpMatrix)
{
    preprocess(viewMatrix, width, height, focal_x, focal_y, tan_fov_x, tan_fov_y, vpMatrix);

    //print the binscpuRender
    computeBins();
    sort();
    glFinish();
    draw(width, height, float(width) / 16.f, float(height) / 16.f);
}

void
    Splats::cpuRender(glm::mat4 viewMatrix, int width, int height, float focal_x, float focal_y, float tan_fov_x,
                  float tan_fov_y, glm::mat4 vpMatrix)
{
    //create the image to render to, as a 2D vector of vec4s
    std::vector<std::vector<glm::vec4>> image(width, std::vector<glm::vec4>(height, glm::vec4(0, 0, 0, 0)));
    //vector for the depth buffer for each splat
    std::vector<float> depthVector(numSplats * 2);
    //vector for the pixel-projected means
    std::vector<glm::vec2> projectedMeans(numSplats);
    //vector for the pixel-projected 2D covariance matrices
    std::vector<glm::vec4> projectedCovariances(numSplats);
    //vector for the bounding radii of the splats
    std::vector<float> boundingRadii(numSplats);
    //vector for the index of the splat
    std::vector<int> splatKeys(numSplats * 2);
    //vector for the index of the depth buffer (to sort the splats)
    std::vector<int> indices(numSplats * 2);

    int bins[256];
    for (int & bin : bins)
    {
        bin = 0;
    }
    int numSplatsCulled = 0;
    float tileWidth = float(width) / 16.f;
    float tileHeight = float(height) / 16.f;
//#define GPUPREPROCESS
#ifndef GPUPREPROCESS
    numDuplicates = 0;
    // for each splat, project the mean
    for (int i = 0; i < numSplats; i++)
    {
        indices[i] = i;
        //project the mean
        glm::vec3 t = viewMatrix * means3D[i];
        float limx = -1.3f * tan_fov_x;
        float limy = -1.3f * tan_fov_y;
        float txtz = t.x / t.z;
        float tytz = t.y / t.z;
        t.x = std::min(limx, std::max(-limx, txtz)) * t.z;
        t.y = std::min(limy, std::max(-limy, tytz)) * t.z;
        glm::mat3 Jacobian = glm::mat3(
                focal_x / t.z, 0.0f, -(focal_x * t.x) / (t.z * t.z),
                0.0f, focal_y / t.z, -(focal_y * t.y) / (t.z * t.z),
                0, 0, 0);


        glm::mat3 viewMatrix3 = glm::mat3{
                viewMatrix[0][0], viewMatrix[0][1], viewMatrix[0][2],
                viewMatrix[1][0], viewMatrix[1][1], viewMatrix[1][2],
                viewMatrix[2][0], viewMatrix[2][1], viewMatrix[2][2]
        };


        glm::mat3 T = glm::transpose(viewMatrix3) * Jacobian;
        int covarianceOffset = i * 6;
        glm::mat3 covarianceMatrix = glm::mat3(
                covarianceMatrices[covarianceOffset], covarianceMatrices[covarianceOffset + 1], covarianceMatrices[covarianceOffset + 2],
                covarianceMatrices[covarianceOffset + 1], covarianceMatrices[covarianceOffset + 3], covarianceMatrices[covarianceOffset + 4],
                covarianceMatrices[covarianceOffset + 2], covarianceMatrices[covarianceOffset + 4], covarianceMatrices[covarianceOffset + 5]);

        glm::mat3 covariance2D = glm::transpose(T) * glm::transpose(covarianceMatrix) * T;
        covariance2D[0][0] += 0.3f;
        covariance2D[1][1] += 0.3f;
        glm::vec3 covariance2DCompressed = glm::vec3(covariance2D[0][0], covariance2D[0][1], covariance2D[1][1]);
        //invert the covariance matrix
        float determinant = covariance2DCompressed.x * covariance2DCompressed.z -
                            covariance2DCompressed.y * covariance2DCompressed.y;
        if (determinant == 0)
        {
            continue;
        }
        float inverseDeterminant = 1.f / determinant;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "ArgumentSelectionDefects"
        glm::vec4 conic = glm::vec4(covariance2DCompressed.z, -covariance2DCompressed.y, covariance2DCompressed.x, 1.0) *
                          inverseDeterminant;
#pragma clang diagnostic pop
        conic.w = opacities[i];
        //store the conic
        projectedCovariances[i] = conic;
        //projectedCovariances[i] = glm::vec4(viewMatrix3[0][0], viewMatrix3[0][1], viewMatrix3[0][2], viewMatrix3[1][0]);
        //projectedCovariances[i] = glm::vec4(T[0][0], T[0][1], T[0][2], T[1][0]);
        //projectedCovariances[i] = glm::vec4(viewMatrix3[1][0], viewMatrix3[1][1], viewMatrix3[1][2], viewMatrix3[2][0]);
        //projectedCovariances[i] = glm::vec4(viewMatrix3[2][0], viewMatrix3[2][1], viewMatrix3[2][2], viewMatrix3[2][0]);
        //projectedCovariances[i] = glm::vec4(Jacobian[0][0], Jacobian[0][2], Jacobian[1][1], Jacobian[1][2]);
        //projectedCovariances[i] = glm::vec4(t.y);
        //projectedCovariances[i] = glm::vec4(covariance2D[0][0], covariance2D[0][1], covariance2D[1][1], covariance2D[2][2]);
        //calculate a bounding box for the conic based on the biggest eigenvalue
        //calculate the eigenvalues
        float middle = (covariance2DCompressed.z + covariance2DCompressed.x) * 0.5f;
        float lambda1 = middle + sqrt(std::max(0.1f, middle * middle - determinant));
        float lambda2 = middle - sqrt(std::max(0.1f, middle * middle - determinant));
        float radius = ceil(3.f * sqrt(std::max(lambda1, lambda2)));
        //store the radius
        boundingRadii[i] = radius;


        //project the mean
        glm::vec4 projectedMean = vpMatrix * means3D[i];
        //normalise the projected mean
        projectedMean /= fmax(projectedMean[3], 0.0001f);
        //store the depth
        //TODO: encode the depths into an int so we can store them in fewer bits
        //convert to screen space
        projectedMean = (projectedMean + 1.f) * 0.5f;
        //convert to pixel space
        projectedMean[0] *= float(width);
        projectedMean[1] *= float(height);
        //store the projected mean
        projectedMeans[i] = glm::vec2(projectedMean[0], projectedMean[1]);

        //if projected mean x or y are less than 0, or greater than the width or height, continue
        if(projectedMean.x < 0 || projectedMean.x > float(width) || projectedMean.y < 0 || projectedMean.y > float(height))
        {
            depthVector[i] = 1000000;
            numSplatsCulled++;
            //set everything else to 0
            projectedMeans[i] = glm::vec2(0, 0);
            projectedCovariances[i] = glm::vec4(0, 0, 0, 0);
            boundingRadii[i] = 0;
            splatKeys[i] = 0;
            indices[i] = i;
            continue;
        }

        int tileMinX = static_cast<int>(fmax(0, std::floor((projectedMean[0] - radius) / tileWidth)));
        int tileMaxX = static_cast<int>(fmax(15, std::floor((projectedMean[0] + radius) / tileWidth)));
        int tileMinY = static_cast<int>(fmax(0, std::floor((projectedMean[1] - radius) / tileHeight)));
        int tileMaxY = static_cast<int>(fmax(15, std::floor((projectedMean[1] + radius) / tileHeight)));
        //calculate the main tile the splat is in
        int tileX = static_cast<int>(projectedMean[0] / tileWidth);
        int tileY = static_cast<int>(projectedMean[1] / tileHeight);
        //calculate the bin the splat is in
        int tileIndex = tileY * 16 + tileX;
        bins[tileIndex]++;
        depthVector[i] = projectedMean[2] + float(tileIndex);
        splatKeys[i] = i;

        //calculate the number of duplicates
        int numberDuplicates = (tileMaxX - tileMinX + 1) * (tileMaxY - tileMinY + 1) - 1;
        int duplicateOffset = numSplats + numDuplicates;
        if(numDuplicates > numSplats * 2)
        {
            continue;
        }
        numDuplicates += numberDuplicates;
        //store the duplicates
        for(int y = tileMinY; y <= tileMaxY; y++)
        {
            for (int x = tileMinX; x <= tileMaxX; x++)
            {
                if (x == tileX && y == tileY)
                {
                    continue;
                }
                tileIndex = y * 16 + x;
                depthVector[duplicateOffset] = projectedMean[2] + float(tileIndex);
                splatKeys[duplicateOffset] = i;
                indices[duplicateOffset] = duplicateOffset;
                bins[tileIndex]++;
                duplicateOffset++;
            }
        }

    }
    std::cout << numDuplicates << " duplicates" << std::endl;
    //prefix sum the bins
    for (int i = 1; i < 256; i++)
    {
        bins[i] += bins[i - 1];
    }
#endif
//#ifdef GPUPREPROCESS
    // GPU pre-processing


    double timer = glfwGetTime();
    preprocess(viewMatrix, width, height, focal_x, focal_y, tan_fov_x, tan_fov_y, vpMatrix);
    std::cout << "Time taken to preprocess: " << glfwGetTime() - timer << std::endl;
    computeBins();
    std::cout << "Time taken to preprocess and bin: " << glfwGetTime() - timer << std::endl;
#define TEST
#ifdef TEST
    // get the buffers from the GPU into the vectors
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedMeansBuffer);
    auto *projectedMeansptr = (float *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < numSplats; i++)
    {
        auto pm = glm::vec2(projectedMeansptr[i * 2], projectedMeansptr[i * 2 + 1]);
        //store in the vector
        assert(abs(projectedMeans[i].x - pm.x) < 0.01);
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedCovarianceBuffer);
    auto *projectedCovariancesptr = (float *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < numSplats; i++)
    {
        auto pc = glm::vec4 (projectedCovariancesptr[i * 4], projectedCovariancesptr[i * 4 + 1], projectedCovariancesptr[i * 4 + 2], projectedCovariancesptr[i * 4 + 3]);
        assert(abs(projectedCovariances[i].x - pc.x) < 0.01);
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    int error = 0;

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, depthBuffer);
    auto *depthVectorptr = (float *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < numSplats; i++)
    {
        auto dv = depthVectorptr[i];
        //std::cout << i << std::endl;
        if(abs(depthVector[i] - dv) > 0.01)
        {
            error++;
        }
    }
    assert(error <= 100);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);



    glBindBuffer(GL_SHADER_STORAGE_BUFFER, binsBuffer);
    auto *binsptr = (GLuint *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < 256; i++)
    {

        std::cout << "bin " << i << " has a difference of " << bins[i] - binsptr[i] << std::endl;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexBuffer);
    int *indicesptr = (int *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < numSplats ; i++)
    {
        assert(indices[i] == indicesptr[i]);
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, splatKeysBuffer);
    int *splatKeysptr = (int *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < numSplats    ; i++)
    {
        assert(splatKeys[i] == splatKeysptr[i]);
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
#endif
    sort();
    computeBins();
    //glFinish();
    std::cout << "Time taken to preprocess, bin and sort: " << glfwGetTime() - timer << std::endl;
    timer = glfwGetTime();
    GLuint timerQuery;
    glGenQueries(1, &timerQuery);
    glBeginQuery(GL_TIME_ELAPSED, timerQuery);
    //draw(width, height, width / 16.f, height / 16.f);
    glEndQuery(GL_TIME_ELAPSED);
    GLuint available = 0;
    while (!available)
    {
        glGetQueryObjectuiv(timerQuery, GL_QUERY_RESULT_AVAILABLE, &available);
    }
    GLuint64 timeElapsed;
    glGetQueryObjectui64v(timerQuery, GL_QUERY_RESULT, &timeElapsed);
    std::cout << "Time taken to draw: " << timeElapsed / 1000000 << "ms" << std::endl;

    glFinish();
    std::cout << "Time taken to draw: " << glfwGetTime() - timer << std::endl;

#define CPUDRAW
#ifdef CPUDRAW
#ifdef GPUPREPROCESS
    // get the buffers from the GPU into the vectors
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedMeansBuffer);
    float *projectedMeansptr = (float *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < numSplats; i++)
    {
        auto pm = glm::vec2(projectedMeansptr[i * 2], projectedMeansptr[i * 2 + 1]);
        //store in the vector
        //assert(abs(projectedMeans[i].x - pm.x) < 0.01);
        projectedMeans[i] = pm;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedCovarianceBuffer);
    float *projectedCovariancesptr = (float *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < numSplats; i++)
    {
        auto pc = glm::vec4 (projectedCovariancesptr[i * 4], projectedCovariancesptr[i * 4 + 1], projectedCovariancesptr[i * 4 + 2], projectedCovariancesptr[i * 4 + 3]);
        //assert(abs(projectedCovariances[i].x - pc.x) < 0.01);
        //store in the vector
        projectedCovariances[i] = pc;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, depthBuffer);
    float *depthVectorptr = (float *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < numSplats + numDuplicates; i++)
    {
        auto dv = depthVectorptr[i];
        //std::cout << i << std::endl;
        //assert(abs(depthVector[i] - dv) < 0.01);
        //store in the vector
        depthVector[i] = dv;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);



    glBindBuffer(GL_SHADER_STORAGE_BUFFER, binsBuffer);
    GLuint *binsptr = (GLuint *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < 256; i++)
    {
        //assert(bins[i] == binsptr[i]);
        bins[i] = binsptr[i];
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexBuffer);
    int *indicesptr = (int *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < numSplats + numDuplicates; i++)
    {
        //assert(indices[i] == indicesptr[i]);
        indices[i] = indicesptr[i];
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, splatKeysBuffer);
    int *splatKeysptr = (int *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < numSplats + numDuplicates   ; i++)
    {
        //assert(splatKeys[i] == splatKeysptr[i]);
        splatKeys[i] = splatKeysptr[i];
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    //get the number of duplicates
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, numDupedBuffer);
    GLuint *numDuped = (GLuint *) glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
    std::cout << "num duped: " << numDuped[0] << std::endl;
    numDuplicates = numDuped[0];

    std::cout << "Time taken to project means: " << glfwGetTime() - timer << std::endl;
#endif

    timer = glfwGetTime();
    //see if the splats are sorted by depth
#ifdef DEBUG
    int errors = 0;
    for (int i = 0; i < numSplats + numDuplicates - 1; i++)
    {
        if (depthVector[indices[i]] > depthVector[indices[i + 1]])
        {
            errors++;
        }
    }
    std::cout << "Errors: " << errors << std::endl;
#endif
    //sort the splats by depth
    //sort the indices by the depth buffer
    std::sort(indices.begin(), indices.begin() + numSplats + numDuplicates, [&depthVector](int i1, int i2)
    { return depthVector[i1] < depthVector[i2]; });
    std::cout << "Time taken to sort: " << glfwGetTime() - timer << std::endl;
    //print the maximum bin size
    int maxBin = 0;
    for (int i = 1; i < 256; i++)
    {
        maxBin = std::max(maxBin, bins[i] - bins[i - 1]);
    }
    std::cout << "Max bin size: " << maxBin << std::endl;
    //tiled per pixel rasterisation
    timer = glfwGetTime();
    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            glm::vec2 pixelPosition = glm::vec2(x, y);
            //get the pixel
            glm::vec4 &pixel = image[x][y];
            int tileX = static_cast<int>(float(x) / tileWidth);
            int tileY = static_cast<int>(float(y) / tileHeight);
            int tileIndex = tileY * 16 + tileX;
            int start = (tileIndex == 0) ? 0 : bins[tileIndex - 1];
            int end = bins[tileIndex];
            for(int i=start; i<end; i++)
            {
                int index = splatKeys[indices[i]];
                glm::vec2 projectedMean = projectedMeans[index];
                //if projected mean x or y are less than 0, or greater than the width or height, continue
                if(projectedMean.x < 0 || projectedMean.x > float(width) || projectedMean.y < 0 || projectedMean.y > float(height))
                {
                    continue;
                }
                float radius = boundingRadii[index];
                //check if the pixel is in the bounding box
                if (float(x) >= projectedMean[0] - radius && float(x) <= projectedMean[0] + radius &&
                    float(y) >= projectedMean[1] - radius && float(y) <= projectedMean[1] + radius)
                {


                    glm::vec4 conic = projectedCovariances[index];
                    //get the position of the mean in screen space
                    glm::vec2 distance = pixelPosition - projectedMean;
                    //get the value of the conic at the distance
                    float power = -0.5f * (conic.x * distance.x * distance.x + conic.z * distance.y * distance.y) -
                                  conic.y * distance.x * distance.y;
                    if(power > 0.f)
                    {
                        continue;
                    }
                    float alpha = std::min(0.99f, exp(power) * conic.w);
                    //if the alpha is less than the bit depth of the image, we want to ignore it
                    //we can reduce the number of computations by increasing this, but some pretty horrific artifacts emerge
                    //these artifacts are due to the "inside" of objects having random noise inside, so could be overcome
                    //by better training.
                    if (alpha < 1.f/255.f)
                    {
                        continue;
                    }

                    //alpha = 1;
                    //add the value to the pixel
                    //only add if number of splats per pixel is less than 10
                    glm::vec3 colour3 = glm::vec3 (colours[index]);
                    pixel = alphaBlend(pixel, glm::vec4(colour3, alpha));
                    //if the alpha is high enough, we are done with this pixel
                    if (pixel.a > 0.99f)
                    {
                        break;
                    }


                }
            }
        }
    }
    //per pixel rasterisation
    /*
    for(int y=0; y < 10; y++)
    {
        if(y % 1 == 0)
        {
            std::cout << "y: " << y << std::endl;
        }
        for(int x=0; x<width; x++)
        {
            for(int i=0; i<numSplats - numSplatsCulled; i++)
            {
                int index = indices[i];
                glm::vec2 projectedMean = projectedMeans[index];
                float radius = boundingRadii[index];
                //check if the pixel is in the bounding box
                if (x >= projectedMean[0] - radius && x <= projectedMean[0] + radius &&
                    y >= projectedMean[1] - radius && y <= projectedMean[1] + radius)
                {
                    glm::vec3 conic = projectedCovariances[index];
                    //get the value of the conic at the distance
                    float power = -0.5f * (conic.x * (x - projectedMean.x) * (x - projectedMean.x) +
                                           conic.z * (y - projectedMean.y) * (y - projectedMean.y)) -
                                  conic.y * (x - projectedMean.x) * (y - projectedMean.y);
                    float alpha = std::min(0.99f, exp(power));
                    //add the value to the pixel
                    image[x][y] = alphaBlend(image[x][y], glm::vec4(1, 0, 0, alpha));
                    //if the pixel alpha is high enough, break
                    if(image[x][y].a > 0.99f)
                    {
                        break;
                    }
                }
            }
        }
    }

    std::cout << "num splats culled: " << numSplatsCulled << std::endl;
    std::cout << "num splats to render: " << numSplats - numSplatsCulled << std::endl;
    //vector to count the number of splats contributing to each pixel
    std::vector<std::vector<int>> numSplatsPerPixel(width, std::vector<int>(height, 0));

    //per splat rasterisation
    for (int i = 0; i < numSplats - numSplatsCulled; i++)
    {
        //std::cout << "i: " << i << std::endl;
        int index = indices[i];
        //index = i;
        //For a real renderer, we would want to cap the size of the bounding box
        float radius = std::min(boundingRadii[index], 5000.f);
        glm::vec4 conic = projectedCovariances[index];
        glm::vec2 projectedMean = projectedMeans[index];
        for (int x = -radius; x < radius; x++)
        {
            for (int y = -radius; y < radius; y++)
            {
                //get the pixel
                int pixelX = projectedMean[0] + x;
                int pixelY = projectedMean[1] + y;
                //check it is in bounds
                if (pixelX >= 0 && pixelX < width && pixelY >= 0 && pixelY < height)
                {
                    //get the pixel
                    glm::vec4 &pixel = image[pixelX][pixelY];
                    //if the alpha is high enough, ignore the splat
                    if (pixel.a > 0.99f)
                    {
                        continue;
                    }
                    //get the position of the pixel in screen space
                    glm::vec2 pixelPosition = glm::vec2(pixelX, pixelY);
                    //get the position of the mean in screen space
                    glm::vec2 meanPosition = glm::vec2(projectedMean[0], projectedMean[1]);
                    glm::vec2 distance = glm::vec2(pixelX, pixelY) - meanPosition;
                    //get the value of the conic at the distance
                    float power = -0.5f * (conic.x * distance.x * distance.x + conic.z * distance.y * distance.y) -
                                  conic.y * distance.x * distance.y;
                    if(power > 0.f)
                    {
                        continue;
                    }
                    float alpha = std::min(0.99f, exp(power) * conic.w);
                    //if the alpha is less than the bit depth of the image, we want to ignore it
                    //we can reduce the number of computations by increasing this, but some pretty horrific artifacts emerge
                    //these artifacts are due to the "inside" of objects having random noise inside, so could be overcome
                    //by better training.
                    if (alpha < 1.f/255.f)
                    {
                        continue;
                    }

                    //alpha = 1;
                    //add the value to the pixel
                    //only add if number of splats per pixel is less than 10

                    //if(numSplatsPerPixel[pixelX][pixelY] < 50)
                    pixel = alphaBlend(pixel, glm::vec4(colours[index], alpha));
                    //count the number of splats contributing to the pixel

                    numSplatsPerPixel[pixelX][pixelY]++;

                }
            }
        }
    }*/
    std::cout << "Time taken to render: " << glfwGetTime() - timer << std::endl;
    //store the image in a png
    saveImage(image, "cpuRender.png");
    throw std::runtime_error("CPU render");

#endif
    //print the mean, median and max number of splats per pixel
//    int total = 0;
//    int max = 0;
//    for (int x = 0; x < width; x++)
//    {
//        for (int y = 0; y < height; y++)
//        {
//            total += numSplatsPerPixel[x][y];
//            if (numSplatsPerPixel[x][y] > max)
//            {
//                max = numSplatsPerPixel[x][y];
//            }
//            image[x][y] = (float) numSplatsPerPixel[x][y] * 3 * glm::vec4(1, 1, 1, 1);
//
//            image[x][y].a = 1;
//        }
//    }
//    //flatten out the number of splats per pixel
//    std::vector<int> flattenedNumSplatsPerPixel(width * height);
//    for (int x = 0; x < width; x++)
//    {
//        for (int y = 0; y < height; y++)
//        {
//            flattenedNumSplatsPerPixel[x * height + y] = numSplatsPerPixel[x][y];
//        }
//    }
//    //get the median
//    std::sort(flattenedNumSplatsPerPixel.begin(), flattenedNumSplatsPerPixel.end());
//    int median = flattenedNumSplatsPerPixel[flattenedNumSplatsPerPixel.size() / 2];
//    //get the 90th percentile
//    int percentile = flattenedNumSplatsPerPixel[flattenedNumSplatsPerPixel.size() * 0.9];
//    saveImage(image, "cpuNumSplatsPerPixel.png");
//    std::cout << "Mean number of splats per pixel: " << total / (width * height) << std::endl;
//    std::cout << "Max number of splats per pixel: " << max << std::endl;
//    std::cout << "Median number of splats per pixel: " << median << std::endl;
//    std::cout << "90th percentile number of splats per pixel: " << percentile << std::endl;

}

glm::vec4 Splats::alphaBlend(glm::vec4 colour1, glm::vec4 colour2)
{
    float alpha1 = colour1.a;
    float alpha2 = colour2.a;
    float remainingAlpha = 1 - alpha1;
    float alphaToBlend = alpha2 * remainingAlpha;
    glm::vec3 blendedColour = glm::vec3(colour1) + glm::vec3(colour2) * alphaToBlend;
    return glm::vec4(blendedColour, alpha1 + alphaToBlend);
}