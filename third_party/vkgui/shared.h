#ifndef __shared__H__
#define __shared__H__
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

#ifndef MC_API
#ifdef _WIN32
#define MC_API __cdecl
#else
#define MC_API __cdecl
#endif
#endif //!MC_API
#ifdef SHD_NH
#include "palloc/palloc.h"


#ifdef __cplusplus
extern "C" {
#endif
	typedef bool(*RETF)(const char* data, uint64_t len, void* ud, palloc_nt* a);
	typedef struct RTFct_u
	{
		const char* path = nullptr;
		const char* pathinfo = nullptr;
		void* request_data = nullptr;
		uint64_t len = 0;
		RETF fun = nullptr;
		void* ud = nullptr;
		void* ud1 = nullptr;
		palloc_nt* alloc = nullptr;

		RTFct_u() {}
		RTFct_u(const char* path_, const char* pathinfo_, void* request_data_, uint64_t len_,
			RETF fun_, void* ud_, void* ud1_)
			: path(path_),
			pathinfo(pathinfo_),
			request_data(request_data_),
			len(len_),
			fun(fun_),
			ud(ud_),
			ud1(ud1_)
		{}
	}RTFct;

	typedef bool(*RTF)(RTFct* rct);

#ifndef _SQL_INFO_
#define _SQL_INFO_
	typedef struct sql_db_info_s sql_db_info_t;
	typedef struct sql_cont_info_s sql_cont_info_t;
	typedef struct dev_sql_s dev_sql_t;

	typedef struct thread_pool_s thread_pool_t;
	typedef struct pstr_s pstr_t;
	typedef struct pjson_s pjson_t;

#endif

	typedef struct module_s
	{
		// 模块句柄
		void* ctx;
		// 模块名(唯一)，关系到替换更新模块文件
		const char* name;
		// 路径头，分号分隔
		const char* ptop;
		void* user_data;
		// 加载模块，如为空则call_path(ctx==nullptr
		void* (*load)();
		void(*unload)(void* ctx);
		// 设置数据库操作，初始化时执行，可选
		void(*set_info)(void* ctx, dev_sql_t* db);
		// 必需实现
		bool (*call_path)(void* ctx, RTFct* rct);
	}module_t;


#ifdef __cplusplus
}
#endif
#endif
#endif //!__shared__H__
