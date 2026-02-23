#version 450

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------

#include "GLTF_VS2PS_IO.h"

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------

layout (std140, binding = ID_PER_FRAME) uniform myFrame 
{
    mat4        u_mCurrViewProj;
    mat4        u_mPrevViewProj;
    mat4        u_mCameraCurrView;
} myPerFrame;

layout(std140, binding = ID_PER_OBJECT) uniform perObject
{
    mat4        u_mCurrWorld;
    mat4        u_mPrevWorld;
} myPerObject;

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------


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
    return myPerFrame.u_mCurrViewProj;
}

mat4 GetPrevWorldMatrix()
{
    return myPerObject.u_mPrevWorld;
}

mat4 GetCameraView()
{
    return myPerFrame.u_mCameraCurrView;
}

mat4 GetPrevCameraViewProj()
{
    return myPerFrame.u_mPrevViewProj;
}

#include "GLTFVertexFactory.h"

//--------------------------------------------------------------------------------------
// main
//--------------------------------------------------------------------------------------
void main()
{
    gltfVertexFactory();
}

