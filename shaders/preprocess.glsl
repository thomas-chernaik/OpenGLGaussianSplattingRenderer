#version 430 core
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

layout (std430, binding = 6) buffer BoundingRadii {
    float data[];
} boundingRadii;

void main()
{
    // get thread index
    uint i = gl_GlobalInvocationID.x;
    if (i >= numSplats)
    {
        return;
    }
    vec4 mean = means3D.data[i];

    // Project the mean to 2D
    vec4 projectedMean = VPMatrix * mean;
    projectedMean /= max(projectedMean.w, 0.0001);
    //store the depth
    //if the pixel is off screen, give it a big depth (100000)
    if (projectedMean.x < -1.0 || projectedMean.x > 1.0 || projectedMean.y < -1.0 || projectedMean.y > 1.0)
    {
        depthBuffer.data[i] = 100000.0;
        //means2D.data[i] = vec2(projectedMean.x, screenWidth);
        return;
    }
    depthBuffer.data[i] = projectedMean.z;
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
    //conics.data[i] = vec4(viewMatrix3[0][0], viewMatrix3[0][1], viewMatrix3[0][2], viewMatrix3[1][0]);
    //conics.data[i] = vec4(viewMatrix3[1][0], viewMatrix3[1][1], viewMatrix3[1][2], viewMatrix3[2][0]);
    //conics.data[i] = vec4(viewMatrix3[2][0], viewMatrix3[2][1], viewMatrix3[2][2], viewMatrix3[2][0]);
    //conics.data[i] = vec4(Jacobian[0][0], Jacobian[0][2], Jacobian[1][1], Jacobian[1][2]);
    //conics.data[i] = vec4(T[0][0], T[0][1], T[0][2], T[1][0]);
    //conics.data[i] = vec4(t.y);
    //conics.data[i] = vec4(covariance2D[0][0], covariance2D[0][1], covariance2D[1][1], covariance2D[2][2]);
    // Calculate a bounding box for the conic
    float middle = (covariance2DCompressed.z + covariance2DCompressed.x) * 0.5;
    float lambda1 = middle + sqrt(max(0.1, middle * middle - determinant));
    float lambda2 = middle - sqrt(max(0.1, middle * middle - determinant));
    boundingRadii.data[i] = ceil(3.0 * sqrt(max(lambda1, lambda2)));


}