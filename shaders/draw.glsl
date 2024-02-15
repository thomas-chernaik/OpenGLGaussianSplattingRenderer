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
} incides;

//num splats uniform
uniform layout(location = 0) int numSplats;
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
    //return;
    //imageStore(outputImage, uv, vec4(uv.x / float(screenWidth), uv.y / float(screenHeight), 0, 1));
    //get tile index from the pixel position, width and height
    //there are 16x16 tiles
    //int tileIndex = (uv.x / tileWidth) + (uv.y / tileHeight) * 16;
    int widthDiv16 = screenWidth / 16;
    int heightDiv16 = screenHeight / 16;
    int x = uv.x / widthDiv16;
    int y = uv.y / heightDiv16;
    int tileIndex = x + y * 16;
    //get the start and end index of the splats for this tile
    //the start index is at location tileIndex-1, or 0 if tileIndex is 0
    //the end index is at location tileIndex
    int startIndex;
    if(tileIndex == 0) {
        startIndex = 0;
    } else {
        startIndex = bins.data[tileIndex - 1];
    }
    int endIndex = bins.data[tileIndex];

    //now we have the start and end index, we can loop through the splats, sample, and blend
    vec4 blendedColour = vec4(0, 0, 0, 0);
    int counter = 0;
    for(int i = startIndex; i < endIndex; i++) {
        int index = i;
        //get the projected mean
        vec2 projectedMean = projectedMeans.data[index];
        //get the vector from the pixel to the projected mean
        vec2 pixelToProjectedMean = projectedMean - vec2(uv.x, uv.y);
        //normalise the vector by dividing by the screen width and height
        //pixelToProjectedMean = vec2(pixelToProjectedMean.x / float(screenWidth), pixelToProjectedMean.y / float(screenHeight));
        pixelToProjectedMean *= 5;
        //sample the covariance matrix to get the opacity at this pixel
        //TODO: sample the covariance matrix//DEBUG: increment the counter
        float power = -0.5 * (conicOpacities.data[index].x * pixelToProjectedMean.x * pixelToProjectedMean.x + conicOpacities.data[index].z * pixelToProjectedMean.y * pixelToProjectedMean.y) - conicOpacities.data[index].y * pixelToProjectedMean.x * pixelToProjectedMean.y;
        if (power > 0) {
            continue;
        }
        //multiply the gaussian opacity with the regular opacity
        float alpha = min(0.99, exp(power) * conicOpacities.data[index].w);
        //alpha = conicOpacities.data[index].w;
        //skip if the calculated opacity is less than 0.01
        if (alpha < 0.01) {
            continue;
        }
        counter++;

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
    //DEBUG
    //go through the splats, count the number within 2 pixels of the current pixel
    vec4 blendedColour2 = vec4(0, 0, 0, 0);
    int counter2 = 0;
    for(int i = startIndex; i < endIndex; i++) {
        //lets check if the splat is within 2 pixels of the current pixel
        int index = incides.data[i];
        //get the projected mean
        vec2 projectedMean = projectedMeans.data[index];
        //get the vector from the pixel to the projected mean
        vec2 pixelToProjectedMean = projectedMean - vec2(uv);
        //if the splat is within 20 pixels, increment the counter
        if(abs(pixelToProjectedMean.x) < 2 && abs(pixelToProjectedMean.y) < 2) {
            counter2++;
            //add the colour to the blended colour
            //if(counter2 < 1)
                blendedColour2 = vec4(colour.data[index].rgb, 1);

        }

    }
    float counter2f = counter2;
    //change the colour space from 0-255 to 0-1, only in the RGB channels
    float alpha = blendedColour.a;
    blendedColour = blendedColour / 255.0;
    blendedColour.a = alpha;
    //write the blended colour to the output image
    //imageStore(outputImage, uv, blendedColour);
    //imageStore(outputImage, uv, blendedColour2 / (255.0));
    //imageStore(outputImage, uv, vec4(counter2f, counter2f, counter2f, 1));
    //debug output the number of splats for this pixel
    float numSplatsFloat = float(endIndex - startIndex);
    //numSplatsFloat = 10;
    //imageStore(outputImage, uv, vec4(numSplatsFloat, numSplatsFloat, numSplatsFloat, 1));
    //debug output white
    //imageStore(outputImage, uv, vec4(1, 1, 1, 1));
    //imageStore(outputImage, uv, vec4(startIndex, startIndex, startIndex, 1));

}
