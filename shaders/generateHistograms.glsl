#version 430 core

//the size of the work group
layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
//the input buffer of keys to be sorted
layout (std430, binding = 0)  buffer InputBuffer {
    float data[];
} inputBuffer;
//the order buffer
layout (std430, binding = 1) buffer OrderBuffer {
    int data[];
} orderBuffer;
//the global histograms, size keySize * numSections
layout (std430, binding = 2) buffer GlobalHistograms {
    int data[];
} globalHistogramsBuffer;


//the number of sections, equivalent to the total number of threads
layout ( location=0 ) uniform int numSections;
//the size of each segment (number of values each thread is responsible for)
layout ( location=1 ) uniform int sectionSize;
//the current segment we are sorting on
layout ( location=2 ) uniform int segmentReadOnly;
//the size of the buffer
layout ( location=3 ) uniform int bufferSize;

const int keySize = 16;

void main()
{
    //the current segment we are sorting on
    uint segment = uint(segmentReadOnly);
    //the start index of the section
    int startIdx = int(gl_GlobalInvocationID.x) * sectionSize;
    if(startIdx >= bufferSize) {
        return;
    }
    //the start index of the section in terms of keys
    int startIdx2 = int(gl_GlobalInvocationID.x) * keySize;
    //the mask to extract the bits we are sorting on
    uint mask = 0x000000000000000F << (segment * 4);

    //create the local histogram
    //this is the histogram for the section from startIdx to startIdx + sectionSize
    int histogram[keySize];
    for (int i = 0; i < keySize; i++) {
        histogram[i] = 0;
    }
    int localSectionSize = sectionSize;
    if(startIdx + localSectionSize > bufferSize) {
        localSectionSize = bufferSize - startIdx;
    }
    uint index;
    //compute the local histogram
    for (int i = startIdx; i < startIdx + localSectionSize; i++) {
        uint key = floatBitsToUint(inputBuffer.data[orderBuffer.data[i]]);
        //extract the bits we are sorting on
        index = ((key) & mask) >> (segment * 4);
        histogram[index]++;
    }

    //write the histogram to the global memory, from startIdx2 to startIdx2 + keySize
    for (int i = 0; i < keySize; i++) {
        globalHistogramsBuffer.data[startIdx2 + i] = histogram[i];
    }

}
