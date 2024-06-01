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

#include "stdafx.h"
#include "GltfCommon.h"
#include "GltfHelpers.h"
#include "Misc/Misc.h"

#ifdef _WIN32
#define fseeki64 _fseeki64
#define ftelli64 _ftelli64
#else			
#define fseeki64 fseeko64
#define ftelli64 ftello64
#endif // _WIN32

bool vkmReadWholeFile(std::vector<unsigned char>* out, std::string* err,
	const std::string& filepath, void*) {
#ifdef TINYGLTF_ANDROID_LOAD_FROM_ASSETS
	if (asset_manager) {
		AAsset* asset = AAssetManager_open(asset_manager, filepath.c_str(),
			AASSET_MODE_STREAMING);
		if (!asset) {
			if (err) {
				(*err) += "File open error : " + filepath + "\n";
			}
			return false;
		}
		size_t size = AAsset_getLength(asset);
		if (size == 0) {
			if (err) {
				(*err) += "Invalid file size : " + filepath +
					" (does the path point to a directory?)";
			}
			return false;
		}
		out->resize(size);
		AAsset_read(asset, reinterpret_cast<char*>(&out->at(0)), size);
		AAsset_close(asset);
		return true;
	}
	else {
		if (err) {
			(*err) += "No asset manager specified : " + filepath + "\n";
		}
		return false;
	}
#else
	int64_t size = 0;
	const char* fn = filepath.c_str();
	FILE* fp = fopen(filepath.c_str(), "rb");
	if (fp)
	{
		fseeki64(fp, 0L, SEEK_END);
		size = ftelli64(fp);
		fseeki64(fp, 0L, SEEK_SET);
		out->resize(size);
		auto buff = out->data();
		auto retval = fread(buff, size, 1, fp);
		assert(retval == 1);
		fclose(fp);
	}
	return fp;// hz::File::read_binary_file(filepath.c_str(), *out);
#endif
}
void Transform::LookAt(glm::vec4 source, glm::vec4 target, bool flipY)
{
	//auto p3 = math::Point3(source.x, source.y, source.z);
	//auto r = math::Matrix4::lookAt(p3
	//	, math::Point3(target.x, target.y, target.z), math::Vector3(0, 1, 0));
	//r = math::inverse(r);
	//auto t = r.getCol(3);  r.setCol(3, math::Vector4(0, 0, 0, 1));

	auto mat = m_rotation = glm::inverse(glm::lookAt(glm::vec3(source), glm::vec3(target), glm::vec3(0, flipY ? -1 : 1, 0)));
	//glm::vec3 scale;
	//glm::quat& rotation = m_rotation;
	//glm::vec3 translation;
	//glm::vec3 skew;
	//glm::vec4 perspective;
	//glm::decompose(mat, scale, rotation, translation, skew, perspective);
	//m_scale = glm::vec4(scale, 0);
	m_translation = mat[3];  m_rotation[3] = glm::vec4(0, 0, 0, 1);

}
void Matrix2::Set(const glm::mat4& m) {
	m_previous = m_current;
	m_current = m;
}


bool GLTFCommon::Load(const std::string& path, const std::string& filename)
{
	Profile p("GLTFCommon::Load");

	m_path = path;


	auto pm1 = new tinygltf::Model();
	if (!pm1)
	{
		return false;
	}
	pm = pm1;
	tinygltf::TinyGLTF loader = {};
	std::string err;
	std::string warn;
	tinygltf::FsCallbacks cb = { &tinygltf::FileExists, &tinygltf::ExpandFilePath,
	&vkmReadWholeFile, &tinygltf::WriteWholeFile, };
	loader.SetFsCallbacks(cb);
	char* bytes = 0;// rawdata.data();
	auto size = 0;// rawdata.size();
	bool ret = false;
	std::string basedir = path;// GetBaseDir(fn);
	if (filename.find(".glb") != std::string::npos)
	{
		ret = loader.LoadBinaryFromFile(pm, &err, &warn, path + filename);
	}
	else
	{
		ret = loader.LoadASCIIFromFile(pm, &err, &warn, path + filename);
	}
	// for binary glTF(.glb)
	//auto ret = loader.LoadBinaryFromMemory(pm, &err, &warn, (unsigned char*)bytes, size, basedir);
	//err.clear();
	//if (!ret) {
	//	ret = loader.LoadASCIIFromString(pm, &err, &warn, (char*)bytes, size, basedir);
	//}
	do {
		if (err.size())
		{
			printf("error:\t%s\n", err.c_str());
			delete pm;
			pm = 0;
			break;
		}
		if (warn.size())
			printf("warn:\t%s\n", warn.c_str());
		load_Buffers();
		load_Meshes();
		load_lights();
		load_cameras();
		load_nodes();
		load_scenes();
		load_skins();
		load_animations();
		InitTransformedData();
		return true;
	} while (0);
	std::ifstream f(path + filename);
	if (!f)
	{
		Trace(format("The file %s cannot be found\n", filename.c_str()));
		return false;
	}
	f >> j3;

	// Load Buffers
	//
	if (j3.find("buffers") != j3.end())
	{
		const json& buffers = j3["buffers"];
		m_buffersData.resize(buffers.size());
		for (int i = 0; i < buffers.size(); i++)
		{
			const std::string& name = buffers[i]["uri"];
			std::ifstream ff(path + name, std::ios::in | std::ios::binary);

			ff.seekg(0, ff.end);
			std::streamoff length = ff.tellg();
			ff.seekg(0, ff.beg);

			char* p = new char[length];
			ff.read(p, length);
			m_buffersData[i] = p;
		}
	}

	// Load Meshes
	//
	m_pAccessors = &j3["accessors"];
	m_pBufferViews = &j3["bufferViews"];
	const json& meshes = j3["meshes"];
	m_meshes.resize(meshes.size());
	for (int i = 0; i < meshes.size(); i++)
	{
		tfMesh* tfmesh = &m_meshes[i];
		auto& primitives = meshes[i]["primitives"];
		tfmesh->m_pPrimitives.resize(primitives.size());
		for (int p = 0; p < primitives.size(); p++)
		{
			tfPrimitives* pPrimitive = &tfmesh->m_pPrimitives[p];

			int positionId = primitives[p]["attributes"]["POSITION"];
			const json& accessor = m_pAccessors->at(positionId);

			glm::vec4 max = GetVector(GetElementJsonArray(accessor, "max", { 0.0, 0.0, 0.0, 0.0 }));
			glm::vec4 min = GetVector(GetElementJsonArray(accessor, "min", { 0.0, 0.0, 0.0, 0.0 }));

			pPrimitive->m_center = (min + max) * 0.5f;
			pPrimitive->m_radius = max - pPrimitive->m_center;

			pPrimitive->m_center = glm::vec4(glm::vec3(pPrimitive->m_center), 1.0f); //set the W to 1 since this is a position not a direction
		}
	}

	// Load lights
	//
	if (j3.find("extensions") != j3.end())
	{
		const json& extensions = j3["extensions"];
		if (extensions.find("KHR_lights_punctual") != extensions.end())
		{
			const json& KHR_lights_punctual = extensions["KHR_lights_punctual"];
			if (KHR_lights_punctual.find("lights") != KHR_lights_punctual.end())
			{
				const json& lights = KHR_lights_punctual["lights"];
				m_lights.resize(lights.size());
				for (int i = 0; i < lights.size(); i++)
				{
					json::object_t light = lights[i];
					m_lights[i].m_color = GetElementVector(light, "color", glm::vec4(1, 1, 1, 0));
					m_lights[i].m_range = GetElementFloat(light, "range", 105);
					m_lights[i].m_intensity = GetElementFloat(light, "intensity", 1);
					m_lights[i].m_innerConeAngle = GetElementFloat(light, "spot/innerConeAngle", 0);
					m_lights[i].m_outerConeAngle = GetElementFloat(light, "spot/outerConeAngle", XM_PIDIV4);

					std::string lightName = GetElementString(light, "name", "");

					std::string lightType = GetElementString(light, "type", "");
					if (lightType == "spot")
						m_lights[i].m_type = tfLight::LIGHT_SPOTLIGHT;
					else if (lightType == "point")
						m_lights[i].m_type = tfLight::LIGHT_POINTLIGHT;
					else if (lightType == "directional")
						m_lights[i].m_type = tfLight::LIGHT_DIRECTIONAL;

					// Deal with shadow settings
					if (m_lights[i].m_type == tfLight::LIGHT_DIRECTIONAL || m_lights[i].m_type == tfLight::LIGHT_SPOTLIGHT)
					{
						// Unless "NoShadow" is present in the name, the light will have a shadow
						if (std::string::npos != lightName.find("NoShadow", 0, 8))
							m_lights[i].m_shadowResolution = 0; // 0 shadow resolution means no shadow
						else
						{
							// See if we have specified a resolution
							size_t offset = lightName.find("Resolution_", 0, 11);
							if (std::string::npos != offset)
							{
								// Update offset to start from after "_"
								offset += 11;

								// Look for the end separator
								size_t endOffset = lightName.find("_", offset, 1);
								if (endOffset != std::string::npos)
								{
									// Try to grab the value
									std::string ResString = lightName.substr(offset, endOffset - offset);
									int32_t Resolution = -1;
									try {
										Resolution = std::stoi(ResString);
									}
									catch (std::invalid_argument)
									{
										// Wasn't a valid argument to convert to int, use default
									}
									catch (std::out_of_range)
									{
										// Value larger than an int can hold (also invalid), use default
									}

									// Check if resolution is a power of 2
									if (Resolution == 1 || (Resolution & (Resolution - 1)) == 0)
										m_lights[i].m_shadowResolution = (uint32_t)Resolution;
								}
							}

							// See if we have specified a bias 
							offset = lightName.find("Bias_", 0, 5);
							if (std::string::npos != offset)
							{
								// Update offset to start from after "_"
								offset += 5;

								// Look for the end separator
								size_t endOffset = lightName.find("_", offset, 1);
								if (endOffset != std::string::npos)
								{
									// Try to grab the value
									std::string BiasString = lightName.substr(offset, endOffset - offset);
									float Bias = (m_lights[i].m_type == LightType_Spot) ? (70.0f / 100000.0f) : (1000.0f / 100000.0f);

									try {
										Bias = std::stof(BiasString);
									}
									catch (std::invalid_argument)
									{
										// Wasn't a valid argument to convert to float, use default
									}
									catch (std::out_of_range)
									{
										// Value larger than a float can hold (also invalid), use default
									}

									// Set what we have 
									m_lights[i].m_bias = Bias;
								}
							}
						}
					}
				}
			}
		}
	}

	// Load cameras
	//
	if (j3.find("cameras") != j3.end())
	{
		const json& cameras = j3["cameras"];
		m_cameras.resize(cameras.size());
		for (int i = 0; i < cameras.size(); i++)
		{
			const json& camera = cameras[i];
			tfCamera* tfcamera = &m_cameras[i];

			tfcamera->yfov = GetElementFloat(camera, "perspective/yfov", 0.1f);
			tfcamera->znear = GetElementFloat(camera, "perspective/znear", 0.1f);
			tfcamera->zfar = GetElementFloat(camera, "perspective/zfar", 100.0f);
			tfcamera->m_nodeIndex = -1;
		}
	}

	// Load nodes
	//
	if (j3.find("nodes") != j3.end())
	{
		const json& nodes = j3["nodes"];
		m_nodes.resize(nodes.size());
		for (int i = 0; i < nodes.size(); i++)
		{
			tfNode* tfnode = &m_nodes[i];

			// Read node data
			//
			json::object_t node = nodes[i];

			if (node.find("children") != node.end())
			{
				for (int c = 0; c < node["children"].size(); c++)
				{
					int nodeID = node["children"][c];
					tfnode->m_children.push_back(nodeID);
				}
			}

			tfnode->meshIndex = GetElementInt(node, "mesh", -1);
			tfnode->skinIndex = GetElementInt(node, "skin", -1);

			int cameraIdx = GetElementInt(node, "camera", -1);
			if (cameraIdx >= 0)
				m_cameras[cameraIdx].m_nodeIndex = i;

			int lightIdx = GetElementInt(node, "extensions/KHR_lights_punctual/light", -1);
			if (lightIdx >= 0)
			{
				m_lightInstances.push_back({ lightIdx, i });
			}

			tfnode->m_tranform.m_translation = GetElementVector(node, "translation", glm::vec4(0, 0, 0, 0));
			tfnode->m_tranform.m_scale = GetElementVector(node, "scale", glm::vec4(1, 1, 1, 0));


			if (node.find("name") != node.end())
				tfnode->m_name = GetElementString(node, "name", "unnamed");

			if (node.find("rotation") != node.end())
			{
				auto q = GetVector(node["rotation"].get<json::array_t>());
				tfnode->m_tranform.m_rotation = glm::mat4(glm::quat(q.w, q.x, q.y, q.z));// , glm::vec3(0.f, 0.f, 0.f));
			}
			//tfnode->m_tranform.m_rotation = glm::mat4(math::Quat(GetVector(node["rotation"].get<json::array_t>())), math::Vector3(0.f, 0.f, 0.f));
			else if (node.find("matrix") != node.end())
				tfnode->m_tranform.m_rotation = GetMatrix(node["matrix"].get<json::array_t>());
			else
				tfnode->m_tranform.m_rotation = {};//glm::mat4::identity();
		}
	}

	// Load scenes
	//
	if (j3.find("scenes") != j3.end())
	{
		const json& scenes = j3["scenes"];
		m_scenes.resize(scenes.size());
		for (int i = 0; i < scenes.size(); i++)
		{
			auto scene = scenes[i];
			for (int n = 0; n < scene["nodes"].size(); n++)
			{
				int nodeId = scene["nodes"][n];
				m_scenes[i].m_nodes.push_back(nodeId);
			}
		}
	}

	// Load skins
	//
	if (j3.find("skins") != j3.end())
	{
		const json& skins = j3["skins"];
		m_skins.resize(skins.size());
		for (uint32_t i = 0; i < skins.size(); i++)
		{
			GetBufferDetails(skins[i]["inverseBindMatrices"].get<int>(), &m_skins[i].m_InverseBindMatrices);

			if (skins[i].find("skeleton") != skins[i].end())
				m_skins[i].m_pSkeleton = &m_nodes[skins[i]["skeleton"]];

			const json& joints = skins[i]["joints"];
			for (uint32_t n = 0; n < joints.size(); n++)
			{
				m_skins[i].m_jointsNodeIdx.push_back(joints[n]);
			}

		}
	}

	// Load animations
	//
	if (j3.find("animations") != j3.end())
	{
		const json& animations = j3["animations"];
		m_animations.resize(animations.size());
		for (int i = 0; i < animations.size(); i++)
		{
			const json& channels = animations[i]["channels"];
			const json& samplers = animations[i]["samplers"];

			tfAnimation* tfanim = &m_animations[i];
			for (int c = 0; c < channels.size(); c++)
			{
				json::object_t channel = channels[c];
				int sampler = channel["sampler"];
				int node = GetElementInt(channel, "target/node", -1);
				std::string path = GetElementString(channel, "target/path", std::string());

				tfChannel* tfchannel;

				auto ch = tfanim->m_channels.find(node);
				if (ch == tfanim->m_channels.end())
				{
					tfchannel = &tfanim->m_channels[node];
				}
				else
				{
					tfchannel = &ch->second;
				}

				tfSampler* tfsmp = new tfSampler();

				// Get time line
				//
				GetBufferDetails(samplers[sampler]["input"], &tfsmp->m_time);
				assert(tfsmp->m_time.m_stride == 4);

				tfanim->m_duration = std::max<float>(tfanim->m_duration, *(float*)tfsmp->m_time.Get(tfsmp->m_time.m_count - 1));

				// Get value line
				//
				GetBufferDetails(samplers[sampler]["output"], &tfsmp->m_value);

				// Index appropriately
				// 
				if (path == "translation")
				{
					tfchannel->m_pTranslation = tfsmp;
					assert(tfsmp->m_value.m_stride == 3 * 4);
					assert(tfsmp->m_value.m_dimension == 3);
				}
				else if (path == "rotation")
				{
					tfchannel->m_pRotation = tfsmp;
					assert(tfsmp->m_value.m_stride == 4 * 4);
					assert(tfsmp->m_value.m_dimension == 4);
				}
				else if (path == "scale")
				{
					tfchannel->m_pScale = tfsmp;
					assert(tfsmp->m_value.m_stride == 3 * 4);
					assert(tfsmp->m_value.m_dimension == 3);
				}
			}
		}
	}

	InitTransformedData();

	return true;
}

void GLTFCommon::Unload()
{
	for (int i = 0; i < m_buffersData.size(); i++)
	{
		delete[] m_buffersData[i];
	}
	m_buffersData.clear();

	m_animations.clear();
	m_nodes.clear();
	m_scenes.clear();
	m_lights.clear();
	m_lightInstances.clear();

	j3.clear();
}


void GLTFCommon::load_Buffers()
{
	{
		auto& buffers = pm->buffers;
		m_buffersData.resize(buffers.size());
		for (int i = 0; i < buffers.size(); i++)
		{
			auto& it = buffers[i];
			if (it.data.size())
			{
				m_buffersData[i] = (char*)it.data.data();
				continue;
			}
			const std::string& name = it.uri;
			std::ifstream ff(m_path + name, std::ios::in | std::ios::binary);

			ff.seekg(0, ff.end);
			std::streamoff length = ff.tellg();
			if (length > 0)
			{
				ff.seekg(0, ff.beg);

				char* p = new char[length];
				ff.read(p, length);
				m_buffersData[i] = p;
			}
		}
	}
}

template<class T, class T1>
T1 getv4(const T& v, T1 d)
{
	T1 r = d;// { 0, 0, 0, 0 };
	for (int i = 0; i < v.size() && i < 4; i++)
	{
		r[i] = v[i];
	}
	return r;
}

template<class T>
glm::mat4 get_mat(const T& accessor)
{
	return glm::mat4(
		glm::vec4(accessor[0], accessor[1], accessor[2], accessor[3]),
		glm::vec4(accessor[4], accessor[5], accessor[6], accessor[7]),
		glm::vec4(accessor[8], accessor[9], accessor[10], accessor[11]),
		glm::vec4(accessor[12], accessor[13], accessor[14], accessor[15]));
}
template<class T>
glm::mat4 get_mat1(const T& accessor)
{
	return glm::mat4(
		glm::vec4(accessor[0], accessor[1], accessor[2], accessor[3]),
		glm::vec4(accessor[4], accessor[5], accessor[6], accessor[7]),
		glm::vec4(accessor[8], accessor[9], accessor[10], accessor[11]),
		glm::vec4(accessor[12], accessor[13], accessor[14], accessor[15]));
}
//glm::vec4 GetVector(const json::array_t& accessor)
//{
//	return glm::vec4(accessor[0], accessor[1], accessor[2], (accessor.size() == 4) ? accessor[3] : 0);
//}

void GLTFCommon::load_Meshes()
{
	//m_pAccessors = &j3["accessors"];
	//m_pBufferViews = &j3["bufferViews"];
	auto& meshes = pm->meshes;
	m_meshes.resize(meshes.size());
	for (int i = 0; i < meshes.size(); i++)
	{
		tfMesh* tfmesh = &m_meshes[i];
		auto& primitives = meshes[i].primitives;
		tfmesh->m_pPrimitives.resize(primitives.size());
		for (int p = 0; p < primitives.size(); p++)
		{
			tfPrimitives* pPrimitive = &tfmesh->m_pPrimitives[p];

			int positionId = primitives[p].attributes["POSITION"];
			auto& accessor = pm->accessors[positionId];// m_pAccessors->at(positionId);

			glm::vec4 max1 = getv4(accessor.maxValues, glm::vec4(0, 0, 0, 0));// = GetVector(GetElementJsonArray(accessor, "max", { 0.0, 0.0, 0.0, 0.0 }));
			glm::vec4 min1 = getv4(accessor.minValues, glm::vec4(0, 0, 0, 0));// = GetVector(GetElementJsonArray(accessor, "min", { 0.0, 0.0, 0.0, 0.0 }));

			pPrimitive->m_center = (min1 + max1) * 0.5f;
			pPrimitive->m_radius = max1 - pPrimitive->m_center;

			pPrimitive->m_center = glm::vec4(glm::vec3(pPrimitive->m_center), 1.0f); //set the W to 1 since this is a position not a direction
		}
	}
}
void GLTFCommon::load_lights()
{
	if (pm)
	{
		m_lights.resize(pm->lights.size());
		for (int i = 0; i < pm->lights.size(); i++)
		{
			auto& light = pm->lights[i];
			m_lights[i].m_color = getv4(light.color, glm::vec4(1, 1, 1, 0));// GetElementVector(light, "color", glm::vec4(1, 1, 1, 0));
			m_lights[i].m_range = light.range;// GetElementFloat(light, "range", 105);
			m_lights[i].m_intensity = light.intensity;// GetElementFloat(light, "intensity", 1);
			m_lights[i].m_innerConeAngle = light.spot.innerConeAngle;//GetElementFloat(light, "spot/innerConeAngle", 0);
			m_lights[i].m_outerConeAngle = light.spot.outerConeAngle;// GetElementFloat(light, "spot/outerConeAngle", XM_PIDIV4);

			std::string lightName = light.name;// GetElementString(light, "name", "");

			std::string lightType = light.type;// GetElementString(light, "type", "");
			if (lightType == "spot")
				m_lights[i].m_type = tfLight::LIGHT_SPOTLIGHT;
			else if (lightType == "point")
				m_lights[i].m_type = tfLight::LIGHT_POINTLIGHT;
			else if (lightType == "directional")
				m_lights[i].m_type = tfLight::LIGHT_DIRECTIONAL;

			// Deal with shadow settings
			if (m_lights[i].m_type == tfLight::LIGHT_DIRECTIONAL || m_lights[i].m_type == tfLight::LIGHT_SPOTLIGHT)
			{
				// Unless "NoShadow" is present in the name, the light will have a shadow
				if (std::string::npos != lightName.find("NoShadow", 0, 8))
					m_lights[i].m_shadowResolution = 0; // 0 shadow resolution means no shadow
				else
				{
					// See if we have specified a resolution
					size_t offset = lightName.find("Resolution_", 0, 11);
					if (std::string::npos != offset)
					{
						// Update offset to start from after "_"
						offset += 11;

						// Look for the end separator
						size_t endOffset = lightName.find("_", offset, 1);
						if (endOffset != std::string::npos)
						{
							// Try to grab the value
							std::string ResString = lightName.substr(offset, endOffset - offset);
							int32_t Resolution = -1;
							try {
								Resolution = std::stoi(ResString);
							}
							catch (std::invalid_argument)
							{
								// Wasn't a valid argument to convert to int, use default
							}
							catch (std::out_of_range)
							{
								// Value larger than an int can hold (also invalid), use default
							}

							// Check if resolution is a power of 2
							if (Resolution == 1 || (Resolution & (Resolution - 1)) == 0)
								m_lights[i].m_shadowResolution = (uint32_t)Resolution;
						}
					}

					// See if we have specified a bias 
					offset = lightName.find("Bias_", 0, 5);
					if (std::string::npos != offset)
					{
						// Update offset to start from after "_"
						offset += 5;

						// Look for the end separator
						size_t endOffset = lightName.find("_", offset, 1);
						if (endOffset != std::string::npos)
						{
							// Try to grab the value
							std::string BiasString = lightName.substr(offset, endOffset - offset);
							float Bias = (m_lights[i].m_type == LightType_Spot) ? (70.0f / 100000.0f) : (1000.0f / 100000.0f);

							try {
								Bias = std::stof(BiasString);
							}
							catch (std::invalid_argument)
							{
								// Wasn't a valid argument to convert to float, use default
							}
							catch (std::out_of_range)
							{
								// Value larger than a float can hold (also invalid), use default
							}

							// Set what we have 
							m_lights[i].m_bias = Bias;
						}
					}
				}
			}
		}
	}
}
void GLTFCommon::load_cameras()
{
	auto& cameras = pm->cameras;
	m_cameras.resize(cameras.size());
	for (int i = 0; i < cameras.size(); i++)
	{
		auto& camera = cameras[i];
		tfCamera* tfcamera = &m_cameras[i];
		tfcamera->yfov = camera.perspective.yfov;// GetElementFloat(camera, "perspective/yfov", 0.1f);
		tfcamera->znear = camera.perspective.znear;// GetElementFloat(camera, "perspective/znear", 0.1f);
		tfcamera->zfar = camera.perspective.zfar;// GetElementFloat(camera, "perspective/zfar", 100.0f);
		tfcamera->m_nodeIndex = -1;
	}
}
void GLTFCommon::load_nodes()
{
	auto& nodes = pm->nodes;
	m_nodes.resize(nodes.size());
	for (int i = 0; i < nodes.size(); i++)
	{
		tfNode* tfnode = &m_nodes[i];

		// Read node data
		//
		auto& node = nodes[i];

		for (int c = 0; c < node.children.size(); c++)
		{
			int nodeID = node.children[c];
			tfnode->m_children.push_back(nodeID);
		}

		tfnode->meshIndex = node.mesh;// GetElementInt(node, "mesh", -1);
		tfnode->skinIndex = node.skin;// GetElementInt(node, "skin", -1);

		int cameraIdx = node.camera;// GetElementInt(node, "camera", -1);
		if (cameraIdx >= 0)
			m_cameras[cameraIdx].m_nodeIndex = i;

		node.extras;
		if (node.extensions.find("KHR_lights_punctual") != node.extensions.end())
		{
			auto lp = node.extensions["KHR_lights_punctual"];
			// GetElementInt(node, "extensions/KHR_lights_punctual/light", -1);
			if (lp.Has("light"))
			{
				int lightIdx = lp.Get("light").Get<int>();
				if (lightIdx > -1)
					m_lightInstances.push_back({ lightIdx, i });
			}

		}
		if (node.translation.size() > 2)
			tfnode->m_tranform.m_translation = glm::vec4(glm::make_vec3(node.translation.data()), 0.0f);// GetElementVector(node, "translation", glm::vec4(0, 0, 0, 0));

		if (node.scale.size() > 2)
			tfnode->m_tranform.m_scale = glm::vec4(glm::make_vec3(node.scale.data()), 0.0f);// getv4(node.scale, glm::vec4(1, 1, 1, 0));//GetElementVector(node, "scale", glm::vec4(1, 1, 1, 0));

		tfnode->m_name = node.name.c_str() ? node.name : "unnamed";// GetElementString(node, "name", "unnamed");
		tfnode->m_tranform.m_rotation = {};//glm::mat4::identity();
		if (node.rotation.size())
		{
			glm::quat q = glm::make_quat(node.rotation.data());
			tfnode->m_tranform.m_rotation = glm::mat4(q);
		}
		else if (node.matrix.size())
		{
			tfnode->m_tranform.m_rotation = (glm::make_mat4x4(node.matrix.data()));
		}

	}
}
void GLTFCommon::load_scenes()
{
	auto& scenes = pm->scenes;
	m_scenes.resize(scenes.size());
	for (int i = 0; i < scenes.size(); i++)
	{
		auto& scene = scenes[i];
		for (int n = 0; n < scene.nodes.size(); n++)
		{
			int nodeId = scene.nodes[n];
			m_scenes[i].m_nodes.push_back(nodeId);
		}
	}
}
void GLTFCommon::load_skins()
{
	auto& skins = pm->skins;
	m_skins.resize(skins.size());
	for (uint32_t i = 0; i < skins.size(); i++)
	{
		GetBufferDetails(skins[i].inverseBindMatrices, &m_skins[i].m_InverseBindMatrices);

		m_skins[i].m_pSkeleton = &m_nodes[skins[i].skeleton];

		auto& joints = skins[i].joints;
		for (uint32_t n = 0; n < joints.size(); n++)
		{
			m_skins[i].m_jointsNodeIdx.push_back(joints[n]);
		}
	}
}
void GLTFCommon::load_animations()
{
	auto& animations = pm->animations;
	m_animations.resize(animations.size());
	for (int i = 0; i < animations.size(); i++)
	{
		auto& channels = animations[i].channels;
		auto& samplers = animations[i].samplers;

		tfAnimation* tfanim = &m_animations[i];
		for (int c = 0; c < channels.size(); c++)
		{
			auto& channel = channels[c];
			int sampler = channel.sampler;
			int node = channel.target_node;// GetElementInt(channel, "target/node", -1);
			std::string path = channel.target_path;// GetElementString(channel, "target/path", std::string());

			tfChannel* tfchannel;

			auto ch = tfanim->m_channels.find(node);
			if (ch == tfanim->m_channels.end())
			{
				tfchannel = &tfanim->m_channels[node];
			}
			else
			{
				tfchannel = &ch->second;
			}

			tfSampler* tfsmp = new tfSampler();

			// Get time line
			//
			GetBufferDetails(samplers[sampler].input, &tfsmp->m_time);
			assert(tfsmp->m_time.m_stride == 4);

			tfanim->m_duration = std::max<float>(tfanim->m_duration, *(float*)tfsmp->m_time.Get(tfsmp->m_time.m_count - 1));

			// Get value line
			//
			GetBufferDetails(samplers[sampler].output, &tfsmp->m_value);

			// Index appropriately
			// 
			if (path == "translation")
			{
				tfchannel->m_pTranslation = tfsmp;
				assert(tfsmp->m_value.m_stride == 3 * 4);
				assert(tfsmp->m_value.m_dimension == 3);
			}
			else if (path == "rotation")
			{
				tfchannel->m_pRotation = tfsmp;
				assert(tfsmp->m_value.m_stride == 4 * 4);
				assert(tfsmp->m_value.m_dimension == 4);
			}
			else if (path == "scale")
			{
				tfchannel->m_pScale = tfsmp;
				assert(tfsmp->m_value.m_stride == 3 * 4);
				assert(tfsmp->m_value.m_dimension == 3);
			}
		}
	}
}
int GLTFCommon::get_animation_count()
{
	return m_animations.size();
}
//
// Animates the matrices (they are still in object space)
//
void GLTFCommon::SetAnimationTime(uint32_t animationIndex, float time)
{
	if (animationIndex < m_animations.size())
	{
		tfAnimation* anim = &m_animations[animationIndex];

		//loop animation
		time = fmod(time, anim->m_duration);

		for (auto it = anim->m_channels.begin(); it != anim->m_channels.end(); it++)
		{
			Transform* pSourceTrans = &m_nodes[it->first].m_tranform;
			Transform animated;
			float frac, * pCurr, * pNext;
			// Animate translation
			//
			if (it->second.m_pTranslation != NULL)
			{
				it->second.m_pTranslation->SampleLinear(time, &frac, &pCurr, &pNext);
				//animated.m_translation = ((1.0f - frac) * glm::vec4(pCurr[0], pCurr[1], pCurr[2], 0)) + ((frac)*glm::vec4(pNext[0], pNext[1], pNext[2], 0));
				animated.m_translation = glm::mix(glm::vec4(pCurr[0], pCurr[1], pCurr[2], 0), glm::vec4(pNext[0], pNext[1], pNext[2], 0), frac);
			}
			else
			{
				animated.m_translation = pSourceTrans->m_translation;
			}
			// Animate rotation
			//
			if (it->second.m_pRotation != NULL)
			{
				it->second.m_pRotation->SampleLinear(time, &frac, &pCurr, &pNext);
				//auto kr = math::Matrix4(math::slerp(frac, math::Quat(pCurr[0], pCurr[1], pCurr[2], pCurr[3])
				//, math::Quat(pNext[0], pNext[1], pNext[2], pNext[3])), math::Vector3(0.0f, 0.0f, 0.0f));
				glm::quat r = glm::normalize(glm::slerp(glm::make_quat(pCurr), glm::make_quat(pNext), frac));
				animated.m_rotation = glm::mat4(r);
			}
			else
			{
				animated.m_rotation = pSourceTrans->m_rotation;
			}
			// Animate scale
			//
			if (it->second.m_pScale != NULL)
			{
				it->second.m_pScale->SampleLinear(time, &frac, &pCurr, &pNext);
				//animated.m_scale = ((1.0f - frac) * glm::vec4(pCurr[0], pCurr[1], pCurr[2], 0)) + ((frac)*glm::vec4(pNext[0], pNext[1], pNext[2], 0));
				animated.m_scale = glm::mix(glm::vec4(pCurr[0], pCurr[1], pCurr[2], 0), glm::vec4(pNext[0], pNext[1], pNext[2], 0), frac);
			}
			else
			{
				animated.m_scale = pSourceTrans->m_scale;
			}

			m_animatedMats[it->first] = animated.GetWorldMat();
		}
	}
}

void GLTFCommon::GetBufferDetails(int accessor, tfAccessor* pAccessor) const
{

	if (pm)
	{
		auto& inAccessor = pm->accessors[accessor];

		int32_t bufferViewIdx = inAccessor.bufferView;// .value("bufferView", -1);
		assert(bufferViewIdx >= 0);
		auto& bufferView = pm->bufferViews[bufferViewIdx];

		int32_t bufferIdx = bufferView.buffer;// .value("buffer", -1);
		assert(bufferIdx >= 0);

		char* buffer = m_buffersData[bufferIdx];

		int32_t offset = bufferView.byteOffset;// .value("byteOffset", 0);

		int byteLength = bufferView.byteLength;

		int32_t byteOffset = inAccessor.byteOffset;
		offset += byteOffset;
		byteLength -= byteOffset;

		pAccessor->m_data = &buffer[offset];
		pAccessor->m_dimension = tinygltf::GetNumComponentsInType(inAccessor.type);
		pAccessor->m_type = GetFormatSize(inAccessor.componentType);
		pAccessor->m_stride = pAccessor->m_dimension * pAccessor->m_type;
		pAccessor->m_count = inAccessor.count;
		return;
	}
	const json& inAccessor = m_pAccessors->at(accessor);

	int32_t bufferViewIdx = inAccessor.value("bufferView", -1);
	assert(bufferViewIdx >= 0);
	const json& bufferView = m_pBufferViews->at(bufferViewIdx);

	int32_t bufferIdx = bufferView.value("buffer", -1);
	assert(bufferIdx >= 0);

	char* buffer = m_buffersData[bufferIdx];

	int32_t offset = bufferView.value("byteOffset", 0);

	int byteLength = bufferView["byteLength"];

	int32_t byteOffset = inAccessor.value("byteOffset", 0);
	offset += byteOffset;
	byteLength -= byteOffset;

	pAccessor->m_data = &buffer[offset];
	pAccessor->m_dimension = GetDimensions(inAccessor["type"]);
	pAccessor->m_type = GetFormatSize(inAccessor["componentType"]);
	pAccessor->m_stride = pAccessor->m_dimension * pAccessor->m_type;
	pAccessor->m_count = inAccessor["count"];
}

void GLTFCommon::GetAttributesAccessors(const json& gltfAttributes, std::vector<char*>* pStreamNames, std::vector<tfAccessor>* pAccessors) const
{
	int streamIndex = 0;
	for (int s = 0; s < pStreamNames->size(); s++)
	{
		auto attr = gltfAttributes.find(pStreamNames->at(s));
		if (attr != gltfAttributes.end())
		{
			tfAccessor accessor;
			GetBufferDetails(attr.value(), &accessor);
			pAccessors->push_back(accessor);
		}
	}
}

//
// Given a mesh find the skin it belongs to
//
int GLTFCommon::FindMeshSkinId(int meshId) const
{
	for (int i = 0; i < m_nodes.size(); i++)
	{
		if (m_nodes[i].meshIndex == meshId)
		{
			return m_nodes[i].skinIndex;
		}
	}

	return -1;
}

//
// given a skinId return the size of the skeleton matrices (vulkan needs this to compute the offsets into the uniform buffers)
//
int GLTFCommon::GetInverseBindMatricesBufferSizeByID(int id) const
{
	if (id == -1 || (id >= m_skins.size()))
		return -1;

	//return m_skins[id].m_InverseBindMatrices.m_count * (4 * 4 * sizeof(float));
	return m_skins[id].m_InverseBindMatrices.m_count * (sizeof(Matrix2));
}

//
// Transforms a node hierarchy recursively 
//
void GLTFCommon::TransformNodes(const glm::mat4& world, const std::vector<tfNodeIdx>* pNodes)
{
	for (uint32_t n = 0; n < pNodes->size(); n++)
	{
		uint32_t nodeIdx = pNodes->at(n);

		glm::mat4 m = world * m_animatedMats[nodeIdx];
		m_worldSpaceMats[nodeIdx].Set(m);
		TransformNodes(m, &m_nodes[nodeIdx].m_children);
	}
}

//
// Initializes the GLTFCommonTransformed structure 
//
void GLTFCommon::InitTransformedData()
{
	// initializes matrix buffers to have the same dimension as the nodes
	m_worldSpaceMats.resize(m_nodes.size());

	// same thing for the skinning matrices but using the size of the InverseBindMatrices
	for (uint32_t i = 0; i < m_skins.size(); i++)
	{
		m_worldSpaceSkeletonMats[i].resize(m_skins[i].m_InverseBindMatrices.m_count);
	}

	// sets the animated data to the default values of the nodes
	// later on these values can be updated by the SetAnimationTime function
	m_animatedMats.resize(m_nodes.size());
	for (uint32_t i = 0; i < m_nodes.size(); i++)
	{
		m_animatedMats[i] = m_nodes[i].m_tranform.GetWorldMat();
	}
}

//
// Takes the animated matrices and processes the hierarchy, also computes the skinning matrix buffers. 
//
void GLTFCommon::TransformScene(int sceneIndex, const glm::mat4& world)
{
	m_worldSpaceMats.resize(m_nodes.size());

	// transform all the nodes of the scene (and make 
	//           
	std::vector<int> sceneNodes = { m_scenes[sceneIndex].m_nodes };
	TransformNodes(world, &sceneNodes);

	//process skeletons, takes the skinning matrices from the scene and puts them into a buffer that the vertex shader will consume
	//
	for (uint32_t i = 0; i < m_skins.size(); i++)
	{
		tfSkins& skin = m_skins[i];

		//pick the matrices that affect the skin and multiply by the inverse of the bind      
		glm::mat4* pM = (glm::mat4*)skin.m_InverseBindMatrices.m_data;

		std::vector<Matrix2>& skinningMats = m_worldSpaceSkeletonMats[i];
		for (int j = 0; j < skin.m_InverseBindMatrices.m_count; j++)
		{
			skinningMats[j].Set(m_worldSpaceMats[skin.m_jointsNodeIdx[j]].GetCurrent() * pM[j]);
		}
	}
}

bool GLTFCommon::GetCamera(uint32_t cameraIdx, Camera* pCam) const
{
	if (cameraIdx < 0 || cameraIdx >= m_cameras.size())
	{
		return false;
	}
	glm::mat4 m1 = m_worldSpaceMats[m_cameras[cameraIdx].m_nodeIndex].GetCurrent();
	pCam->SetMatrix(m1);
	pCam->SetFov(m_cameras[cameraIdx].yfov, pCam->GetAspectRatio(), m_cameras[cameraIdx].znear, m_cameras[cameraIdx].zfar);

	return true;
}

//
// Sets the per frame data from the GLTF, returns a pointer to it in case the user wants to override some values
// The scene needs to be animated and transformed before we can set the per_frame data. We need those final matrices for the lights and the camera.
//
per_frame* GLTFCommon::SetPerFrameData(const Camera& cam)
{
	Matrix2* pMats = m_worldSpaceMats.data();

	//Sets the camera
	m_perFrameData.mCameraCurrViewProj = cam.GetProjection() * cam.GetView();
	m_perFrameData.mCameraPrevViewProj = cam.GetProjection() * cam.GetPrevView();
	// more accurate calculation
	m_perFrameData.mInverseCameraCurrViewProj = glm::affineInverse(cam.GetView()) * glm::inverse(cam.GetProjection());
	m_perFrameData.cameraPos = cam.GetPosition();

	m_perFrameData.wireframeOptions = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

	// Process lights
	m_perFrameData.lightCount = (int32_t)m_lightInstances.size();
	int32_t ShadowMapIndex = 0;
	for (int i = 0; i < m_lightInstances.size(); i++)
	{
		Light* pSL = &m_perFrameData.lights[i];

		// get light data and node trans
		const tfLight& lightData = m_lights[m_lightInstances[i].m_lightId];
		glm::mat4 lightMat = pMats[m_lightInstances[i].m_nodeIndex].GetCurrent();

		glm::mat4 lightView = glm::affineInverse(lightMat);
		glm::mat4 per = glm::perspective(lightData.m_outerConeAngle * 2.0f, 1.0f, .1f, 100.0f);
		pSL->mLightView = lightView;
		if (lightData.m_type == LightType_Spot)
			pSL->mLightViewProj = per * lightView;
		else if (lightData.m_type == LightType_Directional)
			pSL->mLightViewProj = ComputeDirectionalLightOrthographicMatrix(lightView) * lightView;
		//transpose
		GetXYZ(pSL->direction, glm::transpose(lightView) * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
		GetXYZ(pSL->color, lightData.m_color);
		pSL->range = lightData.m_range;
		pSL->intensity = lightData.m_intensity;
		GetXYZ(pSL->position, lightMat[3]);
		pSL->outerConeCos = cosf(lightData.m_outerConeAngle);
		pSL->innerConeCos = cosf(lightData.m_innerConeAngle);
		pSL->type = lightData.m_type;

		// Setup shadow information for light (if it has any)
		if (lightData.m_shadowResolution && lightData.m_type != LightType_Point)
		{
			pSL->shadowMapIndex = ShadowMapIndex++;
			pSL->depthBias = lightData.m_bias;
		}
		else
			pSL->shadowMapIndex = -1;
	}

	return &m_perFrameData;
}


tfNodeIdx GLTFCommon::AddNode(const tfNode& node)
{
	m_nodes.push_back(node);
	tfNodeIdx idx = (tfNodeIdx)(m_nodes.size() - 1);
	m_scenes[0].m_nodes.push_back(idx);
	auto tf = node.m_tranform;
	m_animatedMats.push_back(tf.GetWorldMat());

	return idx;
}

int GLTFCommon::AddLight(const tfNode& node, const tfLight& light)
{
	int nodeID = AddNode(node);
	m_lights.push_back(light);

	int lightInstanceID = (int)(m_lights.size() - 1);
	m_lightInstances.push_back({ lightInstanceID, (tfNodeIdx)nodeID });

	return lightInstanceID;
}

//
// Computes the orthographic matrix for a directional light in order to cover the whole scene
//
glm::mat4 GLTFCommon::ComputeDirectionalLightOrthographicMatrix(const glm::mat4& mLightView) {

	AxisAlignedBoundingBox projectedBoundingBox;

	// NOTE: we consider all objects here, but the scene might not display all of them.
	for (uint32_t i = 0; i < m_nodes.size(); ++i)
	{
		tfNode* pNode = &m_nodes.at(i);
		if ((pNode == NULL) || (pNode->meshIndex < 0))
			continue;

		std::vector<tfPrimitives>& primitives = m_meshes[pNode->meshIndex].m_pPrimitives;
		// loop through primitives
		//
		for (size_t p = 0; p < primitives.size(); ++p)
		{
			tfPrimitives boundingBox = m_meshes[pNode->meshIndex].m_pPrimitives[p];
			projectedBoundingBox.Merge(GetAABBInGivenSpace(mLightView * m_worldSpaceMats[i].GetCurrent(), boundingBox.m_center, boundingBox.m_radius));
		}
	}

	if (projectedBoundingBox.HasNoVolume())
	{
		// default ortho matrix that won't work in most cases
		return glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 100.0f);
	}

	// we now have the final bounding box
	tfPrimitives finalBoundingBox;
	finalBoundingBox.m_center = 0.5f * (projectedBoundingBox.m_max + projectedBoundingBox.m_min);
	finalBoundingBox.m_radius = 0.5f * (projectedBoundingBox.m_max - projectedBoundingBox.m_min);

	finalBoundingBox.m_center.w = (1.0f);
	finalBoundingBox.m_radius.w = (0.0f);

	// we want a square aspect ratio
	float spanX = finalBoundingBox.m_radius.x;
	float spanY = finalBoundingBox.m_radius.y;
	float maxSpan = spanX > spanY ? spanX : spanY;

	// manually create the orthographic matrix
	glm::mat4 projectionMatrix = {};//glm::mat4::identity();
	projectionMatrix[0] = (glm::vec4(
		1.0f / maxSpan, 0.0f, 0.0f, 0.0f
	));
	projectionMatrix[1] = (glm::vec4(
		0.0f, 1.0f / maxSpan, 0.0f, 0.0f
	));
	projectionMatrix[2] = (glm::vec4(
		0.0f, 0.0f, -0.5f / finalBoundingBox.m_radius.z, 0.0f
	));
	projectionMatrix[3] = (glm::vec4(
		-finalBoundingBox.m_center.x / maxSpan,
		-finalBoundingBox.m_center.y / maxSpan,
		0.5f * (finalBoundingBox.m_center.z + finalBoundingBox.m_radius.z) / finalBoundingBox.m_radius.z,
		1.0f
	));
	return projectionMatrix;
}
