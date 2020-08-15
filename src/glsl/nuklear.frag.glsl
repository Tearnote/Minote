/**
 * Nuklear fragment shader
 * @file
 * Renders the output of Nuklear's vertex buffer generator.
 */

#version 330

in vec2 fTexCoord;
in vec4 fColor;

out vec4 outColor;

uniform sampler2D atlas;

void main()
{
    outColor = fColor * texture(atlas, fTexCoord);
}
