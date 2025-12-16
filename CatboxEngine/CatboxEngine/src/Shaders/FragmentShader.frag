#version 440 core

in vec4 vertexColor; // from vertex shader
out vec4 FragColor;

void main()
{
    FragColor = vertexColor;
}