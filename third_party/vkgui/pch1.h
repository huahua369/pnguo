#pragma once
#include <cstdint>
#include <stack>
#include <vector>
#include <list>
#include <functional>
// 定义glm启用simd
#define GLM_FORCE_INTRINSICS
//#define GLM_FORCE_XYZW_ONLY
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/closest_point.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/closest_point.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/matrix_inverse.hpp> 

#include <nlohmann/json.hpp>

#if defined( NLOHMANN_JSON_HPP) || defined(INCLUDE_NLOHMANN_JSON_HPP_)
using njson = nlohmann::json;
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

};
#endif // !GLM_OPERATOR
