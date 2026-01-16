/*
In exactly one CPP file define macro VMA_IMPLEMENTATION and then include
vk_mem_alloc.h to include definitions of its internal implementation
*/
#pragma warning(push)
#pragma warning(disable:4100;disable:4127;disable:4324)
#define VMA_IMPLEMENTATION
#ifdef DEBUG
#define VMA_DEBUG_LOG(format, ...) do { \
	   printf(format, __VA_ARGS__); \
	   printf("\n"); \
   } while(false)
#endif // DEBUG 
#include <cstdio>
#include "../vk_mem_alloc.h"
#pragma warning(pop)
