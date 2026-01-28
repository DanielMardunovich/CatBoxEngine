#version 440 core

// Maximum number of lights
#define MAX_LIGHTS 8

in vec2 TexCoord;
in vec3 FragNormal;
in vec3 FragTangent;
in vec3 FragPos;  // World position
out vec4 FragColor;

// Material properties
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

// Camera
uniform vec3 u_CameraPos;

// Light structure
struct Light {
    int type;              // 0=Directional, 1=Point, 2=Spot
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    
    // Attenuation (Point and Spot)
    float constant;
    float linear;
    float quadratic;
    
    // Spot light
    float innerCutoff;
    float outerCutoff;
    
    // Shadow properties
    bool castsShadows;
    sampler2D shadowMap;
    mat4 lightSpaceMatrix;
    float shadowBias;
    
    bool enabled;
};

// Lights
uniform int u_NumLights;
uniform Light u_Lights[MAX_LIGHTS];

// Shadow calculation with PCF
float CalculateShadow(Light light, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    if (!light.castsShadows)
        return 0.0;
    
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Outside shadow map frustum
    if (projCoords.z > 1.0)
        return 0.0;
    
    // Current depth
    float currentDepth = projCoords.z;
    
    // Calculate bias based on slope
    float bias = max(light.shadowBias * (1.0 - dot(normal, lightDir)), light.shadowBias * 0.1);
    
    // PCF (Percentage Closer Filtering)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(light.shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(light.shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

// Calculate lighting for one light
vec3 CalculateLight(Light light, vec3 normal, vec3 viewDir, vec3 albedo, vec3 specular, float shadow)
{
    if (!light.enabled)
        return vec3(0.0);
    
    vec3 lightDir;
    float attenuation = 1.0;
    
    // Calculate light direction and attenuation based on type
    if (light.type == 0) // Directional
    {
        lightDir = normalize(-light.direction);
    }
    else if (light.type == 1) // Point
    {
        vec3 lightVec = light.position - FragPos;
        float distance = length(lightVec);
        lightDir = normalize(lightVec);
        
        // Attenuation
        attenuation = 1.0 / (light.constant + light.linear * distance + 
                            light.quadratic * distance * distance);
    }
    else if (light.type == 2) // Spot
    {
        vec3 lightVec = light.position - FragPos;
        float distance = length(lightVec);
        lightDir = normalize(lightVec);
        
        // Attenuation
        attenuation = 1.0 / (light.constant + light.linear * distance + 
                            light.quadratic * distance * distance);
        
        // Spotlight cone
        float theta = dot(lightDir, normalize(-light.direction));
        float epsilon = light.innerCutoff - light.outerCutoff;
        float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
        attenuation *= intensity;
    }
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.color * light.intensity * diff * albedo * attenuation;
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), u_Shininess);
    vec3 specularContrib = light.color * light.intensity * spec * specular * attenuation;
    
    // Apply shadow
    diffuse *= (1.0 - shadow);
    specularContrib *= (1.0 - shadow);
    
    return diffuse + specularContrib;
}

void main()
{
    // Sample albedo/diffuse color
    vec3 albedo = u_DiffuseColor;
    if (u_HasDiffuseMap)
    {
        vec4 texColor = texture(u_DiffuseMap, TexCoord);
        albedo *= texColor.rgb;
    }
    
    // Sample specular
    vec3 specularColor = u_SpecularColor;
    if (u_HasSpecularMap)
    {
        float specularIntensity = texture(u_SpecularMap, TexCoord).r;
        specularColor *= specularIntensity;
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
    
    // View direction
    vec3 V = normalize(u_CameraPos - FragPos);
    
    // Ambient lighting (global illumination approximation)
    vec3 ambient = 0.1 * albedo;
    
    // Calculate all lights
    vec3 lighting = ambient;
    for (int i = 0; i < u_NumLights && i < MAX_LIGHTS; ++i)
    {
        // Calculate shadow (placeholder - need light space position)
        float shadow = 0.0;
        // TODO: Calculate shadow properly with light space matrix
        
        lighting += CalculateLight(u_Lights[i], N, V, albedo, specularColor, shadow);
    }
    
    // Final color
    FragColor = vec4(lighting, u_Alpha);
}
