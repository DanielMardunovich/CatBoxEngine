#version 440 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoord;

out vec3 v_TexCoord;
out vec2 v_UV;

uniform mat4 u_Projection;
uniform mat4 u_View;  // rotation-only (translation stripped by caller)

void main()
{
    v_TexCoord  = aPos;
    v_UV        = aTexCoord;
    vec4 pos    = u_Projection * u_View * vec4(aPos, 1.0);
    // Set z = w so that after perspective divide depth is always 1.0 (far plane),
    // ensuring the sky renders behind every scene object.
    gl_Position = pos.xyww;
}
