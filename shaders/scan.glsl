#version 430 core
//the size of the work group
layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;


//the input buffer of keys to be sorted
layout (std430, binding = 0)  buffer InputBuffer {
    vec2 data[];
} inputBuffer;
//the intermediate buffer for the keys so we don't have to sort in place
layout (std430, binding = 1) buffer IntermediateBuffer {
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

//the size of each key in bits
const int keySize = 16;

shared int globalPrefixSum[16];
void main() {
    //the segment we are sorting on
    int segment = segmentReadOnly;
    //the start index of the section
    int startIdx = int(gl_GlobalInvocationID.x) * sectionSize;
    //the mask to extract the bits we are sorting on
    uint mask = 0x000000000000000F << (segment * 4);

    //work out if we are working on the x values or the y values
    bool sortX = segment > 7;
    segment -= sortX ? 8 : 0;

    //if we are the first thread in the work group, we need to fetch the global histogram
    if(gl_LocalInvocationID.x == 0)
    {
        for(int i = 0; i < 16; i++)
        {
            globalPrefixSum[i] = globalHistogramsBuffer.data[i + (16*numSections)];
        }
    }
    //wait for the first thread to finish
    barrier();
    //now we fetch the local offsets
    int offsets[16];
    for(int i = 0; i < 16; i++)
    {
        offsets[i] = globalHistogramsBuffer.data[i + (16*int(gl_GlobalInvocationID.x))];
    }
    /*
    //now we need to work out where each key will go in the output buffer
    //we use two values
    //the prefix sum, which is the sum of all values less than the current value
    //the offset, which is the number of values the same as the current value, but lefter in the buffer
    int offsets[keySize];
    int globalHistogram[keySize];
    int globalPrefixSum[keySize];
    for (int i = 0; i < keySize; i++) {
        globalHistogram[i] = 0;
    }

    //compute the global histogram and the offsets
    for (int i = 0; i < numSections; i++)
    {
        for (int j = 0; j < keySize; j++)
        {
            //if we are at our section, we need to compute the offset
            if (i == int(gl_GlobalInvocationID.x))
            {
                offsets[j] = globalHistogram[j];
            }
            //add the value to the global histogram
            globalHistogram[j] += globalHistogramsBuffer.data[i * keySize + j];
        }
    }

    //compute the global prefix sum
    globalPrefixSum[0] = 0;
    for (int i = 1; i < 16; i++)
    {
        globalPrefixSum[i] = globalPrefixSum[i - 1] + globalHistogram[i - 1];
    }
    */
    int outputSize = 64;
    int outputt[64];
    uint index;
    //write the output
    for (int i = startIdx; i < startIdx + sectionSize; i+=outputSize)
    {
        for(int j = 0; j < outputSize; j++)
        {
            if(sortX)
                index = uint((floatBitsToUint(inputBuffer.data[orderBuffer.data[i+j]].x) & mask) >> (segment * 4));
            else
                index = uint((floatBitsToUint(inputBuffer.data[orderBuffer.data[i+j]].y) & mask) >> (segment * 4));
            int sortedIdx = globalPrefixSum[index] + offsets[index];
            //int dataToWrite = orderBuffer.data[i+j];
            //outputBuffer.data[sortedIdx] = dataToWrite;
            outputt[j] = sortedIdx;
            offsets[index]++;
        }
        for(int j = 0; j < outputSize; j++)
        {
            intermediateBuffer.data[outputt[j]] = orderBuffer.data[i+j];
        }

    }
}
