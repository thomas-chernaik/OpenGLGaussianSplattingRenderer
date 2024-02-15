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

vec3 get2DCovariance(mat3 covariance, vec3 projectedMean, mat3 rotationMatrix);


void main()
{

    //colour the screen edge green
    for(int x = 0; x < width; x++)
    {
        imageStore(outputImage, ivec2(x, 0), vec4(0.0, 255.0, 0.0, 1.0));
        imageStore(outputImage, ivec2(x, height - 1), vec4(0.0, 255.0, 0.0, 1.0));
    }
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

    //get the covariance matrix, which is diagonal
    mat3 covariance3D = mat3(covariances3D.data[index * 6], covariances3D.data[index * 6 + 1], covariances3D.data[index * 6 + 2],
    covariances3D.data[index * 6 + 1], covariances3D.data[index * 6 + 3], covariances3D.data[index * 6 + 4],
    covariances3D.data[index * 6 + 2], covariances3D.data[index * 6 + 4], covariances3D.data[index * 6 + 5]);

    //compute the 2D projection of the covariance matrix
    vec3 covariance2D = get2DCovariance(covariance3D, projectedMean.xyz, rotationMatrix);
    //compute the determinant of the covariance matrix
    float determinant = (covariance2D.x * covariance2D.z) - (covariance2D.y * covariance2D.y);

    float inverseDeterminant = 1.0 / determinant;
    vec3 conic = vec3(covariance2D.z, -covariance2D.y, covariance2D.x) * inverseDeterminant;

    //compute a bounding box for the splat with its eigenvalues
    float mid = (covariance2D.x + covariance2D.z) * 0.5;
    float eigenvalue1 = mid + sqrt(max(0.1, mid * mid - determinant));
    float eigenvalue2 = mid - sqrt(max(0.1, mid * mid - determinant));
    //calculate the radius of the splat, 3 is a magic number that seems to work well
    float radius = ceil(3. * sqrt(max(eigenvalue1, eigenvalue2)));
    radius = 10;
    //screenCoords = vec2(100, 100);

    //draw the splat
    for(int x=int(screenCoords.x - radius); x<=int(screenCoords.x + radius); x++)
    {
        for(int y=int(screenCoords.y - radius); y<=int(screenCoords.y + radius); y++)
        {
            //check if the pixel is within the screen
            if(x < 0 || x >= width || y < 0 || y >= height)
            {
                continue;
            }
            // draw the pixel
            imageStore(outputImage, ivec2(x, y), vec4(1.0, 0.0, 0.0, 1.0));
        }
    }




    //imageStore(outputImage, ivec2(gl_LocalInvocationID.xy), vec4(1.0, 1.0, 1.0, 1.0));
}

vec3 get2DCovariance(mat3 covariance, vec3 projectedMean, mat3 rotationMatrix)
{
    //the original paper clamps the projected mean inside the view frustum, but I don't think this is necessary
    float lPrime = length(projectedMean);
    //equation 29 from https://www.cs.umd.edu/~zwicker/publications/EWASplatting-TVCG02.pdf
    //the splatting code from the paper uses a different jacobian, but I think this one is correct
    mat3 jacobian = mat3(
    1/projectedMean.z, 0, -projectedMean.x/(projectedMean.z*projectedMean.z),
    0, 1/projectedMean.z, -projectedMean.y/(projectedMean.z*projectedMean.z),
    0,0,0);

    //equation 30 from https://www.cs.umd.edu/~zwicker/publications/EWASplatting-TVCG02.pdf
    mat3 cov2D = jacobian * rotationMatrix * covariance * transpose(rotationMatrix) * transpose(jacobian);

    //apply low pass filter, provides some anti-aliasing, probs better methods
    cov2D[0][0] += 0.3;
    cov2D[1][1] += 0.3;
    return vec3(cov2D[0][0], cov2D[0][1], cov2D[1][1]);

}
