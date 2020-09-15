/**
 * MSDF fragment shader
 * @file
 * Multi-channel signed distance field drawing, used for sharp text at any
 * zoom level.
 */

#version 330 core

in vec4 fColor;
in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D atlas;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main()
{
    /*float pxRange = 4.0;

    vec2 msdfUnit = pxRange / vec2(textureSize(atlas, 0));
    vec3 s = texture(atlas, fTexCoords).rgb;
    float sigDist = median(s.r, s.g, s.b) - 0.5;
    sigDist *= dot(msdfUnit, 0.5 / fwidth(fTexCoords));
    float opacity = clamp(sigDist + 0.5, 0.0, 1.0);

    outColor = vec4(fColor.rgb, smoothstep(0.0, fColor.a, opacity));*/

    float pxRange = 4.0;

    vec3 s = texture(atlas, fTexCoords).rgb;
    float distance = (median(s.r, s.g, s.b) - 0.5) * pxRange;
    vec2 uv = fTexCoords * textureSize(atlas, 0);
    vec2 grad_dist = normalize(vec2(dFdx(distance), dFdy(distance)));
    vec2 Jdx = dFdx(uv);
    vec2 Jdy = dFdy(uv);
    vec2 grad = vec2(grad_dist.x * Jdx.x + grad_dist.y*Jdy.x, grad_dist.x * Jdx.y + grad_dist.y*Jdy.y);
    float afwidth = 0.7071*length(grad);
    float coverage = 1.0 - smoothstep(afwidth, -afwidth, distance);

    outColor = vec4(fColor.rgb, smoothstep(0.0, fColor.a, coverage));
}
