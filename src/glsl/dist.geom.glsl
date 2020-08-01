#version 330
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec4 gColor[];
in vec4 gHighlight[];

out vec4 fColor;
out vec4 fHighlight;
noperspective out vec3 fDist;
flat out ivec3 fDir;

float ComputeDist(vec2 pos0, vec2 pos1, vec2 pos2, inout int major_dir)
{
    vec2 dir = normalize(pos1 - pos0);
    vec2 normal = vec2(-dir.y, dir.x);
    float dist = dot(pos0, normal) - dot(pos2, normal);

    // Check major direction
    if(abs(normal.x) > abs(normal.y)){
        major_dir = 0;
        return dist / normal.x;
    } else {
        major_dir = 1;
        return dist / normal.y;
    }
}

void main()
{
    vec2 wh = vec2(1280.0, 720.0);

    // triangle in ndc (before persp div)
    vec4 v0_in = gl_in[0].gl_Position;
    vec4 v1_in = gl_in[1].gl_Position;
    vec4 v2_in = gl_in[2].gl_Position;

    // triangle in screen space
    vec2 v0 = (v0_in.xy/v0_in.w * 0.5 + 0.5) * wh;
    vec2 v1 = (v1_in.xy/v1_in.w * 0.5 + 0.5) * wh;
    vec2 v2 = (v2_in.xy/v2_in.w * 0.5 + 0.5) * wh;

    // normal distances to opposite triangle side in screen space
    float v0_dist = ComputeDist(v1, v2, v0, fDir.x);
    float v1_dist = ComputeDist(v2, v0, v1, fDir.y);
    float v2_dist = ComputeDist(v0, v1, v2, fDir.z);

    gl_Position = v0_in;
    fDist = vec3(v0_dist, 0, 0);
    fColor = gColor[0];
    fHighlight = gHighlight[0];
    EmitVertex();

    gl_Position = v1_in;
    fDist = vec3(0, v1_dist, 0);
    fColor = gColor[1];
    fHighlight = gHighlight[1];
    EmitVertex();

    gl_Position = v2_in;
    fDist = vec3(0, 0, v2_dist);
    fColor = gColor[2];
    fHighlight = gHighlight[2];
    EmitVertex();

    EndPrimitive();
}
