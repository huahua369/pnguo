// AMD Cauldron code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifdef ID_MORPHING_DATA
// 插值数据
layout(set = 0, binding = ID_MORPHING_DATA) buffer per_morphing_mw
{
	float u_morphWeights[];
};
// 目标数据：每个顶点有插值数据数量的vec3
layout(set = 0, binding = ID_TARGET_DATA) buffer per_morphing
{
	vec4 per_target_data[];
};
#endif

#ifdef ID_SKINNING_MATRICES

struct Matrix2
{
	mat4 m_current;
	mat4 m_previous;
};

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
#ifdef MORPH_TARGET_POSITION_OFFSET 
	for (int i = 0; i < WEIGHT_COUNT; i++)
	{
		vec4 displacement = getDisplacement(vertexID, MORPH_TARGET_POSITION_OFFSET + i * vertex_count);
		pos += u_morphWeights[i] * displacement;
	}
#endif

	return pos;
}

vec3 getTargetNormal(int vertexID)
{
	vec3 normal = vec3(0);

#ifdef MORPH_TARGET_NORMAL_OFFSET 
	for (int i = 0; i < WEIGHT_COUNT; i++)
	{
		vec3 displacement = getDisplacement(vertexID, MORPH_TARGET_NORMAL_OFFSET + i * vertex_count).xyz;
		normal += u_morphWeights[i] * displacement;
	}
#endif

	return normal;
}


vec3 getTargetTangent(int vertexID)
{
	vec3 tangent = vec3(0);

#ifdef MORPH_TARGET_TANGENT_OFFSET 
	for (int i = 0; i < WEIGHT_COUNT; i++)
	{
		vec3 displacement = getDisplacement(vertexID, MORPH_TARGET_TANGENT_OFFSET + i * vertex_count).xyz;
		tangent += u_morphWeights[i] * displacement;
	}
#endif

	return tangent;
}

vec2 getTargetTexCoord0(int vertexID)
{
	vec2 uv = vec2(0);

#ifdef MORPH_TARGET_TEXCOORD_0_OFFSET 
	for (int i = 0; i < WEIGHT_COUNT; i++)
	{
		vec2 displacement = getDisplacement(vertexID, MORPH_TARGET_TEXCOORD_0_OFFSET + i * vertex_count).xy;
		uv += u_morphWeights[i] * displacement;
	}
#endif

	return uv;
}

vec2 getTargetTexCoord1(int vertexID)
{
	vec2 uv = vec2(0);

#ifdef MORPH_TARGET_TEXCOORD_1_OFFSET 
	for (int i = 0; i < WEIGHT_COUNT; i++)
	{
		vec2 displacement = getDisplacement(vertexID, MORPH_TARGET_TEXCOORD_1_OFFSET + i * vertex_count).zw;
		uv += u_morphWeights[i] * displacement;
	}
#endif

	return uv;
}

vec4 getTargetColor0(int vertexID)
{
	vec4 color = vec4(0);

#ifdef MORPH_TARGET_COLOR_0_OFFSET 
	for (int i = 0; i < WEIGHT_COUNT; i++)
	{
		vec4 displacement = getDisplacement(vertexID, MORPH_TARGET_COLOR_0_OFFSET + i * vertex_count);
		color += u_morphWeights[i] * displacement;
	}
#endif

	return color;
}

#endif

