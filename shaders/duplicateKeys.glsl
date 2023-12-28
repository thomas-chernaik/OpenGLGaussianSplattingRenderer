#version 430 core
layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

//buffer for the keys
layout(std430, binding = 0) buffer Keys {
    vec2 data[];
} keys;

//buffer for the splat indices
layout(std430, binding = 1) buffer splatIndices {
uint data[];
} indices;

//uniform for the number of keys
layout (location = 0) uniform uint numKeys;
//uniform for the maximum number of keys
layout (location = 1) uniform uint maxNumKeys;

const float allOnes = 0xFFFFFFFF;

//atomic counter for the number of culled splats
layout(binding = 0, offset = 0) uniform atomic_uint numCulledSplats;
//atomic counter for the number of duplicated splats
layout(binding = 1, offset = 0) uniform atomic_uint numDuplicatedSplats;

void main() {
    atomicCounterIncrement(numCulledSplats);
    //get the index of the splat
    uint index = gl_GlobalInvocationID.x;
    //if the index is greater than the number of keys, return
    if(index >= numKeys) {
        return;
    }
    //get the key
    vec2 key = keys.data[index];
    float depth = key.y;
    uint keysToUnpack = floatBitsToUint(key.x);
    //convert the keysToUnpack to an integer
    //check the splat isn't culled (keysToUnpack == FFFFFFFF)
    if(keysToUnpack == allOnes) {
        atomicCounterIncrement(numCulledSplats);
        return;
    }
    //write the first splat key out (in place)
    float firstKey = uintBitsToFloat(keysToUnpack & 0xF);
    keys.data[index].x = firstKey;
    //work out the number of keys to unpack
    uint numKeysToUnpack = 1;
    for(uint i=1; i<7; i++) {
        if((keysToUnpack & (0xF << i)) != 0) {
            numKeysToUnpack++;
        }
    }
    //loop through the keys to unpack
    //this gets skipped if there is only one key to unpack
    for(uint i=1; i<numKeysToUnpack; i++) {
        //get the key
        float keyToUnpack = uintBitsToFloat((keysToUnpack >> (i*4)) & 0xF);
        uint keyIndex = atomicCounterIncrement(numDuplicatedSplats) + numKeys;
        //write the key out
        keys.data[keyIndex] = vec2(keyToUnpack, depth);
        //write the index out
        indices.data[keyIndex] = index;
    }
}
