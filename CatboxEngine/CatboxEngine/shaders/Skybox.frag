#version 440 core
in  vec3 v_TexCoord;
in  vec2 v_UV;
out vec4 FragColor;

uniform sampler2D u_SkyTexture;

uniform bool u_UseProceduralSky;
uniform vec3 u_SkyColorTop;
uniform vec3 u_SkyColorHorizon;
uniform vec3 u_SkyColorBottom;

void main()
{
    if (u_UseProceduralSky)
    {
        // v_TexCoord is the raw cube-vertex position used as a direction.
        float t = clamp(normalize(v_TexCoord).y, -1.0, 1.0);
        vec3 color;
        if (t >= 0.0)
            color = mix(u_SkyColorHorizon, u_SkyColorTop, t);
        else
            color = mix(u_SkyColorHorizon, u_SkyColorBottom, -t);
        FragColor = vec4(color, 1.0);
    }
    else
    {
        FragColor = texture(u_SkyTexture, v_UV);
    }
}
