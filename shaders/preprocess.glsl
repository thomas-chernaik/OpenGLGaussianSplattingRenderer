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

//the number of splats uniform
layout (location = 1) uniform int numSplats;

//screen width and height uniform
layout (location = 2) uniform int width;
layout (location = 3) uniform int height;

//output
//tiles touched and depth as a vec2
layout(std430, binding = 3) buffer Keys
{
    vec2 data[];
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
const float far = 100.0;
const float allOnes = 0xFFFFFFFF;

vec3 get2DCovariance(mat3 covariance);
float normalisedSpaceToPixelSpace(float value, int numPixels);

void main() {
    //get the index of the splat
    int index = int(gl_GlobalInvocationID.x);
    //if the index is greater than the number of splats, return
    if (index >= numSplats) {
        //set keys to a value we can recognise and ignore later
        keys.data[index] = vec2(allOnes, allOnes);

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
    if (depth < near || depth > far) {
        return;
    }

    //get the opacity
    float opacity = opacities.data[index];
    //if the opacity is zero, return
    if (opacity == 0.0) {
        keys.data[index] = vec2(allOnes, allOnes);
        return;
    }
    vec3 cov= vec3(covariances3D.data[index * 3 + 0], covariances3D.data[index * 3 + 1], covariances3D.data[index * 3 + 2]);
    //get the covariance matrix, which is diagonal
    mat3 covariance = mat3(cov.x, cov.y, cov.z, cov.y, cov.x, cov.y, cov.z, cov.y, cov.x);
    //compute the 2D projection of the covariance matrix
    vec3 covariance2D = get2DCovariance(covariance);
    //compute the determinant of the covariance matrix
    float determinant = (cov.x * cov.z) - (cov.y * cov.y);
    if (determinant <= 0.0) {
        return;
    }
    float inverseDeterminant = 1.0 / determinant;
    vec3 conic = vec3(covariance2D.z, -covariance2D.y, covariance2D.x) * inverseDeterminant;
    //compute a bounding box for the splat
    //this will be square, as we compute it via the largest eigenvalue, which could be pointing in any direction
    //calculate the eigenvalues of the covariance matrix
    float mid = (cov.x + cov.z) * 0.5;
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
    int numOverlappingTiles = 0;
    int keysToWrite = 0;
    for(int x=minX; x<=maxX; x++) {
        for (int y=minY; y<=maxY; y++) {
            //if we've already found 8 overlapping tiles, return
            if (numOverlappingTiles == 8) {
                keys.data[index] = vec2(allOnes, allOnes);
                return;
            }
            //calculate the index of the tile
            int tileIndex = x + y * 16;
            //set the key to the 4*numOverlappingTiles to the tile index
            keysToWrite &= (tileIndex << (4 * numOverlappingTiles));
            //increment the number of overlapping tiles
            numOverlappingTiles++;

        }
    }
    //if the number of overlapping tiles is zero, return
    if (numOverlappingTiles == 0 || numOverlappingTiles > 8) {
        keys.data[index] = vec2(allOnes, allOnes);
        return;
    }
    //set the values
    keys.data[index] = vec2(keysToWrite, depth);
    conicOpacities.data[index] = vec4(conic, opacity);
    projectedMeans.data[index] = projectedMeanPixelSpace;







}

//converts from a value between -1 and 1 to a value between 0 and numPixels
float normalisedSpaceToPixelSpace(float value, int numPixels) {
    return ((value + 1.0) * numPixels - 1.0) * 0.5;
}

//projects a 3D covariance matrix to 2D
vec3 get2DCovariance(mat3 covariance) {
    
}
