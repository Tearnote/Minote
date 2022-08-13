#include "instanceList/index.hlsli"
#include "access.hlsli"
#include "types.hlsli"

struct Constants {
	float4x4 viewProjection;
};

struct VSOutput {
	float4 position: SV_Position;
};

[[vk::binding(0)]] StructuredBuffer<uint> b_vertIndices;
[[vk::binding(1)]] StructuredBuffer<float> b_vertices;
[[vk::binding(2)]] StructuredBuffer<Meshlet> b_meshlets;
[[vk::binding(3)]] StructuredBuffer<float4> b_transforms;
[[vk::binding(4)]] StructuredBuffer<Instance> b_instances;

[[vk::push_constant]] Constants c_push;

VSOutput main(uint _vid: SV_VertexID) {
	
	VSOutput output;
	
	// Retrieve meshlet
	uint index;
	uint instanceIdx;
	unpackIndex(_vid, index, instanceIdx);
	Instance instance = b_instances[instanceIdx];
	Meshlet meshlet = b_meshlets[instance.meshletIdx];
	
	// Retrieve vertex
	uint vertIndex = b_vertIndices[index + meshlet.vertexOffset];
	float3 vertex = fetchVertex(b_vertices, vertIndex);
	
	// Transform and output
	float4x4 transform = getTransform(b_transforms, instance.objectIdx);
	output.position = float4(vertex, 1.0);
	output.position = mul(output.position, transform);
	output.position = mul(output.position, c_push.viewProjection);
	
	return output;
	
}
