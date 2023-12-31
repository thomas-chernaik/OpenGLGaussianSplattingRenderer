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

//num splats uniform
uniform layout(location = 0) int numSplats;
//screen width uniform
uniform layout(location = 1) int screenWidth;
//screen height uniform
uniform layout(location = 2) int screenHeight;

//output image2D
layout(rgba8, binding = 0) writeonly uniform image2D outputImage;


void main() {
    //get global UV coordinates
    ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
    //debug for now, output the white to the pixel
    //imageStore(outputImage, uv, vec4(1, 1, 1, 1));
    imageStore(outputImage, uv, vec4(uv.x / float(screenWidth), uv.y / float(screenHeight), 0, 1));
}
