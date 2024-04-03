#version 460 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;


// Input data
layout (std430, binding = 0) buffer Means3D {
    vec4 data[];
} means3D;

layout (std430, binding = 1) buffer Covariances3D {
    float data[];
} covariances3D;

layout (std430, binding = 2) buffer Opacities {
    float data[];
} opacities;

// Input uniforms
// View matrix
layout (location = 0) uniform mat4 viewMatrix;
// Screen dimensions
layout (location = 1) uniform uint screenWidth;
layout (location = 2) uniform uint screenHeight;

// focal x and y
layout (location = 3) uniform float focalX;
layout (location = 4) uniform float focalY;

// tan of fov x and y
layout (location = 5) uniform float tanFovX;
layout (location = 6) uniform float tanFovY;

// VP matrix
layout (location = 7) uniform mat4 VPMatrix;

//num splats
layout (location = 8) uniform uint numSplats;

//atomic counter for the number of duplicates
layout (binding = 9, offset = 0) uniform atomic_uint duplicates;

// Output data
layout (std430, binding = 3) buffer Means2D {
    vec2 data[];
} means2D;

layout (std430, binding = 4) buffer DepthBuffer {
    float data[];
} depthBuffer;

layout (std430, binding = 5) buffer Conics {
    vec4 data[];
} conics;


layout (std430, binding = 6) buffer Indices {
    int data[];
} indices;

layout (std430, binding = 7) buffer SplatKeys {
    int data[];
} splatKeys;

void main()
{
    // get thread index
    uint i = gl_GlobalInvocationID.x;
    if (i >= numSplats)
    {
        return;
    }
    //set the index
    indices.data[i] = int(i);
    vec4 mean = means3D.data[i];

    // Project the mean to 2D
    vec4 projectedMean = VPMatrix * mean;
    projectedMean /= max(projectedMean.w, 0.0001);
    //if the pixel is off screen, give it a big depth (100000)
    if (projectedMean.x < -1.0 || projectedMean.x > 1.0 || projectedMean.y < -1.0 || projectedMean.y > 1.0)
    {
        depthBuffer.data[i] = 1000000.0;
        indices.data[i] = int(i);
        //set everything else to 0
        means2D.data[i] = vec2(0.0, 0.0);
        conics.data[i] = vec4(0.0, 0.0, 0.0, 0.0);
        splatKeys.data[i] = 0;
        return;
    }
    //convert the mean to screen space
    projectedMean = (projectedMean + 1.0) * 0.5;
    projectedMean.x *= screenWidth;
    projectedMean.y *= screenHeight;
    means2D.data[i] = vec2(projectedMean.x, projectedMean.y);


    // get covariance matrix
    uint covarianceOffset = 6 * i;
    mat3 covarianceMatrix = mat3(
    covariances3D.data[covarianceOffset], covariances3D.data[covarianceOffset + 1], covariances3D.data[covarianceOffset + 2],
    covariances3D.data[covarianceOffset + 1], covariances3D.data[covarianceOffset + 3], covariances3D.data[covarianceOffset + 4],
    covariances3D.data[covarianceOffset + 2], covariances3D.data[covarianceOffset + 4], covariances3D.data[covarianceOffset + 5]
    );
    mat3 viewMatrix3 = mat3(
    viewMatrix[0][0], viewMatrix[0][1], viewMatrix[0][2],
    viewMatrix[1][0], viewMatrix[1][1], viewMatrix[1][2],
    viewMatrix[2][0], viewMatrix[2][1], viewMatrix[2][2]
    );
    // project mean
    vec3 t = vec3(viewMatrix * mean);
    float limx = -1.3 * tanFovX;
    float limy = -1.3 * tanFovY;
    float txtz = t.x / t.z;
    float tytz = t.y / t.z;
    t.x = min(limx, max(-limx, txtz)) * t.z;
    t.y = min(limy, max(-limy, tytz)) * t.z;

    mat3 Jacobian = mat3(
    focalX / t.z, 0.0, -(focalX * t.x) / (t.z * t.z),
    0.0, focalY / t.z, -(focalY * t.y) / (t.z * t.z),
    0.0, 0.0, 0.0
    );

    mat3 T =  transpose(viewMatrix3) * Jacobian;

    mat3 covariance2D = transpose(T) * transpose(covarianceMatrix) * T;
    covariance2D[0][0] += 0.3;
    covariance2D[1][1] += 0.3;
    vec3 covariance2DCompressed = vec3(covariance2D[0][0], covariance2D[0][1], covariance2D[1][1]);
    float determinant = covariance2DCompressed.x * covariance2DCompressed.z - covariance2DCompressed.y * covariance2DCompressed.y;
    if (determinant == 0)
    {
        return;
    }
    float inverseDeterminant = 1.0 / determinant;
    conics.data[i] = vec4(vec3(covariance2DCompressed.z, -covariance2DCompressed.y, covariance2DCompressed.x) * inverseDeterminant, opacities.data[i]);

    // Calculate a bounding box for the conic
    float middle = (covariance2DCompressed.z + covariance2DCompressed.x) * 0.5;
    float lambda1 = middle + sqrt(max(0.1, middle * middle - determinant));
    float lambda2 = middle - sqrt(max(0.1, middle * middle - determinant));
    float radius = ceil(3.0 * sqrt(max(lambda1, lambda2)));
    float tileWidth = screenWidth / 16;
    float tileHeight = screenHeight / 16;

    uint tileMinX = max(0, int((projectedMean.x  - radius) / tileWidth));
    uint tileMaxX = min(15, int((projectedMean.x + radius) / tileWidth));
    uint tileMinY = max(0, int((projectedMean.y - radius) / tileHeight));
    uint tileMaxY = min(15, int((projectedMean.y + radius) / tileHeight));
    //calculate the main tile for the splat
    int tileX = int(projectedMean.x / tileWidth);
    int tileY = int(projectedMean.y / tileHeight);
    uint tileIndex = tileY * 16 + tileX;
    depthBuffer.data[i] = tileIndex + projectedMean.z;
    splatKeys.data[i] = int(i);

    //calculate the number of duplicates
    uint numDuplicates = (tileMaxX - tileMinX + 1) * (tileMaxY - tileMinY + 1) - 1;
    if(numDuplicates == 0)
    {
        return;
    }
    if(atomicCounter(duplicates) >= numSplats)
    {
        return;
    }
    int duplicateOffset = int(atomicCounterAdd(duplicates, numDuplicates)) + int(numSplats) + 1;


    //for each tile the splat is in, duplicate the splat
    for(uint y = tileMinY; y <= tileMaxY; y++)
    {
        for (uint x = tileMinX; x <= tileMaxX; x++)
        {
            if (x == tileX && y == tileY)
            {
                continue;
            }
            tileIndex = y * 16 + x;
            depthBuffer.data[duplicateOffset] = tileIndex + projectedMean.z;
            splatKeys.data[duplicateOffset] = int(i);
            indices.data[duplicateOffset] = duplicateOffset;
            duplicateOffset++;
            if(duplicateOffset >= int(numSplats) * 2)
            {
                return;
            }
        }
    }

}