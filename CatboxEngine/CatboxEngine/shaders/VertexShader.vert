#version 440 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in ivec4 aBoneIndices;
layout (location = 5) in vec4 aBoneWeights;

out vec2 TexCoord;
out vec3 FragNormal;
out vec3 FragTangent;
out vec3 FragPos;
out vec4 FragPosLightSpace[8];  // Light space position for each light

uniform mat4 u_MVP;
uniform mat4 transform;

// Skeletal animation
uniform bool u_HasSkeleton;
uniform mat4 u_BoneMatrices[128];

// Light space matrices for shadow mapping (separate from Light struct)
uniform mat4 u_LightSpaceMatrices[8];
uniform int u_NumLights;

void main()
{
    vec3 localPos    = aPos;
    vec3 localNormal = aNormal;
    vec3 localTangent = aTangent;

    // Apply skeletal animation if bones are present
    if (u_HasSkeleton)
    {
        mat4 boneTransform = u_BoneMatrices[aBoneIndices[0]] * aBoneWeights[0]
                           + u_BoneMatrices[aBoneIndices[1]] * aBoneWeights[1]
                           + u_BoneMatrices[aBoneIndices[2]] * aBoneWeights[2]
                           + u_BoneMatrices[aBoneIndices[3]] * aBoneWeights[3];

        localPos    = (boneTransform * vec4(aPos, 1.0)).xyz;
        localNormal = (boneTransform * vec4(aNormal, 0.0)).xyz;
        localTangent = (boneTransform * vec4(aTangent, 0.0)).xyz;
    }

    // World position
    vec4 worldPos = transform * vec4(localPos, 1.0);
    FragPos = worldPos.xyz;

    TexCoord = aTexCoord;

    // Transform normal and tangent to world space
    mat3 normalMat = transpose(inverse(mat3(transform)));
    FragNormal = normalize(normalMat * localNormal);
    FragTangent = normalize(mat3(transform) * localTangent);

    // Calculate light space positions for shadow mapping
    for (int i = 0; i < u_NumLights && i < 8; ++i)
    {
        FragPosLightSpace[i] = u_LightSpaceMatrices[i] * worldPos;
    }

    gl_Position = u_MVP * worldPos;
}
