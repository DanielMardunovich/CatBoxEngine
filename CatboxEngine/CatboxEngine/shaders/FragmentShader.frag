#version 440 core
in vec2 TexCoord;
out vec4 FragColor;

uniform vec3 u_DiffuseColor;
uniform sampler2D u_DiffuseMap;
uniform bool u_HasDiffuseMap;

void main()
{
    vec3 base = u_DiffuseColor;
    if (u_HasDiffuseMap)
    {
        base *= texture(u_DiffuseMap, TexCoord).rgb;
    }
    FragColor = vec4(base, 1.0);
}
