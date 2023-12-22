#version 430 core
layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

//the members here aren't called data because it gave me an error I have no clue why?
//inputs
//means3D
layout(std430, binding = 0) buffer means3D
{
    vec3 means[];
};
//opacities
layout(std430, binding = 1) buffer opacities
{
    float opacities[];
};
//3D covariance matrices
layout(std430, binding = 2) buffer covariances3D
{
    float covariances[];
};

//VP matrix uniform
layout (std140) uniform vpMatrix
{
    mat4 vp;
};

//output
//tiles touched and depth as a vec2
layout(std430, binding = 3) buffer keys
{
    vec2 keys[];
};

//conic opacities
layout(std430, binding = 4) buffer conicOpacities
{
    vec4 conics[];
};

//projected means
layout(std430, binding = 5) buffer projectedMeans
{
    vec2 projectedMeans[];
};

void main() {

}
