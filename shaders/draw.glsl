#version 430 core
layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

//bins buffer
layout(std430, binding = 0) buffer Bins {
    int data[];
} bins;
//projected means buffer
layout(std430, binding = 1) buffer ProjectedMeans {
    vec2 data[];
} projectedMeans;

//projected covariances buffer
layout(std430, binding = 2) buffer ConicOpacities
{
    vec4 data[];
} conicOpacities;

//colour buffer
layout(std430, binding = 3) buffer Colour {
    vec4 data[];
} colour;

//index buffer
layout(std430, binding = 4) buffer Indices {
    int data[];
} indices;

//splat key buffer
layout(std430, binding = 5) buffer SplatKeys {
    int data[];
} splatKeys;

//bounding radii buffer
layout(std430, binding = 6) buffer BoundingRadii {
    float data[];
} boundingRadii;
//num splats uniform
uniform layout(location = 0) int numSplats;
//screen width uniform
uniform layout(location = 2) int screenWidth;
//screen height uniform
uniform layout(location = 3) int screenHeight;
//tile width uniform
uniform layout(location = 4) float tileWidth;
//tile height uniform
uniform layout(location = 5) float tileHeight;

//output image2D
layout(rgba8, binding = 0) writeonly uniform image2D outputImage;
vec4 alphaBlend(vec4 colour1, vec4 colour2)
{
    float alpha1 = colour1.a;
    float alpha2 = colour2.a;
    float remainingAlpha = 1.0 - alpha1;
    float alphaToBlend = alpha2 * remainingAlpha;
    vec3 blendedColour = vec3(colour1) + vec3(colour2) * alphaToBlend;
    return vec4(blendedColour, alpha1 + alphaToBlend);
}

void main() {
    //get the pixel position
    int x = int(gl_GlobalInvocationID.x);
    int y = int(gl_GlobalInvocationID.y);
    ivec2 pixelPosition = ivec2(x, y);
    // initialise the colour
    vec4 col = vec4(0.0, 0.0, 0.0, 0.0);
    //get the tile index
    int tileX = int(pixelPosition.x / tileWidth);
    int tileY = int(pixelPosition.y / tileHeight);
    int tileIndex = tileY * 16 + tileX;
    int start;
    if (tileIndex == 0) {
        start = 0;
    } else {
        start = bins.data[tileIndex - 1];
    }
    int end = bins.data[tileIndex];
    //end = min(start + 50000, end);

    //loop through the splats in the tile
    for (int i=start; i<end; i++)
    {
        int index = splatKeys.data[indices.data[i]];
        vec2 projectedMean = projectedMeans.data[index];
        float radius = boundingRadii.data[index];
        vec4 conic = conicOpacities.data[index];
        vec2 distance = pixelPosition - projectedMean;
        //get the value of the conic
        float power = -0.5 * (conic.x * distance.x * distance.x + conic.z * distance.y * distance.y) -
        conic.y * distance.x * distance.y;

        if (power > 0.0)
        {
            continue;
        }
        float alpha = min(0.99, exp(power) * conic.w);
        if (alpha < 1.0 / 255.0)
        {
            continue;
        }

        col = alphaBlend(col, vec4(colour.data[index].xyz, alpha));
        if (col.a >= 0.99)
        {
            break;
        }

    }
    //convert col from 0-255 to 0-1
    col = col / 255.0;
    imageStore(outputImage, pixelPosition, col);
}
