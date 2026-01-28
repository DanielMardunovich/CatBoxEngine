#version 440 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;

out vec2 TexCoord;
out vec3 FragNormal;
out vec3 FragTangent;
out vec3 FragPos;

uniform mat4 u_MVP;
uniform mat4 transform;

void main()
{
    // World position
    vec4 worldPos = transform * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    
    TexCoord = aTexCoord;
    
    // Transform normal and tangent to world space
    mat3 normalMat = transpose(inverse(mat3(transform)));
    FragNormal = normalize(normalMat * aNormal);
    FragTangent = normalize(mat3(transform) * aTangent);
    
    gl_Position = u_MVP * worldPos;
}
