#version 430 core
layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

//histogram buffer
layout(std430, binding = 0) buffer Histogram {
    int data[];
} histogram;

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
} incides;

//num splats uniform
uniform layout(location = 0) int numSplats;
//the offset to find the prefix sum in the histogram
uniform layout(location = 1) int histogramOffset;
//screen width uniform
uniform layout(location = 2) int screenWidth;
//screen height uniform
uniform layout(location = 3) int screenHeight;

//output image2D
layout(rgba8, binding = 0) writeonly uniform image2D outputImage;


void main() {
    //get global UV coordinates
    ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
    //debug for now, output the white to the pixel
    //imageStore(outputImage, uv, vec4(1, 1, 1, 1));
    //imageStore(outputImage, uv, vec4(uv.x / float(screenWidth), uv.y / float(screenHeight), 0, 1));
    //get tile index from the work group index
    int tileIndex = int(gl_WorkGroupID.x);
    //get the start and end index of the splats for this tile
    //the start index is at location offset + tileIndex + 1
    //the end index is at location offset + tileIndex + 2, or if it is the last tile, it is numSplats
    int startIndex = histogram.data[histogramOffset + tileIndex + 1];
    int endIndex;
    if(tileIndex == gl_NumWorkGroups.x * gl_NumWorkGroups.y - 1) {
        endIndex = numSplats;
    } else {
        endIndex = histogram.data[histogramOffset + tileIndex + 2];
    }
    //now we have the start and end index, we can loop through the splats, sample, and blend
    vec4 blendedColour = vec4(0, 0, 0, 0);
    for(int i = startIndex; i < endIndex; i++) {
        int index = incides.data[i];
        //get the projected mean
        vec2 projectedMean = projectedMeans.data[index];
        //get the vector from the pixel to the projected mean
        vec2 pixelToProjectedMean = projectedMean - vec2(uv);
        //sample the covariance matrix to get the opacity at this pixel
        //TODO: sample the covariance matrix
        float power = -0.5 * (conicOpacities.data[index].x * pixelToProjectedMean.x * pixelToProjectedMean.x + conicOpacities.data[index].z * pixelToProjectedMean.y * pixelToProjectedMean.y) - conicOpacities.data[index].y * pixelToProjectedMean.x * pixelToProjectedMean.y;
        if (power > 0) {
            continue;
        }
        //multiply the gaussian opacity with the regular opacity
        float alpha = min(0.99, exp(power) * conicOpacities.data[i].w);
        //skip if the calculated opacity is less than 0.01
        if (alpha < 0.01) {
            continue;
        }
        //get remaining alpha in the pixel
        float remainingAlpha = 1.0 - blendedColour.a;
        //calculate the alpha to blend with
        float alphaToBlend = alpha * remainingAlpha;
        //calculate the alpha to blend with the existing colour
        blendedColour = blendedColour + vec4(colour.data[index].rgb * alphaToBlend, alphaToBlend);
        //if the blended colour is opaque, we can stop blending
        if(blendedColour.a >= 0.99) {
            break;
        }
    }

    //write the blended colour to the output image
    //imageStore(outputImage, uv, blendedColour);
    //debug output the number of splats for this pixel
    float numSplatsFloat = float(endIndex - startIndex) / float(gl_NumWorkGroups.x * gl_NumWorkGroups.y);
    imageStore(outputImage, uv, vec4(numSplatsFloat, numSplatsFloat, numSplatsFloat, 1));

}
