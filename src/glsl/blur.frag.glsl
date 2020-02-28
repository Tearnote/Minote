/**
 * Blur fragment shader
 * @file
 * A box blur pass. Execute multiple times with an output texture 2x smaller
 * than the input in both dimensions.
 */

#version 330 core

in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D image;

// https://catlikecoding.com/unity/tutorials/advanced-rendering/bloom/
void main()
{
    vec4 fragColor = texture(image, fTexCoords);
    outColor = fragColor;
}
