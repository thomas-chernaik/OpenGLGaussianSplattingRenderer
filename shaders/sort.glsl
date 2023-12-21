//sort.glsl

#version 430 core
//the size of the work group
layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;
//the input buffer of keys to be sorted
layout (binding = 0)  buffer InputBuffer {
    int data[];
} inputBuffer;
//the output buffer of sorted keys
layout (binding = 1) buffer OutputBuffer {
    int data[];
} outputBuffer;
//the global histograms, size keySize * numSections
layout (binding = 2) buffer GlobalHistograms {
    int data[];
} globalHistogramsBuffer;

//the number of sections, equivalent to the total number of threads
layout ( location=0 ) uniform int numSections;
//the size of each segment (number of values each thread is responsible for)
layout ( location=1 ) uniform int sectionSize;

//the size of each key in bits
const int keySize = 16;

void main()
{
    //we need to compute this 8 times. Each time we sort based on a different 4 bits
    for (int segment = 0; segment < 8; segment++)
    {
        //the start index of the section
        int startIdx = int(gl_GlobalInvocationID.x) * sectionSize;
        //the start index of the section in terms of keys
        int startIdx2 = int(gl_GlobalInvocationID.x) * keySize;
        //the mask to extract the bits we are sorting on
        int mask = 0x0000000F << (segment * 4);

        //create the local histogram
        //this is the histogram for the section from startIdx to startIdx + sectionSize
        int histogram[keySize];
        for (int i = 0; i < keySize; i++) {
            histogram[i] = 0;
        }
        //compute the local histogram
        for (int i = startIdx; i < startIdx + sectionSize; i++) {
            int index = (int(inputBuffer.data[i]) & mask) >> (segment * 4);
            histogram[index]++;
        }

        //write the histogram to the global memory, from startIdx2 to startIdx2 + keySize
        for (int i = 0; i < keySize; i++) {
            globalHistogramsBuffer.data[startIdx2 + i] = histogram[i];
        }

        //wait for all threads to finish writing to the shared memory
        memoryBarrier();

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

        //write the output
        for (int i = startIdx; i < startIdx + sectionSize; i++)
        {
            int index = (int(inputBuffer.data[i]) & mask) >> (segment * 4);
            int sortedIdx = globalPrefixSum[index] + offsets[index];
            outputBuffer.data[sortedIdx] = inputBuffer.data[i];
            offsets[index]++;
        }


        //wait for all threads to finish writing to the output buffer
        memoryBarrier();
        //copy the output buffer to the input buffer for just this section
        for (int i = startIdx; i < startIdx + sectionSize; i++)
        {
            inputBuffer.data[i] = outputBuffer.data[i];
        }
        //wait for all threads to finish writing to the input buffer
        memoryBarrier();
    }
}

/*
// computeShader.glsl
#version 430 core
layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;
layout (binding = 0)  buffer InputBuffer {
    int data[];
} inputBuffer;


//the output buffer
layout (binding = 1) buffer OutputBuffer {
    int data[];
} outputBuffer;

//the shared histogram in global memory
layout (binding = 2) buffer SharedHistogram {
    int data[];
} sharedHistogramBuffer;

layout ( location=0 ) uniform int numSections;
layout ( location=1 ) uniform int sectionSize;

const int keySize = 16;

void main()
{
    for (int segment = 0; segment < 8; segment++)
    {
        int startIdx = int(gl_GlobalInvocationID.x) * sectionSize;
        int startIdx2 = int(gl_GlobalInvocationID.x) * keySize;

        int mask = 0x0000000F << (segment * 4);
        int histogram[keySize];
        for (int i = 0; i < keySize; i++) {
            histogram[i] = 0;
        }

        for (int i = startIdx; i < startIdx + sectionSize; i++) {
            int index = (int(inputBuffer.data[i]) & mask) >> (segment * 4);
            histogram[index]++;
        }
        //write the histogram to the global memory
        for (int i = startIdx; i < startIdx + keySize; i++) {
            sharedHistogramBuffer.data[i] = histogram[i - startIdx];
        }

        //wait for all threads to finish writing to the shared memory
        memoryBarrier();

        int offsets[keySize];
        int globalHistogram[keySize];
        int globalPrefixSum[keySize];
        for (int i = 0; i < keySize; i++) {
            offsets[i] = 0;
            globalHistogram[i] = 0;
        }

        //compute the global histogram and the offsets
        for (int i = 0; i < numSections * sectionSize; i += sectionSize)
        {
            for (int j = 0; j < keySize; j++)
            {
                if (i == startIdx2)
                {
                    offsets[j] = globalHistogram[j];
                }
                globalHistogram[j] += sharedHistogramBuffer.data[i + j];
            }
        }
        //compute the global prefix sum
        globalPrefixSum[0] = 0;
        for (int i = 1; i < keySize; i++)
        {
            globalPrefixSum[i] = globalHistogram[i - 1] + globalPrefixSum[i - 1];
        }
        //write the output
        for (int i = startIdx; i < startIdx + sectionSize; i++)
        {
            int inputNum = int(inputBuffer.data[i]);
            int index = (inputNum & mask) >> (segment * 4);
            outputBuffer.data[globalPrefixSum[index] + offsets[index]] = inputNum;
            offsets[index]++;
        }
        memoryBarrier();
        for (int i = startIdx; i < startIdx + sectionSize; i++)
        {
            inputBuffer.data[i] = outputBuffer.data[i];
        }
        memoryBarrier();
    }
}*/