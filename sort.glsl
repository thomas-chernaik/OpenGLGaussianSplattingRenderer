// computeShader.glsl
#version 430 core
layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;
layout (binding = 0)  buffer InputBuffer {
    int data[];
} inputBuffer;

//the array to store the prefix sum for the work group
shared int sharedHistogram[256*16];

//the output buffer
layout (binding = 1) buffer OutputBuffer {
    int data[];
} outputBuffer;

void main()
{
    int startIdx = int(gl_LocalInvocationID.x) * 16;
    //initialise the shared memory
    for (int i = startIdx; i < startIdx + 16; i++) {
        sharedHistogram[i] = 0;
    }
    int mask = 0x0000000F;
    int histogram[16];
    for (int i = 0; i < 16; i++) {
        histogram[i] = 0;
    }

    for (int i = startIdx; i < startIdx + 16; i++) {
        int index = int(inputBuffer.data[i]) & mask;
        histogram[index]++;
    }
    //write the histogram to the shared memory
    for (int i = startIdx; i < startIdx + 16; i++) {
        sharedHistogram[i] = histogram[i & mask];
    }

    //wait for all threads to finish writing to the shared memory
    groupMemoryBarrier();

    int offsets[16];
    int globalHistogram[16];
    int globalPrefixSum[16];
    for (int i = 0; i < 16; i++) {
        offsets[i] = 0;
        globalHistogram[i] = 0;
    }

    //compute the global histogram and the offsets
    for (int i = 0; i < 256 * 16; i += 16)
    {
        for (int j = 0; j < 16; j++)
        {
            if (int(i / 16) == gl_LocalInvocationID.x)
            {
                offsets[j] = globalHistogram[j];
            }
            globalHistogram[j] += sharedHistogram[i + j];
        }
    }
    //compute the global prefix sum
    globalPrefixSum[0] = 0;
    for (int i = 1; i < 16; i++)
    {
        globalPrefixSum[i] = globalHistogram[i - 1] + globalPrefixSum[i - 1];
    }
    //write the output
    for (int i = startIdx; i < startIdx + 16; i++)
    {
        int index = int(inputBuffer.data[i]) & mask;
        outputBuffer.data[globalPrefixSum[index] + offsets[index]] = inputBuffer.data[i];
        offsets[index]++;
    }

}