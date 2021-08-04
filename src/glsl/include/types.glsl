struct IndirectCommand {
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	uint vertexOffset;
	uint firstInstance;
};

struct MeshDescriptor {
	uint indexOffset;
	uint indexCount;
	uint vertexOffset;
	float radius;
};

struct Transform {
	vec3 position;
	float pad0;
	vec3 scale;
	float pad1;
	vec4 rotation; // quat
};

struct RowTransform {
	vec4 rows[3];
};

struct Material {
	vec4 tint;
	float roughness;
	float metalness;
	vec2 pad0;
};

RowTransform encodeTransform(Transform t) {
	
	const float rw = t.rotation.x;
	const float rx = t.rotation.y;
	const float ry = t.rotation.z;
	const float rz = t.rotation.w;
	
	mat3 rotationMat = mat3(
		1.0 - 2.0 * (ry * ry + rz * rz),       2.0 * (rx * ry - rw * rz),       2.0 * (rx * rz + rw * ry),
		      2.0 * (rx * ry + rw * rz), 1.0 - 2.0 * (rx * rx + rz * rz),       2.0 * (ry * rz - rw * rx),
		      2.0 * (rx * rz - rw * ry),       2.0 * (ry * rz + rw * rx), 1.0 - 2.0 * (rx * rx + ry * ry));
	
	rotationMat[0] *= t.scale.x;
	rotationMat[1] *= t.scale.y;
	rotationMat[2] *= t.scale.z;
	
	RowTransform result;
	result.rows[0] = vec4(rotationMat[0], t.position.x);
	result.rows[1] = vec4(rotationMat[1], t.position.y);
	result.rows[2] = vec4(rotationMat[2], t.position.z);
	return result;
	
}

mat4 getTransform(RowTransform t) {
	
	return mat4(
		t.rows[0].x, t.rows[1].x, t.rows[2].x, 0.0,
		t.rows[0].y, t.rows[1].y, t.rows[2].y, 0.0,
		t.rows[0].z, t.rows[1].z, t.rows[2].z, 0.0,
		t.rows[0].w, t.rows[1].w, t.rows[2].w, 1.0);
	
}
