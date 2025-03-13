#pragma once

#ifdef MC_STATIC
#  define MC_EXPORT extern
#else
#ifdef _WIN32
#ifdef __cplusplus
#  ifndef MC_IMPORT
#    define MC_EXPORT extern "C" __declspec(dllexport)
#    define MCPP_EXPORT extern __declspec(dllexport)
#  else
#    define MC_EXPORT extern "C" __declspec(dllimport)
#    define MCPP_EXPORT __declspec(dllimport)
#  endif
#else
#  ifndef MC_IMPORT
#    define MC_EXPORT extern __declspec(dllexport)
#  else
#    define MC_EXPORT extern __declspec(dllimport)
#  endif
#endif
#else
#ifdef __cplusplus
#define MC_EXPORT extern "C" __attribute__((visibility ("default")))
#define MCPP_EXPORT __attribute__((visibility ("default")))
#else
#define MC_EXPORT extern __attribute__((visibility ("default")))
#define MCPP_EXPORT 
#endif
#endif // _WIN32
#endif

#ifndef MC_API
#ifdef _WIN32
#define MC_API __cdecl
#else
#define MC_API __cdecl
#endif
#endif

#include <cstdint>
#include <stdint.h>
#include <stdbool.h>
 
#include <map>
#include <set> 
#include <string> 
#include <stack>
#include <vector>
#include <list>
#include <queue>
#include <functional>
#include <thread>
#include <mutex> 
#include <random> 
#include <algorithm>
#include <array>
#include <fstream> 
#include <memory>
#include <numeric> // iota
#include <stdio.h>
#include <string.h>
#include <unordered_map>
 
#define GLM_ENABLE_EXPERIMENTAL
//#define GLM_FORCE_ALIGNED
//#define GLM_FORCE_INTRINSICS
// 定义glm启用simd
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
//#define GLM_FORCE_XYZW_ONLY
#include <glm/glm.hpp>  

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/closest_point.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/matrix_inverse.hpp> 
#include <glm/gtx/matrix_transform_2d.hpp>

#include <nlohmann/json.hpp>
#if defined( NLOHMANN_JSON_HPP) || defined(INCLUDE_NLOHMANN_JSON_HPP_)
using njson = nlohmann::json;			// key有序
using njson0 = nlohmann::ordered_json;	// key无序
#define NJSON_H
#endif

#include <exprtk.hpp>

#if 0
#include <mapView.h>
//#include <curve.h>
#include <file.h>
#include <mem_pe.h>
#include <base_util.h>
#include <ecc_sv.h>
#include <print_time.h>

#ifndef ENTT_ID_TYPE
#   include <cstdint>
#   define ENTT_ID_TYPE size_t
#endif
#include <entt.hpp>
#endif


#ifndef GLM_OPERATOR

namespace glm
{
#define GLM_OPERATOR
	//vec
	static bool operator<(glm::ivec4 const& v1, glm::ivec4 const& v2)
	{
		std::array<int, 4> va[2] = { {v1.x,v1.y,v1.z,v1.w}, {v2.x,v2.y,v2.z,v2.w} };
		bool ret = va[0] < va[1];
		return ret;
	}
	static bool operator<(glm::vec4 const& v1, glm::vec4 const& v2)
	{
		std::array<double, 4> va[2] = { {v1.x,v1.y,v1.z,v1.w}, {v2.x,v2.y,v2.z,v2.w} };
		bool ret = va[0] < va[1];
		return ret;
	}
	static bool operator<(glm::vec3 const& v1, glm::vec3 const& v2)
	{
		bool yr = (v1.z < v2.z) || (v1.y < v2.y || (v1.y == v2.y && v1.x < v2.x));
		std::array<double, 3> va[2] = { {v1.x,v1.y,v1.z}, {v2.x,v2.y,v2.z} };
		bool ret = va[0] < va[1];
		return ret;
	}
	static bool operator<(glm::vec2 const& v1, glm::vec2 const& v2)
	{
		bool yr = (v1.y < v2.y || (v1.y == v2.y && v1.x < v2.x));
		std::array<double, 2> va[2] = { {v1.x,v1.y},{v2.x,v2.y} };
		bool ret = va[0] < va[1];
		return ret;
	}
	static bool operator>(glm::vec3 const& v1, glm::vec3 const& v2)
	{
		bool yr = (v1.z > v2.z) || (v1.y > v2.y || (v1.y == v2.y && v1.x > v2.x));
		std::array<double, 3> va[2] = { {v1.x,v1.y,v1.z},{v2.x,v2.y,v2.z} };
		bool ret = va[1] < va[0];
		return ret;
	}
	static bool operator>(glm::vec2 const& v1, glm::vec2 const& v2)
	{
		bool yr = (v1.y > v2.y || (v1.y == v2.y && v1.x > v2.x));
		std::array<double, 3> va[2] = { {v1.x,v1.y},{v2.x,v2.y} };
		bool ret = va[1] < va[0];
		return ret;
	}
	/*
		bool operator>=(glm::vec3 const & v1, glm::vec3 const & v2)
		{
			bool yr = (v1.z >= v2.z) || (v1.y >= v2.y || (v1.y == v2.y&& v1.x >= v2.x));
			std::array<double, 3> va[2] = { {v1.x,v1.y,v1.z},{v2.x,v2.y,v2.z} };
			bool ret = va[0] < va[1];
			return ret;
		}
		bool operator>=(glm::vec2 const & v1, glm::vec2 const & v2)
		{
			bool yr = (v1.y >= v2.y || (v1.y == v2.y&& v1.x >= v2.x));
			std::array<double, 3> va[2] = { {v1.x,v1.y,v1.z},{v2.x,v2.y,v2.z} };
			bool ret = va[0] < va[1];
			return ret;
		}*/
		//uvec
	static bool operator<(glm::uvec3 const& v1, glm::uvec3 const& v2)
	{
		return (v1.z < v2.z) || (v1.y < v2.y || (v1.y == v2.y && v1.x < v2.x));
	}
	static bool operator<(glm::uvec2 const& v1, glm::uvec2 const& v2)
	{
		return (v1.y < v2.y || (v1.y == v2.y && v1.x < v2.x));
	}
	static bool operator>(glm::uvec3 const& v1, glm::uvec3 const& v2)
	{
		return (v1.z > v2.z) || (v1.y > v2.y || (v1.y == v2.y && v1.x > v2.x));
	}
	static bool operator>(glm::uvec2 const& v1, glm::uvec2 const& v2)
	{
		return (v1.y > v2.y || (v1.y == v2.y && v1.x > v2.x));
	}
	static bool operator>=(glm::uvec3 const& v1, glm::uvec3 const& v2)
	{
		return (v1.z >= v2.z) || (v1.y >= v2.y || (v1.y == v2.y && v1.x >= v2.x));
	}
	static bool operator>=(glm::uvec2 const& v1, glm::uvec2 const& v2)
	{
		return (v1.y >= v2.y || (v1.y == v2.y && v1.x >= v2.x));
	}
	//ivec
	static bool operator<(glm::ivec3 const& v1, glm::ivec3 const& v2)
	{
		return (v1.z < v2.z) || (v1.y < v2.y || (v1.y == v2.y && v1.x < v2.x));
	}
	static bool operator<(glm::ivec2 const& v1, glm::ivec2 const& v2)
	{
		return (v1.y < v2.y || (v1.y == v2.y && v1.x < v2.x));
	}
	static bool operator>(glm::ivec3 const& v1, glm::ivec3 const& v2)
	{
		return (v1.z > v2.z) || (v1.y > v2.y || (v1.y == v2.y && v1.x > v2.x));
	}
	static bool operator>(glm::ivec2 const& v1, glm::ivec2 const& v2)
	{
		return (v1.y > v2.y || (v1.y == v2.y && v1.x > v2.x));
	}
	static bool operator>=(glm::ivec3 const& v1, glm::ivec3 const& v2)
	{
		return (v1.z >= v2.z) || (v1.y >= v2.y || (v1.y == v2.y && v1.x >= v2.x));
	}
	static bool operator>=(glm::ivec2 const& v1, glm::ivec2 const& v2)
	{
		return (v1.y >= v2.y || (v1.y == v2.y && v1.x >= v2.x));
	}

	static glm::vec2 operator*(glm::vec2 const& v1, float v2)
	{
		return { v1.x * v2,v1.y * v2 };
	}
	static glm::vec3 operator*(glm::vec3 const& v1, float v2)
	{
		return { v1.x * v2, v1.y * v2, v1.z * v2 };
	}
	static glm::vec4 operator*(glm::vec4 const& v1, float v2)
	{
		return { v1.x * v2, v1.y * v2, v1.z * v2, v1.w * v2 };
	}

};

namespace gp {
	uint64_t toUInt(const njson& v, uint64_t de = 0);
	int64_t toInt(const njson& v, const char* k, int64_t de);
	int64_t toInt(const njson& v, int64_t de = 0);
	double toDouble(const njson& v, double de = 0);
	std::string toStr(const njson& v, const char* k, const std::string& des = "");
	std::string toStr(const njson& v, const std::string& des = "");
	int64_t str2int(const char* str, int64_t de = 0);
	njson str2ints(const std::string& s);

	std::vector<float> get_vs(njson& n, const char* k);
	std::vector<glm::vec2> get_v2(njson& n, const char* k);
	std::vector<glm::ivec2> get_iv2(njson& n, const char* k);
	std::vector<glm::vec3> get_v3(njson& n, const char* k);
	std::vector<glm::vec4> get_v4(njson& n, const char* k);
	std::vector<glm::ivec4> get_iv4(njson& n, const char* k);
}

#endif // !GLM_OPERATOR
