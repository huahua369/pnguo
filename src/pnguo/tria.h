// tria.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <array>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
//#define GLM_FORCE_ALIGNED
//#define GLM_FORCE_INTRINSICS
// 定义glm启用simd
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_XYZW_ONLY
#include <glm/glm.hpp>  

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/closest_point.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/matrix_inverse.hpp> 
#include <glm/gtx/matrix_transform_2d.hpp>
#endif
// 三角形
struct mesh3_mt
{
	std::vector<glm::ivec3>	indices;	// 三角形索引
	std::vector<glm::vec3>	vertices;	// 顶点坐标
};

struct mesh_mt
{
	std::vector<uint32_t> face_size;		// 面的边数3\4、多边形
	std::vector<uint32_t> face_indice;		// 索引
	std::vector<double> vertex_coord;		// 顶点坐标 
};
// 多边形三角化，输出到3角面网络opt，自动判断洞，，pccw是否置反三角面，type 0=CDT,1=割耳法
int triangulates(std::vector<glm::vec2>* poly, int n, bool pccw, mesh_mt* opt, int type);
int triangulates(std::vector<glm::vec2>* poly, int n, bool pccw, mesh3_mt* opt, int type);
int triangulates(std::vector<glm::vec2>* poly, int n, bool pccw, std::vector<glm::vec2>* p, int type);
