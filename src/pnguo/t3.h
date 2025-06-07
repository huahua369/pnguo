#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef MC_STATIC
#  define MC_EXPORT extern
#else
#ifdef _WIN32
#ifdef __cplusplus
#  ifndef MC_IMPORT
#    define MC_EXPORT extern "C" __declspec(dllexport)
#    define MCPP_EXPORT extern __declspec(dllexport)
#  else
#    define MC_EXPORT extern __declspec(dllimport)
#    define MCPP_EXPORT
#  endif
#else
#  ifndef MC_IMPORT
#    define MC_EXPORT extern "C" __declspec(dllexport)
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

#ifdef __cplusplus
extern "C" {
#endif
	struct m3_t
	{
		float* v2; 
		int dim;	// 维数
		int count;	// 顶点数量
	}; 
	struct m3idx_t
	{
		float* v3;	// 顶点数据
		int *indexs;// 索引
		int count;	// 索引数量
		int vcount;	// 顶点数量
	}; 
#ifdef __cplusplus
}
#endif
// 多边形三角化，输出到3角面网络，自动判断洞，pccw是否置反三角面0和1，type 0=CDT,1=割耳法
MC_EXPORT m3_t* triangulates(float* polys, int* n, int count, int pccw, int type);
MC_EXPORT void free_m3(m3_t* p);
MC_EXPORT m3idx_t* triangulates_idx(float* polys, int* n, int count, int pccw, int type);
MC_EXPORT void free_m3idx(m3idx_t* p);

