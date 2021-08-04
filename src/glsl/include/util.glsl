// https://rauwendaal.net/2014/06/14/rendering-a-screen-covering-triangle-in-opengl/
vec2 triangleVertex(int vertexID, out vec2 texCoords) {
    float x = -1.0 + float((vertexID & 1) << 2);
    float y = -1.0 + float((vertexID & 2) << 1);
    texCoords.x = (x+1.0)*0.5;
    texCoords.y = (y+1.0)*0.5;
    return vec2(x, y);
}

// https://twitter.com/donzanoid/status/616370134278606848
// Uses a triangle strip
vec3 cubeVertex(int vertexID) {
    int b = 1 << vertexID;
    return vec3((0x287a & b) != 0, (0x02af & b) != 0, (0x31e3 & b) != 0);
}

// http://www.java-gaming.org/topics/fast-srgb-conversion-glsl-snippet/37583/view.html
vec3 srgbEncode(vec3 color) {
   float r = color.r < 0.0031308 ? 12.92 * color.r : 1.055 * pow(color.r, 1.0/2.4) - 0.055;
   float g = color.g < 0.0031308 ? 12.92 * color.g : 1.055 * pow(color.g, 1.0/2.4) - 0.055;
   float b = color.b < 0.0031308 ? 12.92 * color.b : 1.055 * pow(color.b, 1.0/2.4) - 0.055;
   return vec3(r, g, b);
}

vec3 srgbDecode(vec3 color) {
   float r = color.r < 0.04045 ? (1.0 / 12.92) * color.r : pow((color.r + 0.055) * (1.0 / 1.055), 2.4);
   float g = color.g < 0.04045 ? (1.0 / 12.92) * color.g : pow((color.g + 0.055) * (1.0 / 1.055), 2.4);
   float b = color.b < 0.04045 ? (1.0 / 12.92) * color.b : pow((color.b + 0.055) * (1.0 / 1.055), 2.4);
   return vec3(r, g, b);
}

float Luminance(vec3 color) {
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    return dot(color, W);
}

// https://github.com/hughsk/glsl-luma/blob/master/index.glsl
float luma(vec3 color) {
    const vec3 W = vec3(0.299, 0.587, 0.114);
    return dot(color, W);
}
