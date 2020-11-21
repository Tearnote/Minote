// Minote - glsl/flat.frag.glsl
// Draw vertices with no lighting, just constant color + per-instance tint.

#version 330 core

in vec4 fColor;
in vec4 fHighlight;

out vec4 outColor;

void main()
{
    outColor = vec4(mix(fColor.rgb, fHighlight.rgb, fHighlight.a), fColor.a);
}
