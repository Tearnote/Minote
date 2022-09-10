#include "access.hlsli"
#include "types.hlsli"
#include "util.hlsli"

[[vk::binding(0)]] StructuredBuffer<Meshlet> b_meshlets;
[[vk::binding(1)]] StructuredBuffer<float4> b_transforms;
[[vk::binding(2)]] StructuredBuffer<uint4> b_instanceCount;
[[vk::binding(3)]] StructuredBuffer<Instance> b_instances;
[[vk::binding(4)]] RWStructuredBuffer<uint4> b_outInstanceCount;
[[vk::binding(5)]] RWStructuredBuffer<Instance> b_outInstances;
[[vk::binding(6)]][[vk::combinedImageSampler]] Texture2D<float> s_hiz;
[[vk::binding(6)]][[vk::combinedImageSampler]] SamplerState s_hizSmp;

[[vk::constant_id(0)]] const uint UseHiZ = 0; // 1 - occlusion culling enabled
[[vk::constant_id(1)]] const float ZNear = 0.0;
[[vk::constant_id(2)]] const uint HiZWidth = 0;
[[vk::constant_id(3)]] const uint HiZHeight = 0;
static const uint2 HiZSize = {HiZWidth, HiZHeight};
[[vk::constant_id(4)]] const uint HiZInnerWidth = 0;
[[vk::constant_id(5)]] const uint HiZInnerHeight = 0;
static const uint2 HiZInnerSize = {HiZInnerWidth, HiZInnerHeight};

struct Constants {
	float4x4 view;
	float4 frustum;
	float P00;
	float P11;
};

[[vk::push_constant]] Constants C;

// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. Michael Mara, Morgan McGuire. 2013
bool projectSphere(float3 C, float r, float znear, float P00, float P11,
	out float4 aabb) {
	
	if (C.z < r + znear)
		return false;
	
	float2 cx = -C.xz;
	float2 vx = float2(sqrt(dot(cx, cx) - r * r), r);
	float2 minx = mul(float2x2(vx.x, vx.y, -vx.y, vx.x), cx);
	float2 maxx = mul(float2x2(vx.x, -vx.y, vx.y, vx.x), cx);
	
	float2 cy = -C.yz;
	float2 vy = float2(sqrt(dot(cy, cy) - r * r), r);
	float2 miny = mul(float2x2(vy.x, vy.y, -vy.y, vy.x), cy);
	float2 maxy = mul(float2x2(vy.x, -vy.y, vy.y, vy.x), cy);
	
	aabb = float4(minx.x / minx.y * P00, miny.x / miny.y * P11, maxx.x / maxx.y * P00, maxy.x / maxy.y * P11);
	aabb = aabb.xwzy * float4(0.5f, -0.5f, 0.5f, -0.5f) + float4(0.5f, 0.5f, 0.5f, 0.5f); // clip space -> uv space
	
	return true;
	
}

[numthreads(64, 1, 1)]
void main(uint3 _tid: SV_DispatchThreadID) {
	
	if (_tid.x >= b_instanceCount[0].w)
		return;
	
	// Retrieve meshlet data
	Instance instance = b_instances[_tid.x];
	Meshlet meshlet = b_meshlets[instance.meshletIdx];
	uint transformIdx = instance.objectIdx;
	float4x4 transform = getTransform(b_transforms, transformIdx);
	
	// Transform meshlet data
	float scale = max3( //FIXME transpose??
		length(transform[0].xyz),
		length(transform[1].xyz),
		length(transform[2].xyz));
	float3 boundingSphereCenter = mul(float4(meshlet.boundingSphereCenter, 1.0), transform).xyz;
	float boundingSphereRadius = meshlet.boundingSphereRadius * scale;
	
	// Frustum culling
	float3 viewCenter = mul(float4(boundingSphereCenter, 1.0), C.view).xyz;
	if ((viewCenter.z * C.frustum[1] - abs(viewCenter.x) * C.frustum[0] <= -boundingSphereRadius) ||
	    (viewCenter.z * C.frustum[3] - abs(viewCenter.y) * C.frustum[2] <= -boundingSphereRadius))
		return;
	
	// Occlusion culling
	if (UseHiZ) {
		viewCenter.y *= -1;
		float4 aabb;
		if (projectSphere(viewCenter, boundingSphereRadius, ZNear, C.P00, C.P11, aabb)) {
			float width = (aabb.z - aabb.x) * HiZInnerSize.x;
			float height = (aabb.w - aabb.y) * HiZInnerSize.y;
		
			float level = floor(log2(max(width, height)));
			float2 offset = float2(HiZSize - HiZInnerSize) / 2.0 / float2(HiZSize);
			float2 scale = float2(HiZInnerSize) / float2(HiZSize);
		
			// Sampler is set up to do min reduction, so this computes the minimum depth of a 2x2 texel quad
			float2 depthUv = saturate((aabb.xy + aabb.zw) * 0.5) * scale + offset;
			float depth = s_hiz.SampleLevel(s_hizSmp, depthUv, level);
			float depthSphere = ZNear / (viewCenter.z - boundingSphereRadius);
		
			if (depthSphere <= depth)
				return;
		}
	}
	
	// Meshlet survived - write out
	uint writeIdx;
	InterlockedAdd(b_outInstanceCount[0].w, 1, writeIdx);
	b_outInstances[writeIdx] = instance;
	if (writeIdx % 64 == 0)
		InterlockedAdd(b_outInstanceCount[0].x, 1);
	
}
