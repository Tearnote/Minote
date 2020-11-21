// Minote - glsl/phong.frag.glsl
// Basic Phong-Blinn lighting model with one light source and per-instance tint.
// Fragment stage inputs are transformed to view space.

#version 330 core

in vec3 fPosition;
in vec4 fColor;
in vec4 fHighlight;
in vec3 fNormal;
in vec3 fLightPosition;

out vec4 outColor;

uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform float ambient;
uniform float diffuse;
uniform float specular;
uniform float shine;

void main()
{
    vec3 normal = normalize(fNormal);
    vec3 lightDirection = normalize(fLightPosition - fPosition);
    vec3 viewDirection = normalize(-fPosition);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);

    vec3 outAmbient = ambient * ambientColor * lightColor;
    vec3 outDiffuse = diffuse * max(dot(normal, lightDirection), 0.0) * lightColor;
    vec3 outSpecular = specular * pow(max(dot(normal, halfwayDirection), 0.0), shine) * lightColor;

    outColor = vec4(outAmbient + outDiffuse + outSpecular, 1.0) * fColor;
    outColor = vec4(mix(outColor.rgb, fHighlight.rgb, fHighlight.a), outColor.a);
}
