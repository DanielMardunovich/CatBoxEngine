#version 440 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 u_MVP;
uniform mat4 transform;

void main()
{
    TexCoord = aTexCoord;
    gl_Position = u_MVP * transform * vec4(aPos, 1.0);
}
