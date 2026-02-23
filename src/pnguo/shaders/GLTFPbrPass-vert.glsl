#version 450

//--------------------------------------------------------------------------------------
//  Include IO structures
//--------------------------------------------------------------------------------------

#include "GLTF_VS2PS_IO.h"

//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------
 

layout (std140, binding = ID_PER_FRAME) uniform _PerFrame 
{
	mat4          u_mCameraCurrViewProj;
	mat4          u_mCameraPrevViewProj;
}myPerFrame;

layout (std140, binding = ID_PER_OBJECT) uniform perObject
{
    mat4 u_mCurrWorld;
    mat4 u_mPrevWorld;
} myPerObject;


#ifdef ID_INSTANCING_A
layout(location = ID_INSTANCING_A) in mat4 a_instance_model_matrix;
#endif

#ifdef ID_INSTANCING
layout (std140, set = 0, binding = ID_INSTANCING) buffer readonly insObject
{
    mat4 instance_model_matrix[]; 
};
mat4 GetWorldMatrix()
{
	return instance_model_matrix[gl_InstanceIndex] * myPerObject.u_mCurrWorld;
}
#else
mat4 GetWorldMatrix()
{
    return myPerObject.u_mCurrWorld;
}
#endif
mat4 GetCameraViewProj()
{
    return myPerFrame.u_mCameraCurrViewProj;
}

mat4 GetPrevWorldMatrix()
{
    return myPerObject.u_mPrevWorld;
}

mat4 GetPrevCameraViewProj()
{
    return myPerFrame.u_mCameraPrevViewProj;
}

#include "GLTFVertexFactory.h"

//--------------------------------------------------------------------------------------
// Main
//--------------------------------------------------------------------------------------
void main()
{
	gltfVertexFactory();
}

