// AMD Cauldron code
// 
// Copyright(c) 2020 Advanced Micro Devices, Inc.All rights reserved.
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

#pragma once
#include <json.hpp>
#define GLM_FORCE_ALIGNED
#define GLM_FORCE_XYZW_ONLY
#define GLM_FORCE_CTOR_INIT GLM_CTOR_INITIALISATION
// 四元数xyxw
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
//#define GLM_FORCE_LEFT_HANDED
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/matrix_inverse.hpp"
#include "../glm/gtc/type_ptr.hpp"
//#include "../glm/gtx/intersect.hpp"
#include "../glm/gtx/matrix_decompose.hpp"

#include "../Misc/Camera.h"
#include "GltfStructures.h"
#include "GltfPbrMaterial.h"

// The GlTF file is loaded in 2 steps
//
// 1) loading the GPU agnostic data that is common to all passes (This is done in the GLTFCommon class you can see here below)
//     - nodes
//     - scenes
//     - animations
//     - binary buffers
//     - skinning skeletons
//
// 2) loading the GPU specific data that is common to any pass (This is done in the GLTFCommonVK class)
//     - textures
//

using json = nlohmann::json;

// Define a maximum number of shadows supported in a scene (note, these are only for spots and directional)
static const uint32_t MaxLightInstances = 80;
static const uint32_t MaxShadowInstances = 32;
class Matrix2
{	
	glm::mat4 m_current = {};
	glm::mat4 m_previous = {};
	//glm::mat4 m_current = {};//glm::mat4::identity();
	//glm::mat4 m_previous = {};//glm::mat4::identity();
public:
	void Set(const glm::mat4& m);
	glm::mat4 GetCurrent() const { return m_current; }
	glm::mat4 GetPrevious() const { return m_previous; }
};

//
// Structures holding the per frame constant buffer data. 
//
struct Light
{
	glm::mat4   mLightViewProj;
	glm::mat4   mLightView;

	float         direction[3];
	float         range;

	float         color[3];
	float         intensity;

	float         position[3];
	float         innerConeCos;

	float         outerConeCos;
	uint32_t      type;
	float         depthBias;
	int32_t       shadowMapIndex = -1;
};


const uint32_t LightType_Directional = 0;
const uint32_t LightType_Point = 1;
const uint32_t LightType_Spot = 2;

struct per_frame
{
	glm::mat4 mCameraCurrViewProj;
	glm::mat4 mCameraPrevViewProj;
	glm::mat4  mInverseCameraCurrViewProj;
	glm::vec4  cameraPos;
	float     iblFactor;
	float     emmisiveFactor;
	float     invScreenResolution[2];

	glm::vec4 wireframeOptions;
	float     lodBias = 0.0f;
	uint32_t  padding[2];
	uint32_t  lightCount;
	Light     lights[MaxLightInstances];
};

//
// GLTFCommon, common stuff that is API agnostic
//
class GLTFCommon
{
public:
	json j3;
#ifdef TINY_GLTF_H_
	tinygltf::Model* pm = 0;
#endif
	std::string m_path;
	std::vector<tfScene> m_scenes;
	std::vector<tfMesh> m_meshes;
	std::vector<tfSkins> m_skins;
	std::vector<tfLight> m_lights;
	std::vector<LightInstance> m_lightInstances;
	std::vector<tfCamera> m_cameras;

	std::vector<tfNode> m_nodes;

	std::vector<tfAnimation> m_animations;
	std::vector<char*> m_buffersData;

	const json* m_pAccessors;
	const json* m_pBufferViews;

	std::vector<glm::mat4> m_animatedMats;       // object space matrices of each node after being animated

	std::vector<Matrix2> m_worldSpaceMats;     // world space matrices of each node after processing the hierarchy
	std::map<int, std::vector<Matrix2>> m_worldSpaceSkeletonMats; // skinning matrices, following the m_jointsNodeIdx order

	per_frame m_perFrameData;

public:
	bool Load(const std::string& path, const std::string& filename);
	void Unload();

	// misc functions
	int FindMeshSkinId(int meshId) const;
	int GetInverseBindMatricesBufferSizeByID(int id) const;
	void GetBufferDetails(int accessor, tfAccessor* pAccessor) const;
	void GetAttributesAccessors(const json& gltfAttributes, std::vector<char*>* pStreamNames, std::vector<tfAccessor>* pAccessors) const;

	// transformation and animation functions
	void SetAnimationTime(uint32_t animationIndex, float time);
	void TransformScene(int sceneIndex, const glm::mat4& world);
	// 运行中使用
	per_frame* SetPerFrameData(const Camera& cam);
	bool GetCamera(uint32_t cameraIdx, Camera* pCam) const;
	tfNodeIdx AddNode(const tfNode& node);
	int AddLight(const tfNode& node, const tfLight& light);
	int get_animation_count();
private:
	void InitTransformedData(); //this is called after loading the data from the GLTF
	void TransformNodes(const glm::mat4& world, const std::vector<tfNodeIdx>* pNodes);
	glm::mat4 ComputeDirectionalLightOrthographicMatrix(const glm::mat4& mLightView);
	void load_Buffers();
	void load_Meshes();
	void load_lights();
	void load_cameras();
	void load_nodes();
	void load_scenes();
	void load_skins();
	void load_animations();

};
