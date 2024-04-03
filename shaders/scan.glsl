#version 430 core
//the size of the work group
layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;


//the input buffer of keys to be sorted
layout (std430, binding = 0)  buffer InputBuffer {
    float data[];
} inputBuffer;
//the intermediate buffer for the keys so we don't have to sort in place
layout (std430, binding = 1) buffer Inte256rmediateBuffer {
    int data[];
} intermediateBuffer;
//the output buffer for the order of the keys
layout (std430, binding = 3) buffer OrderBuffer {
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
//the segment
layout ( location=2 ) uniform int segmentReadOnly;
//the buffer size
layout ( location=3 ) uniform int bufferSize;

//the size of each key in bits
const int keySize = 16;

shared int globalPrefixSum[16];
void main() {
    //the segment we are sorting on
    int segment = segmentReadOnly;
    //the start index of the section
    int startIdx = int(gl_GlobalInvocationID.x) * sectionSize;
    if(startIdx >= bufferSize)
    {
        return;
    }
    int localSectionSize = sectionSize;
    if(startIdx + sectionSize > bufferSize)
    {
        localSectionSize = bufferSize - startIdx;
    }

    //the mask to extract the bits we are sorting on
    uint mask = 0x0000000F << (segment * 4);

    //work out if we are working on the x values or the y values
    bool sortX = segment > 7;
    segment -= sortX ? 8 : 0;

    //if we are the first thread in the work group, we need to fetch the global histogram
    if(gl_LocalInvocationID.x < 16)
    {
        globalPrefixSum[gl_LocalInvocationID.x] = globalHistogramsBuffer.data[gl_LocalInvocationID.x + (16*numSections)];
    }
    //wait for the first thread to finish
    barrier();
    //now we fetch the local offsets
    int offsets[16];
    for(int i = 0; i < 16; i++)
    {
        offsets[i] = globalHistogramsBuffer.data[i + (16*int(gl_GlobalInvocationID.x))];
    }
    uint index;
    //write the output
    for(int i=startIdx; i < startIdx + localSectionSize; i++)
    {
        uint key = floatBitsToUint(inputBuffer.data[orderBuffer.data[i]]);
        index = ((key) & mask) >> (segment * 4);
        int sortedIdx = globalPrefixSum[index] + offsets[index]++;
        intermediateBuffer.data[sortedIdx] = orderBuffer.data[i];
    }


}
