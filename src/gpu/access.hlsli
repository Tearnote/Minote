#ifndef ACCESS_HLSLI
#define ACCESS_HLSLI

float3 fetchVertex(StructuredBuffer<float> _buf, uint _index) {
	
	uint base = _index * 3;
	return float3(
		_buf[base + 0],
		_buf[base + 1],
		_buf[base + 2]
	);
	
}

float4x4 getTransform(StructuredBuffer<float4> _buf, uint _idx) {
	
	return transpose(float4x4(
		_buf[_idx * 3 + 0],
		_buf[_idx * 3 + 1],
		_buf[_idx * 3 + 2],
		float4(0.0, 0.0, 0.0, 1.0)
	));
	
}

#endif
