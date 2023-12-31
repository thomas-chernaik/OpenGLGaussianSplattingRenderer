#version 430 core

in vec2 texCoord;

layout (binding = 0) uniform sampler2D tex;

out vec3 color;

void main()
{
    color = texture(tex, texCoord).rgb;
}