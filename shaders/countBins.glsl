#version 430 core

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

//the array of indexes to count in
layout(std430, binding = 0) buffer DepthBuffer
{
    float data[];
} depthBuffer;

//the length of the array
layout(location = 0) uniform int length;

//the buffer of values to count in
layout(std430, binding = 1) buffer BinsBuffer
{
    uint data[];
} binsBuffer;

void main() {
    //get the index of the thread
    int index = int(gl_GlobalInvocationID.x);

    //if the index is out of bounds, return
    if(index > length) return;

    //get the value at the index
    int value = int(depthBuffer.data[index]);
    if(value < 0 || value >= 256) return;
    //increment the bin at the value
    atomicAdd(binsBuffer.data[value], 1);
}