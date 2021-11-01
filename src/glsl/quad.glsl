#ifndef QUAD_GLSL
#define QUAD_GLSL

#define SUBSAMPLE_COUNT 8
#define VIS_SAMPLE_COUNT (4 * SUBSAMPLE_COUNT)

const vec2 QuadSubsampleLocations[SUBSAMPLE_COUNT] = {
	{0.5625, 0.3125},
	{0.4375, 0.6875},
	{0.8125, 0.5625},
	{0.3125, 0.1875},
	{0.1875, 0.8125},
	{0.0625, 0.4375},
	{0.6875, 0.9375},
	{0.9375, 0.0625}};

const vec2 QuadPixelOffsets[4] = {
	{0.0, 0.0},
	{1.0, 0.0},
	{0.0, 1.0},
	{1.0, 1.0}};

vec2 subsamplePosAverage(uint _subsamples) {
	
	vec2 sum = vec2(0.0);
	for (uint i = 0; i < 32; i += 1) {
		
		if ((_subsamples & (1u << i)) != 0)
			sum += QuadPixelOffsets[i / 8] + QuadSubsampleLocations[i % 8];
	}
	
	return sum / float(bitCount(_subsamples));
	
}

#endif //QUAD_GLSL
