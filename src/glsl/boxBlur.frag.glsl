/**
 * Box blur fragment shader
 * @file
 * Basic fullscreen blit function. Generates its own vertices, just draw
 * 3 vertices with no buffers attached.
 */

#version 330 core

in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D image;
uniform float step;
uniform vec2 imageTexel;

void main()
{
    vec2 step1 = vec2(step) * imageTexel;
    vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
    color += texture(image, fTexCoords + step1) / 4.0;
    color += texture(image, fTexCoords - step1) / 4.0;
    vec2 step2 = step1;
    step2.x = -step2.x;
    color += texture(image, fTexCoords + step2) / 4.0;
    color += texture(image, fTexCoords - step2) / 4.0;
    outColor = color;
}
