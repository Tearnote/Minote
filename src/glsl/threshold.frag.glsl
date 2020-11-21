// Minote - glsl/threshold.frag.glsl
// Shifts the brightness of the image with a smooth function, ensuring all HDR
// pixels are within LDR range.

#version 330 core

in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D image;
uniform float threshold; ///< Lower limit of the input range
uniform float softKnee; ///< Percentage of the 0,threshold range to include
uniform float strength; ///< Final multiplier

// https://catlikecoding.com/unity/tutorials/advanced-rendering/bloom/
void main()
{
    vec4 fragColor = texture(image, fTexCoords);
    float brightness = max(fragColor.r, max(fragColor.g, fragColor.b));
    float knee = threshold * softKnee;
    float soft = brightness - threshold + knee;
    soft = clamp(soft, 0, 2 * knee);
    soft = soft * soft / (4 * knee + 0.00001);
    float contribution = max(soft, brightness - threshold);
    contribution /= max(brightness, 0.00001);
    outColor = fragColor * contribution * strength;
}
