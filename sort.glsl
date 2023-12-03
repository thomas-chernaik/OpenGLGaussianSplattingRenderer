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

int numSegments = 256;
const int segmentSize = 16*128;
const int keySize = 16;

void main()
{
    for (int segment = 0; segment < 8; segment++)
    {
        int startIdx = int(gl_LocalInvocationID.x) * segmentSize;

        int mask = 0x0000000F << (segment * 4);
        int histogram[keySize];
        for (int i = 0; i < keySize; i++) {
            histogram[i] = 0;
        }

        for (int i = startIdx; i < startIdx + segmentSize; i++) {
            int index = (int(inputBuffer.data[i]) & mask) >> (segment * 4);
            histogram[index]++;
        }
        //write the histogram to the shared memory
        for (int i = startIdx; i < startIdx + segmentSize; i++) {
            sharedHistogramBuffer.data[i] = histogram[i - startIdx];
        }

        //wait for all threads to finish writing to the shared memory
        groupMemoryBarrier();

        int offsets[keySize];
        int globalHistogram[keySize];
        int globalPrefixSum[keySize];
        for (int i = 0; i < keySize; i++) {
            offsets[i] = 0;
            globalHistogram[i] = 0;
        }

        //compute the global histogram and the offsets
        for (int i = 0; i < numSegments * segmentSize; i += segmentSize)
        {
            for (int j = 0; j < keySize; j++)
            {
                if (i == startIdx)
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
        for (int i = startIdx; i < startIdx + segmentSize; i++)
        {
            int inputNum = int(inputBuffer.data[i]);
            int index = (inputNum & mask) >> (segment * 4);
            outputBuffer.data[globalPrefixSum[index] + offsets[index]] = inputNum;
            offsets[index]++;
        }
        groupMemoryBarrier();
        for (int i = startIdx; i < startIdx + segmentSize; i++)
        {
            inputBuffer.data[i] = outputBuffer.data[i];
        }
        groupMemoryBarrier();
    }
}