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
    mat4 u_MVPMatrix;
} myPerFrame;

layout (std140, binding = ID_PER_OBJECT) uniform perObject
{
    mat4 u_ModelMatrix;
} myPerObject;

#ifdef ID_INSTANCING
layout (std140, set = 0, binding = ID_INSTANCING) buffer readonly insObject
{
    mat4 instance_model_matrix[]; 
};
mat4 GetWorldMatrix()
{
	return instance_model_matrix[gl_InstanceIndex] * myPerObject.u_ModelMatrix;
}
#else
mat4 GetWorldMatrix()
{
    return myPerObject.u_ModelMatrix;
}
#endif

mat4 GetCameraViewProj()
{
    return myPerFrame.u_MVPMatrix;
}

mat4 GetCameraView()
{    
    mat4 m =
    {
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 }
    };
    return m;
}

#include "GLTFVertexFactory.h"

//--------------------------------------------------------------------------------------
// main
//--------------------------------------------------------------------------------------
void main()
{
	gltfVertexFactory();
}
