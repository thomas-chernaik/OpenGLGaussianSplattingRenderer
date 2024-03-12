#version 430 core
layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = 0) buffer DepthBufferToPad {
    float data[];
} depthBufferToPad;

layout (std430, binding = 1) buffer OrderBufferToPad {
    int data[];
} orderBufferToPad;

layout (std430, binding = 2) buffer IntermediaryBufferToPad {
    int data[];
} intermediaryBufferToPad;

//uniform of the size of the buffer
layout (location = 0) uniform int bufferSize;
//uniform of the size of the padding
layout (location = 1) uniform int paddingSize;

void main() {
    //get the global index
    int index = int(gl_GlobalInvocationID.x);
    orderBufferToPad.data[0] = 10;
    intermediaryBufferToPad.data[0] = 10;
//    //if the index is less than the padding size
//    if (index < paddingSize) {
//        //set the value to large
//        depthBufferToPad.data[bufferSize + index] = 10000.0;
//        orderBufferToPad.data[0] = 10;
//        intermediaryBufferToPad.data[0] = 10;
//    }
}