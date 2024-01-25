#version 430 core

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;
//the buffer of values to be prefixed
layout(std430, binding = 1) buffer BinsBuffer
{
    uint data[];
} binsBuffer;

//shared memory to load our data into for faster access, and to allow for in-place prefixing
shared uint sharedData[256];

void main()
{
    uint index = gl_GlobalInvocationID.x;
    //load this thread's data into local shared memory
    uint count = binsBuffer.data[index];

    //wait for all threads to finish loading
    barrier();
    /*
    //O(n) prefixing
    //255/2 avg shared writes per thread
    //prefix the data in shared memory
    for(uint i = index; i<256; i++)
    {
        //add this value to all later values
        sharedData[i] += count;
    }
    */
    //O(log(n)) prefixing
    //8 shared writes per thread
    //this works by writing to the next term, then waiting, then writing to 2 terms ahead, then 4, then 8, then 16, then 32, then 64, then 128, then 256
    //this means that we get all the previous values added to our current value
    sharedData[index] = count;
    barrier();
    for(uint i = 0; i<8; i++)
    {
        //1, 2, 4, 8, 16, 32, 64, 128, 256
        uint offset = 1 << i;
        if(index+offset < 256)
        {
            sharedData[index + offset] += sharedData[index];
        }
        //wait for all threads to finish writing
        barrier();
    }


    //wait for all threads to finish prefixing
    barrier();

    //write the data back to global memory
    binsBuffer.data[index] = sharedData[index];
}