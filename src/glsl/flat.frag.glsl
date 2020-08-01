/**
 * Flat shading fragment shader
 * @file
 * No lighting, just constant color + per-instance tint.
 */

#version 330 core

in vec4 fColor;
in vec4 fHighlight;
noperspective in vec3 fDist;
flat in ivec3 fDir;

out vec4 outColor;
out vec4 outGeometry;

void main()
{
    // get smallest distance
    float min_dist = fDist.x;
    int   min_horz = fDir.x;

    if (abs(fDist.y) < abs(min_dist)){
        min_dist = fDist.y;
        min_horz = fDir.y;
    }
    if (abs(fDist.z) < abs(min_dist)){
        min_dist = fDist.z;
        min_horz = fDir.z;
    }

    // get sample offset
    vec2 offset = (min_horz == 0) ? vec2(min_dist, 0.5) : vec2(0.5, min_dist);
    outGeometry = vec4(offset, 0, 1);

    outColor = vec4(mix(fColor.rgb, fHighlight.rgb, fHighlight.a), fColor.a);
}
