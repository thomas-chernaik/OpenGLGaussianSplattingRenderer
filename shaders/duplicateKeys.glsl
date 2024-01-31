#version 430 core
layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

//buffer for the keys
layout(std430, binding = 0) buffer Keys {
    uvec2 data[];
} keys;

//buffer for the splat indices
layout(std430, binding = 1) buffer splatIndices {
uint data[];
} indices;

//uniform for the number of keys
layout (location = 0) uniform int numKeys;
//uniform for the maximum number of keys
layout (location = 1) uniform int maxNumKeys;


//atomic counter for the number of culled splats
layout(binding = 0, offset = 0) uniform atomic_uint numCulledSplats;
//atomic counter for the number of duplicated splats
layout(binding = 1, offset = 0) uniform atomic_uint numDuplicatedSplats;

void main() {
    uint allOnes = 0xFFFFFFFF;
    //get the index of the splat
    uint index = gl_GlobalInvocationID.x;
    //if the index is greater than the number of keys, return
    if(index >= numKeys) {
        return;
    }
    //get the key
    uvec2 key = keys.data[index];
    uint depth = key.y;
    uint keysToUnpack = key.x;
    //convert the keysToUnpack to an integer
    //check the splat isn't culled (keysToUnpack == FFFFFFFF)
    if(keysToUnpack == allOnes) {
        atomicCounterIncrement(numCulledSplats);
        return;
    }
    //write the first splat key out (in place)
    keys.data[index].x = keysToUnpack & 0xFFu;
    keys.data[index].y = depth | ((keysToUnpack & 0xFFu) << 20);
    indices.data[index] = index;
    //work out the number of keys to unpack
    uint numKeysToUnpack = 1;
    for(uint i=1; i<3; i++) {
        if((keysToUnpack & (0xFFu << i*8)) != 0) {
            numKeysToUnpack++;
        }
    }
    //loop through the keys to unpack
    //this gets skipped if there is only one key to unpack
    for(uint i=1; i<numKeysToUnpack; i++) {
        //get the key
        uint keyToUnpack = (keysToUnpack >> (i*8)) & 0xFFu;
        uint keyIndex = atomicCounterIncrement(numDuplicatedSplats) + numKeys;
        //write the key out
        keys.data[keyIndex] = uvec2(keyToUnpack, depth | keyToUnpack << 20);
        //write the index out
        indices.data[keyIndex] = index;
    }
}
