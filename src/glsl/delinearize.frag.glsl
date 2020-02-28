/**
 * Delinearize fragment shader
 * @file
 * Blits while converting from linear to sRGB. Generates its own vertices,
 * just draw 3 vertices with no buffers attached.
 */

#version 330 core

in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D image;
uniform float gamma;

void main()
{
    outColor = texture(image, fTexCoords);
    outColor.rgb = pow(outColor.rgb, vec3(1.0 / gamma));
}
