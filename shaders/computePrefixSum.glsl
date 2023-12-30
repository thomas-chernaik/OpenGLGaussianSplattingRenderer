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
    barrier();
    //if we are the first thread, calculate the global prefix sum, and store it in the global histogram buffer at the end
    if(localIndex == 0)
    {
        int globalPrefixSum = 0;
        int index = numSections * 16;
        for(int i=0; i<16; i++)
        {
            globalHistogramsBuffer.data[index + i] = globalPrefixSum;
            globalPrefixSum += histogram[i];
        }
    }

}
