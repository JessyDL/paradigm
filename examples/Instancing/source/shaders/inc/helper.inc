
mat4x4 make_object_matrix(mat4x4 instance_data) {
	vec3 inst_position = vec3(instance_data[0][0], instance_data[0][1], instance_data[0][2]);
	vec3 inst_scale = vec3(instance_data[2][0], instance_data[2][1], instance_data[2][2]);

	// Construct rotation matrix from quaternion
	float x = instance_data[1][0];
	float y = instance_data[1][1];
	float z = instance_data[1][2];
	float w = instance_data[1][3];
	
	float x2 = x + x;
    float y2 = y + y;
    float z2 = z + z;
    float xx = x * x2;
    float xy = x * y2;
    float xz = x * z2;
    float yy = y * y2;
    float yz = y * z2;
    float zz = z * z2;
    float wx = w * x2;
    float wy = w * y2;
    float wz = w * z2;
    
    mat4 rotationMatrix = mat4(
        1.0 - (yy + zz), xy + wz, xz - wy, 0.0,
        xy - wz, 1.0 - (xx + zz), yz + wx, 0.0,
        xz + wy, yz - wx, 1.0 - (xx + yy), 0.0,
        0.0, 0.0, 0.0, 1.0
    );
	
	// Construct scale matrix
	mat4 scaleMatrix = mat4(
		inst_scale.x, 0.0, 0.0, 0.0,
		0.0, inst_scale.y, 0.0, 0.0,
		0.0, 0.0, inst_scale.z, 0.0,
		0.0, 0.0, 0.0, 1.0
	);
	
	// Construct translation matrix
	mat4 translationMatrix = mat4(
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		inst_position.x, inst_position.y, inst_position.z, 1.0
	);
	
	// Combine transformations: T * R * S
	return translationMatrix * rotationMatrix * scaleMatrix;
}
