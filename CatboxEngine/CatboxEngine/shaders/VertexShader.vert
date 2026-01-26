#version 440 core
layout (location = 0) in vec3 aPos;

uniform mat4 u_MVP;

uniform mat4 transform;

void main()
{
    gl_Position = u_MVP * transform * vec4(aPos, 1.0);
}
