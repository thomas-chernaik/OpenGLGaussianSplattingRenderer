#version 430 core
layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

//the members here aren't called data because it gave me an error I have no clue why?
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
//tiles touched and depth as a vec2
layout(std430, binding = 3) buffer Keys
{
    uvec2 data[];
} keys;

//conic opacities
layout(std430, binding = 4) buffer ConicOpacities
{
    vec4 data[];
} conicOpacities;

//projected means
layout(std430, binding = 5) buffer ProjectedMeans
{
    vec2 data[];
} projectedMeans;


const float near = 0.01;
const float far = 1000.0;
const uint allOnes = 0xFFFFFFFF;

vec3 get2DCovariance(mat3 covariance, vec3 projectedMean, mat3 rotationMatrix);
float normalisedSpaceToPixelSpace(float value, int numPixels);

void main() {
    //get the index of the splat
    int index = int(gl_GlobalInvocationID.x);
    //if the index is greater than the number of splats, return
    if (index >= numSplats) {
        //set keys to a value we can recognise and ignore later
        keys.data[index] = uvec2(allOnes, allOnes);

        return;
    }
    //initialise the key
    vec2 key = vec2(0.0, 0.0);
    //get the mean
    vec3 mean = means3D.data[index];
    //project the mean
    vec4 projectedMean = vpMatrix * vec4(mean, 1.0);
    //divide by w to get the projected mean
    projectedMean *=  1./max(projectedMean.w, 0.0001);
    //get the depth
    float depth = projectedMean.z;
    //near and far cull
    if (depth < near) {
        keys.data[index] = uvec2(allOnes, allOnes);
        return;
    }

    //get the opacity
    float opacity = opacities.data[index];
    //if the opacity is zero, return
    if (opacity < 0.05) {
        keys.data[index] = uvec2(allOnes, allOnes);
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
    if (determinant <= 0.0) {
        keys.data[index] = uvec2(allOnes, allOnes);
        return;
    }
    float inverseDeterminant = 1.0 / determinant;
    vec3 conic = vec3(covariance2D.z, -covariance2D.y, covariance2D.x) * inverseDeterminant;
    //compute a bounding box for the splat
    //this will be square, as we compute it via the largest eigenvalue, which could be pointing in any direction
    //calculate the eigenvalues of the covariance matrix
    float mid = (covariance2D.x + covariance2D.z) * 0.5;
    float eigenvalue1 = mid + sqrt(max(0.1, mid * mid - determinant));
    float eigenvalue2 = mid - sqrt(max(0.1, mid * mid - determinant));
    //calculate the radius of the splat, 3 is a magic number that seems to work well
    float radius = ceil(3. * sqrt(max(eigenvalue1, eigenvalue2)));
    //convert the projected mean to pixel space
    vec2 projectedMeanPixelSpace = vec2(normalisedSpaceToPixelSpace(projectedMean.x, width), normalisedSpaceToPixelSpace(projectedMean.y, height));
    //calculate the bounding box
    int minX = int(projectedMeanPixelSpace.x - radius);
    int maxX = int(projectedMeanPixelSpace.x + radius);
    int minY = int(projectedMeanPixelSpace.y - radius);
    int maxY = int(projectedMeanPixelSpace.y + radius);
    //convert the boundings to numbers 0 to 15
    int widthDiv16 = width / 16;
    int heightDiv16 = height / 16;
    minX /= widthDiv16;
    maxX /= widthDiv16;
    minY /= heightDiv16;
    maxY /= heightDiv16;
    //if mins are more than 15, or maxes are less than 0, return
    if (minX > 15 || maxX < 0 || minY > 15 || maxY < 0) {
        keys.data[index] = uvec2(allOnes, allOnes);
        return;
    }
    //clamp the bounding box to the screen
    minX = clamp(minX, 0, 15);
    maxX = clamp(maxX, 0, 15);
    minY = clamp(minY, 0, 15);
    maxY = clamp(maxY, 0, 15);
    int numOverlappingTiles = 0;
    uint keysToWrite = 0;
    for(int x=minX; x<=maxX; x++) {
        for (int y=minY; y<=maxY; y++) {
            //if we've already found 8 overlapping tiles, return
            if (numOverlappingTiles == 4) {
                keys.data[index] = uvec2(allOnes, allOnes);
                return;
            }
            //calculate the index of the tile
            uint tileIndex = uint(x) + uint(y) * 16;
            //set the key to the 4*numOverlappingTiles to the tile index
            keysToWrite |= (tileIndex << (8 * numOverlappingTiles));
            //increment the number of overlapping tiles
            numOverlappingTiles++;

        }
    }
    //if the number of overlapping tiles is zero, return
    if (numOverlappingTiles == 0 || numOverlappingTiles > 4) {
        keys.data[index] = uvec2(allOnes, allOnes);
        return;
    }
    //convert the depth to an integer
    uint depthInt = uint(depth * 1000.0);
    //cap the depth to 20 bits. This allows us to do less passes for the sorting
    depthInt = clamp(depthInt, 0, 1048575);
    //set the values
    keys.data[index] = uvec2(keysToWrite, depthInt);
    conicOpacities.data[index] = vec4(conic, opacity);
    projectedMeans.data[index] = projectedMeanPixelSpace;







}

//converts from a value between -1 and 1 to a value between 0 and numPixels
float normalisedSpaceToPixelSpace(float value, int numPixels) {
    return ((value + 1.0) * numPixels - 1.0) * 0.5;
}

//projects a 3D covariance matrix to 2D
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
