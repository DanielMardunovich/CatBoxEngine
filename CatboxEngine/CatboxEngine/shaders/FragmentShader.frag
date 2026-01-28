#version 440 core
in vec2 TexCoord;
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

void main()
{
    vec3 base = u_DiffuseColor;
    if (u_HasDiffuseMap)
    {
        base *= texture(u_DiffuseMap, TexCoord).rgb;
    }
    // simple lighting: ambient + diffuse + specular using a single directional light
    vec3 N = normalize(vec3(0,0,1));
    if (u_HasNormalMap)
    {
        vec3 nmap = texture(u_NormalMap, TexCoord).rgb;
        nmap = nmap * 2.0 - 1.0;
        N = normalize(nmap);
    }
    vec3 L = normalize(vec3(0.5, 0.7, 1.0));
    vec3 V = normalize(vec3(0,0,1));
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = base * diff;
    vec3 specColor = u_SpecularColor;
    if (u_HasSpecularMap) specColor *= texture(u_SpecularMap, TexCoord).rgb;
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(R, V), 0.0), u_Shininess);
    vec3 specular = specColor * spec;

    vec3 color = 0.1 * base + diffuse + specular;
    FragColor = vec4(color, u_Alpha);
}
