#version 440 core
in vec2 TexCoord;
in vec3 FragNormal;
in vec3 FragTangent;
in vec3 FragPos;  // World position
out vec4 FragColor;

uniform vec3 u_DiffuseColor;
uniform sampler2D u_DiffuseMap;
uniform bool u_HasDiffuseMap;

uniform vec3 u_SpecularColor;
uniform sampler2D u_SpecularMap;
uniform bool u_HasSpecularMap;

uniform float u_Shininess;
uniform float u_Alpha;

uniform sampler2D u_NormalMap;
uniform bool u_HasNormalMap;

uniform vec3 u_CameraPos;  // Camera position for specular
uniform vec3 u_LightDir;   // Light direction

void main()
{
    // Sample diffuse/albedo color
    vec3 albedo = u_DiffuseColor;
    if (u_HasDiffuseMap)
    {
        vec4 texColor = texture(u_DiffuseMap, TexCoord);
        albedo *= texColor.rgb;
    }
    
    // Normal mapping
    vec3 N = normalize(FragNormal);
    if (u_HasNormalMap)
    {
        vec3 T = normalize(FragTangent);
        vec3 B = normalize(cross(N, T));
        mat3 TBN = mat3(T, B, N);
        
        vec3 normalMap = texture(u_NormalMap, TexCoord).rgb;
        normalMap = normalMap * 2.0 - 1.0;  // [0,1] -> [-1,1]
        N = normalize(TBN * normalMap);
    }
    
    // Lighting vectors
    vec3 L = normalize(-u_LightDir);  // Light direction (toward light)
    vec3 V = normalize(u_CameraPos - FragPos);  // View direction
    vec3 H = normalize(L + V);  // Half vector
    
    // Ambient
    vec3 ambient = 0.1 * albedo;
    
    // Diffuse
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = albedo * NdotL;
    
    // Specular
    vec3 specularColor = u_SpecularColor;
    if (u_HasSpecularMap)
    {
        // Use specular map to modulate specular intensity
        float specularIntensity = texture(u_SpecularMap, TexCoord).r;
        specularColor *= specularIntensity;
    }
    
    float NdotH = max(dot(N, H), 0.0);
    float specularPower = pow(NdotH, u_Shininess);
    vec3 specular = specularColor * specularPower;
    
    // Only add specular if surface faces light
    if (NdotL <= 0.0)
        specular = vec3(0.0);
    
    // Final color
    vec3 finalColor = ambient + diffuse + specular;
    FragColor = vec4(finalColor, u_Alpha);
}
