// Minote - src/msdf.frag.glsl
// Multi-channel signed distance field rendering, used for sharp text at any
// zoom level. Performs its own antialiasing - do not use a MSAA target.

#version 330 core

in vec4 fColor;
in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D atlas; // Do not use mipmapping

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main()
{
    float pxRange = 4.0; //TODO make uniform

    vec3 s = texture(atlas, fTexCoords).rgb;
    float distance = (median(s.r, s.g, s.b) - 0.5) * pxRange;
    vec2 dxDist = vec2(dFdx(distance), dFdy(distance));
    if (all(equal(vec2(0.0),dxDist)))
        discard; // Avoid normalizing a zero vector
    vec2 gradDist = normalize(dxDist);

    vec2 uv = fTexCoords * textureSize(atlas, 0);
    vec2 Jdx = dFdx(uv);
    vec2 Jdy = dFdy(uv);

    vec2 grad = vec2(gradDist.x * Jdx.x + gradDist.y * Jdy.x,
                     gradDist.x * Jdx.y + gradDist.y * Jdy.y);
    float afwidth = 0.7071 * length(grad);
    float coverage = 1.0 - smoothstep(afwidth, -afwidth, distance);
    // This is how to do outline
    /*float borderThickness = 0.25;
    float highCoverage = 1.0 - smoothstep(afwidth, -afwidth, distance + borderThickness);
    float lowCoverage = smoothstep(afwidth, -afwidth, distance - borderThickness);
    float coverage = min(highCoverage, lowCoverage);*/

    outColor = vec4(fColor.rgb, mix(0.0, fColor.a, coverage));
}
