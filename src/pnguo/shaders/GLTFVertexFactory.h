
#include "skinning.h"

//--------------------------------------------------------------------------------------
//  For VS layout
//--------------------------------------------------------------------------------------

#ifdef ID_POSITION
layout(location = ID_POSITION) in vec3 a_Position;
#endif

#ifdef ID_COLOR_0
layout(location = ID_COLOR_0) in  vec3 a_Color0;
#endif

#ifdef ID_TEXCOORD_0
layout(location = ID_TEXCOORD_0) in  vec2 a_UV0;
#endif

#ifdef ID_TEXCOORD_1
layout(location = ID_TEXCOORD_1) in  vec2 a_UV1;
#endif

#ifdef ID_NORMAL
layout(location = ID_NORMAL) in  vec3 a_Normal;
#endif

#ifdef ID_TANGENT
layout(location = ID_TANGENT) in vec4 a_Tangent;
#endif

#ifdef ID_WEIGHTS_0
layout(location = ID_WEIGHTS_0) in  vec4 a_Weights0;
#endif

#ifdef ID_WEIGHTS_1
layout(location = ID_WEIGHTS_1) in  vec4 a_Weights1;
#endif

#ifdef ID_JOINTS_0
layout(location = ID_JOINTS_0) in  uvec4 a_Joints0;
#endif

#ifdef ID_JOINTS_1
layout(location = ID_JOINTS_1) in  uvec4 a_Joints1;
#endif
#ifdef ID_TARGETS
layout(location = ID_TARGETS) in  vec4 a_Targets;
#endif

// Instanced attributes
#ifdef ID_INSTANCE_MAT
layout(location = ID_INSTANCE_MAT) in mat4 instance_mat;
#endif
//--------------------------------------------------------------------------------------
#ifdef ID_POSITION
layout(location = 0) out VS2PS Output;
#endif
void gltfVertexFactory()
{
#ifdef ID_WEIGHTS_0
	mat4 skinningMatrix;
	skinningMatrix = GetSkinningMatrix(a_Weights0, a_Joints0);
#ifdef ID_WEIGHTS_1
	skinningMatrix += GetSkinningMatrix(a_Weights1, a_Joints1);
#endif
#else
	mat4 skinningMatrix =
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 1 }
	};
#endif

	mat4 transMatrix = GetWorldMatrix() * skinningMatrix;
#ifdef ID_INSTANCE_MAT
	transMatrix *= instance_mat;
#endif
	vec4 pos = vec4(a_Position, 1);
#ifdef ID_MORPHING_DATA
	pos += getTargetPosition(gl_VertexIndex);
#endif
	pos = transMatrix * pos;
#ifndef __cplusplus
	Output.WorldPos = vec3(pos.xyz) / pos.w;
#endif
	gl_Position = GetCameraViewProj() * pos; // needs w for proper perspective correction
	mat4 v = GetCameraView();
	vec4 npos = v * pos; // view space position
	Output.depth = npos.z;
#ifdef HAS_MOTION_VECTORS
	Output.CurrPosition = gl_Position; // current's frame vertex position 

	mat4 prevTransMatrix = GetPrevWorldMatrix() * skinningMatrix;
	vec3 worldPrevPos = (prevTransMatrix * vec4(a_Position, 1)).xyz;
	Output.PrevPosition = GetPrevCameraViewProj() * vec4(worldPrevPos, 1);
#endif

#ifdef ID_NORMAL
	vec3 normal = a_Normal;
#ifdef HAS_MORPH_TARGETS
	normal += getTargetNormal(gl_VertexIndex);
#endif
	Output.Normal = normalize(vec3(transMatrix * vec4(normal.xyz, 0.0)));
#endif

#ifdef ID_TANGENT
	vec3 tangent = a_Tangent.xyz;
#ifdef HAS_MORPH_TARGETS
	tangent += getTargetTangent(gl_VertexIndex);
#endif
	Output.Tangent = normalize(vec3(transMatrix * vec4(tangent.xyz, 0.0)));
	Output.Binormal = cross(Output.Normal, Output.Tangent) * a_Tangent.w;
#endif


#ifdef ID_COLOR_0
	Output.Color0 = a_Color0;
#ifdef HAS_MORPH_TARGETS
	Output.Color0 = clamp(Output.Color0 + getTargetColor0(gl_VertexIndex), 0.0f, 1.0f);
#endif 
#endif

#ifdef ID_TEXCOORD_0
	Output.UV0 = a_UV0;
#ifdef HAS_MORPH_TARGETS
	Output.UV0 += getTargetTexCoord0(gl_VertexIndex);
#endif

#endif

#ifdef ID_TEXCOORD_1
	Output.UV1 = a_UV1;
#ifdef HAS_MORPH_TARGETS 
	Output.UV1 += getTargetTexCoord1(gl_VertexIndex);
#endif

#endif  
}

