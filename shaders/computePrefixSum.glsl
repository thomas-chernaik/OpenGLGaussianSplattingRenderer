#version 430 core

layout(local_size_x = 16, local_size_y = 1, local_size_z = 1) in;

//the global histograms, size keySize * numSections
layout (std430, binding = 2) buffer GlobalHistograms {
    int data[];
} globalHistogramsBuffer;

//the number of sections in the global histogram
layout (location = 0) uniform int numSections;

//the prefix sum of the global histogram in shared memory
shared int histogram[16];

void main() {
    //get the local index
    int localIndex = int(gl_GlobalInvocationID.x);
    int prefixSum = 0;
    //for each section
    for(int i=localIndex; i<numSections * 16; i+=16)
    {
        //get the value
        int value = globalHistogramsBuffer.data[i];
        //store the prefix sum
        globalHistogramsBuffer.data[i] = prefixSum;
        //add the value to the prefix sum
        prefixSum += value;
    }
    //store the histogram in shared memory
    histogram[localIndex] = prefixSum;
    //wait for all threads to finish with a shared memory barrier
    memoryBarrierShared();

    //compute the global prefix sum
    for(uint i=0; i<4; i++)
    {
        uint offset = 1 << i;
        if(localIndex + offset < 16)
        {
            histogram[localIndex + offset] += histogram[localIndex];
        }
        barrier();
    }
    int index = numSections * 16;
    if(localIndex == 0)
        globalHistogramsBuffer.data[index + localIndex] = 0;
    else
        globalHistogramsBuffer.data[index + localIndex] = histogram[localIndex-1];

}
