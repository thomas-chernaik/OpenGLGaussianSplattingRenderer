#version 430 core

layout( location=0 ) in vec3 position;

out vec2 texCoord;
void main()
{
    gl_Position = vec4( position, 1.0 );
    texCoord = (position.xy + vec2(0.5)) / vec2(2.0);
}
