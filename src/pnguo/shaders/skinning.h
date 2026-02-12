// gltf变形动画和骨骼动画
#ifdef ID_MORPHING_DATA
// 变形插值数据
layout(set = 0, binding = ID_MORPHING_DATA) buffer per_morphing_mw
{
	float u_morphWeights[];
};
// 静态目标数据：每个顶点有插值数据数量的vec3
layout(set = 0, binding = ID_TARGET_DATA) buffer per_morphing
{
	int vertex_count;
	int weight_count;
	int position_offset;
	int normal_offset;
	int tangent_offset;
	int texcoord0_offset;
	int color0_offset;
	int texcoord1_offset;
	vec4 per_target_data[];
};
#endif

#ifdef ID_SKINNING_MATRICES
layout(set = 0, binding = ID_SKINNING_MATRICES) buffer perSkeleton
{
	mat4 u_ModelMatrix[];
};

mat4 GetSkinningMatrix(vec4 Weights, uvec4 Joints)
{
	mat4 skinningMatrix =
		Weights.x * u_ModelMatrix[Joints.x] +
		Weights.y * u_ModelMatrix[Joints.y] +
		Weights.z * u_ModelMatrix[Joints.z] +
		Weights.w * u_ModelMatrix[Joints.w];
	return skinningMatrix;
}
#endif

#ifdef ID_MORPHING_DATA 

// 获取目标数据
vec4 getDisplacement(int vertexID, int targetIndex)
{
	return per_target_data[vertexID + targetIndex];
}


vec4 getTargetPosition(int vertexID)
{
	vec4 pos = vec4(0);
#ifdef HAS_MORPH_TARGET_POSITION
	for (int i = 0; i < weight_count; i++)
	{
		vec4 displacement = getDisplacement(vertexID, position_offset + i * vertex_count);
		pos += u_morphWeights[i] * displacement;
	}
#endif 
	return pos;
}

vec3 getTargetNormal(int vertexID)
{
	vec3 normal = vec3(0);

#ifdef HAS_MORPH_TARGET_NORMAL
	for (int i = 0; i < weight_count; i++)
	{
		vec3 displacement = getDisplacement(vertexID, normal_offset + i * vertex_count).xyz;
		normal += u_morphWeights[i] * displacement;
	}
#endif

	return normal;
}


vec3 getTargetTangent(int vertexID)
{
	vec3 tangent = vec3(0);

#ifdef HAS_MORPH_TARGET_TANGENT
	for (int i = 0; i < weight_count; i++)
	{
		vec3 displacement = getDisplacement(vertexID, tangent_offset + i * vertex_count).xyz;
		tangent += u_morphWeights[i] * displacement;
	}
#endif

	return tangent;
}

vec2 getTargetTexCoord0(int vertexID)
{
	vec2 uv = vec2(0);

#ifdef HAS_MORPH_TARGET_TEXCOORD_0
	for (int i = 0; i < weight_count; i++)
	{
		vec2 displacement = getDisplacement(vertexID, texcoord0_offset + i * vertex_count).xy;
		uv += u_morphWeights[i] * displacement;
	}
#endif

	return uv;
}

vec2 getTargetTexCoord1(int vertexID)
{
	vec2 uv = vec2(0);

#ifdef HAS_MORPH_TARGET_TEXCOORD_1
	for (int i = 0; i < weight_count; i++)
	{
		vec2 displacement = getDisplacement(vertexID, texcoord1_offset + i * vertex_count).zw;
		uv += u_morphWeights[i] * displacement;
	}
#endif

	return uv;
}

vec4 getTargetColor0(int vertexID)
{
	vec4 color = vec4(0);

#ifdef HAS_MORPH_TARGET_COLOR_0
	for (int i = 0; i < weight_count; i++)
	{
		vec4 displacement = getDisplacement(vertexID, color0_offset + i * vertex_count);
		color += u_morphWeights[i] * displacement;
	}
#endif

	return color;
}

#endif

