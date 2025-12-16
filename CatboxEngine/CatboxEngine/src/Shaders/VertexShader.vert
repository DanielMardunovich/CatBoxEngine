#version 440 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor; // read per-vertex color from Cube.vbo
layout(location = 2) in vec2 aTexCoords; // read per-vertex texture coordinates

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 vertexColor;
out vec2 TexCoords; // output texture coordinates to fragment shader

void main()
{
    // Transform vertex position by model, view and projection matrices
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = vec4(aColor, 1.0); // use vertex color provided by Cube's VBO
    TexCoords = aTexCoords; // pass texture coordinates to fragment shader
}