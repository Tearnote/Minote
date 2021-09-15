// Utilities for working with visibility buffer data

// Barycentrics interpolator. The below 3 functions are made available under
// the following terms. The full Apache 2.0 license is included with the source.
// The source has been modified to comply with project code style.
/*
Copyright (C) 2019 - DevSH Graphics Programming Sp. z O.O.
This software is provided 'as-is', under the Apache 2.0 license,
without any express or implied warranty.  In no event will the authors
be held liable for any damages arising from the use of this software.
*/
vec2 reconstructBarycentrics(vec3 _positionRelativeToV0, mat2x3 _edges) {
	
	float e0_2 = dot(_edges[0], _edges[0]);
	float e0e1 = dot(_edges[0], _edges[1]);
	float e1_2 = dot(_edges[1], _edges[1]);
	
	float qe0 = dot(_positionRelativeToV0, _edges[0]);
	float qe1 = dot(_positionRelativeToV0, _edges[1]);
	vec2 protoBary = vec2(qe0*e1_2 - qe1*e0e1, qe1*e0_2 - qe0*e0e1);
	
	float rcp_dep = 1.0 / (e0_2*e1_2 - e0e1*e0e1);
	return protoBary * rcp_dep;
	
}

vec2 reconstructBarycentrics(vec3 _pointPosition, mat3 _vertexPositions) {
	
	return reconstructBarycentrics(
		_pointPosition - _vertexPositions[2],
		mat2x3(_vertexPositions[0] - _vertexPositions[2], _vertexPositions[1] - _vertexPositions[2]));
	
}

vec3 expandBarycentrics(vec2 _compactBarycentrics) {
	
	return vec3(_compactBarycentrics.xy, 1.0 - _compactBarycentrics.x - _compactBarycentrics.y);
	
}
