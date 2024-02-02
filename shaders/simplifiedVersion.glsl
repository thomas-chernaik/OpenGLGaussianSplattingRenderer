#version 430 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

//inputs
//means3D
layout(std430, binding = 0) buffer Means3D
{
    vec3 data[];
} means3D;
//opacities
layout(std430, binding = 1) buffer Opacities
{
    float data[];
} opacities;
//3D covariance matrices
layout(std430, binding = 2) buffer Covariances3D
{
    float data[];
} covariances3D;

//VP matrix uniform
layout (location = 0)uniform mat4 vpMatrix;

//view matrix uniform
layout (location = 1)uniform mat3 rotationMatrix;

//the number of splats uniform
layout (location = 2) uniform int numSplats;

//screen width and height uniform
layout (location = 3) uniform int width;
layout (location = 4) uniform int height;

//output
//image texture
layout(rgba8, binding = 0) writeonly uniform image2D outputImage;

void main()
{

    //we are just going to draw the closest pixel to each splat, if it is within the screen
    //get the index of the splat
    int index = int(gl_GlobalInvocationID.x);
    //get the mean of the splat
    vec3 mean = means3D.data[index];
    //project the mean to the screen
    vec4 projectedMean = vpMatrix * vec4(mean, 1.0);
    //divide by the w component
    projectedMean /= projectedMean.w;
    //get the screen coordinates
    vec2 screenCoords = (projectedMean.xy + 1.0) / 2.0 * vec2(width, height);
    //check if the splat is within the screen
    if(screenCoords.x < 0 || screenCoords.x >= width || screenCoords.y < 0 || screenCoords.y >= height)
    {
        return;
    }
    //check the depth of the splat isn't behind the screen
    if(projectedMean.z < -1.0 || projectedMean.z > 1.0)
    {
        return;
    }
    // round the screen coordinates
    ivec2 screenCoordsI = ivec2(screenCoords);
    //screenCoordsI = ivec2(100,100);
    //draw a 10x10 square around the screen coordinates
    for(int x = screenCoordsI.x; x < screenCoordsI.x + 10; x++)
    {
        for (int y = screenCoordsI.y; y < screenCoordsI.y + 10; y++)
        {
            //check if the pixel is within the screen
            if (x >= 0 && x < width && y >= 0 && y < height)
            {
                //draw white
                imageStore(outputImage, ivec2(x, y), vec4(255.0, 0, 0, 1.0));
            }
        }
    }
    //colour the screen edge green
    for(int x = 0; x < width; x++)
    {
        imageStore(outputImage, ivec2(x, 0), vec4(0.0, 255.0, 0.0, 1.0));
        imageStore(outputImage, ivec2(x, height - 1), vec4(0.0, 255.0, 0.0, 1.0));
    }
    //imageStore(outputImage, ivec2(gl_LocalInvocationID.xy), vec4(1.0, 1.0, 1.0, 1.0));
}