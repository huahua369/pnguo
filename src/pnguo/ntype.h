#pragma once
#define NOMINMAX
#if __cplusplus >= 201703L
#include <memory>
#endif
#if (__has_include(<memory_resource>))
#include <memory_resource>
//#elif (__has_include(<experimental/memory_resource>))
//#include <experimental/memory_resource>
//#include <experimental/utility>
//#include <experimental/string>
//#include <experimental/vector>
#endif
#include <string.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>


#ifdef max
#undef max
#undef min
#endif
namespace hz {
#if (__has_include(<memory_resource>))
#include <memory_resource>
	//__cplusplus >= 201703L
#define t_vector std::pmr::vector
#define t_list std::pmr::list
#define t_set std::pmr::set
#define t_map std::pmr::map
#define t_multiset std::pmr::multiset
#define t_multimap std::pmr::multimap
#define t_unordered_map std::pmr::unordered_map
#define t_unordered_set std::pmr::unordered_set
#define t_string std::pmr::string
#if _HAS_CXX20
#define t_u8string  std::u8string
#endif
#if 1
#define _pmr_pool_nt
	template<class _Ty>
	using pmalloc_t = std::pmr::polymorphic_allocator<_Ty>;		// 指定类型内存分配
	using uspool_t = std::pmr::unsynchronized_pool_resource;	// 线程不安全
	using mbpool_t = std::pmr::monotonic_buffer_resource;		// 线程不安全，多次分配，统一释放
	using spool_t = std::pmr::synchronized_pool_resource;		// 线程安全的



	class usp_ac
	{
	public:
		uspool_t _alloc = {};				// pmr内存分配
		size_t _Align = 16;
	public:
		usp_ac() {}
		~usp_ac() {}
	public:
		void* allocate(const size_t _Bytes, const size_t align = 0) {
			return  _alloc.allocate(_Bytes, align > 0 ? align : _Align);
		}
		void* new_mem(size_t n)
		{
			n = std::max((size_t)1, n);
			auto p = _alloc.allocate(n, _Align);
			memset(p, 0, n);
			return p;
		}
		void* new_mem0(size_t n)
		{
			n = std::max((size_t)1, n);
			auto p = _alloc.allocate(n, _Align);
			return p;
		}
		template<class T>
		T* new_mem(size_t n)
		{
			n = std::max((size_t)1, n);
			auto p = (T*)_alloc.allocate(sizeof(T) * n, _Align);
			auto ptr = p;
			for (int i = 0; i < n; i++)
			{
				p[i] = {};
			}
			return p;
		}
		template<class T >
		T* new_mem(size_t n, T*& p)
		{
			n = std::max((size_t)1, n);
			p = (T*)_alloc.allocate(sizeof(T) * n, _Align);
			auto ptr = p;
			for (int i = 0; i < n; i++)
			{
				p[i] = {};
			}
			return p;
		}
		template<class T >
		T* new_mem_o(size_t n)
		{
			n = std::max((size_t)1, n);
			auto p = (T*)_alloc.allocate(sizeof(T) * n, _Align);
			return p;
		}
		template<class T>
		T* new_mem(T*& p, size_t n)
		{
			return new_mem(n, p);
		}
		template<class T>
		void free_mem(T* t, size_t n)
		{
			auto ptr = t;
			if (t && n > 0)
			{
				_alloc.deallocate(t, sizeof(T) * n, _Align);
			}
		}
		void free_mem0(void* t, size_t n)
		{
			auto ptr = t;
			if (t && n > 0)
			{
				_alloc.deallocate(t, n, _Align);
			}
		}
		template<class T, class... Ts>
		T* new_obj(Ts &&... args)
		{
			auto p = (T*)new_mem(sizeof(T));
			if (p)
			{
#ifdef _WIN32
				std::uninitialized_construct_using_allocator(p, _alloc, std::forward<Ts>(args)...);
#else
				std::__uninitialized_construct_using_allocator(p, _alloc, std::forward<Ts>(args)...);
#endif
			}
			return p;
		}
		template<class T>
		void free_obj(T* t)
		{
			auto ptr = t;
			if (t)
			{
				std::destroy_at(ptr);
				_alloc.deallocate(t, sizeof(T), _Align);
			}
		}
	};

#endif
#else
#define _NO_PMR_
#define t_vector std::vector
#define t_list std::list
#define t_set std::set
#define t_map std::map
#define t_multiset std::multiset
#define t_multimap std::multimap
#define t_unordered_map std::unordered_map
#define t_unordered_set std::unordered_set
#define t_string  std::string
#if _HAS_CXX20
#define t_u8string  std::string
#endif

	class usp_ac
	{
	public:
		t_set<char*> _ac;
		size_t _Align = 16;
	public:
		usp_ac() {}
		~usp_ac() {
			for (auto it : _ac)
			{
				if (it)free(it);
			}
		}
	public:
		void* new_mem(size_t n)
		{
			n = std::max((size_t)1, n);
			n = alignUp(n, _Align);
			void* p = malloc(n);
			if (p)
			{
				_ac.insert((char*)p);
				memset(p, 0, n);
			}
			return p;
		}
		template<class T>
		T* new_mem(size_t n)
		{
			n = std::max((size_t)1, n);
			auto n1 = alignUp(sizeof(T) * n, _Align);
			auto p = (T*)malloc(n1);
			if (p)
			{
				_ac.insert((char*)p);
				for (int i = 0; i < n; i++)
					p[i] = {};
			}
			return p;
		}
		template<class T>
		T* new_mem(size_t n, T*& p)
		{
			n = std::max((size_t)1, n);
			auto n1 = alignUp(sizeof(T) * n, _Align);
			p = (T*)malloc(n1);
			if (p)
			{
				_ac.insert((char*)p);
				for (int i = 0; i < n; i++)
					p[i] = {};
			}
			return p;
		}

		template<class T>
		T* new_mem(T*& p, size_t n)
		{
			return new_mem(n, p);
		}
		template<class T>
		void free_mem(T* t, size_t n)
		{
			if (t && n > 0)
			{
				_ac.erase((char*)t);
				free(t);
			}
		}
		template<class T, class... Ts>
		T* new_obj(Ts &&... args)
		{
			auto p = (T*)new_mem(sizeof(T));
			if (p)
			{
				void* p1 = p;
				p = new(p1) T(args...);
				//std::uninitialized_construct_using_allocator(p, _alloc, std::forward<Ts>(args)...);
			}
			return p;
		}
		template<class T>
		void free_obj(T* t)
		{
			auto ptr = t;
			if (t)
			{
				std::destroy_at(ptr);
				_ac.erase((char*)t);
				free(t);
			}
		}

		template<typename T> inline T alignUp(const T& val, T alignment)
		{
			T r = (val + alignment - (T)1) & ~(alignment - (T)1);
			//val = r;
			return r;
		}
	};


#endif // _HAS_CXX17

#define USP_CX hz::usp_ac

	// todu 一次清空对象自动调用析构函数
#ifndef AUTO_DC
#define AUTO_DC
#endif
	class auto_destroy_cx
	{
	private:
		typedef void (*destructor_cb) (void* p);
		struct obj_t
		{
			void* p = 0;
			destructor_cb cb = 0;
		};
		t_vector<obj_t> _gc;// , _gcd, _gcf;
	public:
		auto_destroy_cx() {}
		~auto_destroy_cx() {
			clear();
		}
	public:
		// 创建对象
		template <class _T, typename... Args>
		_T* ac(Args... args)
		{
			_T* p = new _T(args...);
			_gc.push_back({ p, (destructor_cb)&gc_delete<_T> });
			return p;
		}
		template <class _T, typename... Args>
		_T* acp(_T*& p, Args... args)
		{
			p = new _T(args...);
			_gc.push_back({ p, (destructor_cb)&gc_delete<_T> });
			return p;
		}
		// 从内存m上创建对象
		template <class _T, typename... Args>
		_T* mc(void* m, Args... args)
		{
			_T* p = new(m) _T(args...);
			_gc.push_back({ p, (destructor_cb)&gc_free<_T> });
			return p;
		}
		// 添加对象
		template <class _T>
		void push(_T* p)
		{
			if (p)
				_gc.push_back({ p, (destructor_cb)&gc_delete<_T> });
		}
		// 添加对象
		template <class _T>
		void pushf(_T* p)
		{
			if (p)
				_gc.push_back({ p, (destructor_cb)&gc_free<_T> });
		}
		// 添加对象
		template <class _T>
		void push_cf(_T* p)
		{
			if (p)
				_gc.push_back({ p, (destructor_cb)&gc_cfree<_T> });
		}
		template <class _T>
		static void gc_free(_T* p)
		{
			if (p)
				p->~_T();
		}
		template <class _T>
		static void gc_delete(_T* p)
		{
			if (p)
				delete p;
		}
		template <class _T>
		static void gc_cfree(_T* p)
		{
			if (p)
				::free(p);
		}
		void clear()
		{
			for (auto it = _gc.rbegin(); it != _gc.rend(); it++)
				it->cb(it->p);
			_gc.clear();
			//for (auto it = _gcd.rbegin(); it != _gcd.rend(); it++)
			//	it->cb(it->p);
			//_gcd.clear();
			//for (auto it = _gcf.rbegin(); it != _gcf.rend(); it++)
			//	it->cb(it->p);
			//_gcf.clear();
		}
		void pop(void* p)
		{
			int t = 0;
			if (t >= 0 && t < 3 && p)
			{
				t_vector<obj_t>* vo[3] = { &_gc };
				for (auto it = vo[t]->begin(); it != vo[t]->end(); )
				{
					if (it->p == p)
					{
						auto cb = it->cb;
						cb(p);
						vo[t]->erase(it++);
						break;
					}
					else {
						it++;
					}
				}
			}

		}
	};

	template <class _T>
	_T* delop(_T*& p, bool isa = false)
	{
		if (p) {
			if (isa)
			{
				delete[]p;
			}
			else {
				delete p;
			}
		}
		p = 0;
		return p;
	}
}
//!hz

// 友元函数，比较int和对象的大小
// 创建枚举自动转换类DECALRE_RUNTIME_ENUM
#define DR_ENUM(clsName) \
	class clsName##_t\
	{\
	public:\
		clsName e={};\
	public:\
		clsName##_t(){}\
		clsName##_t(clsName t) :e(t) { }\
		clsName##_t(std::underlying_type<clsName>::type t) : e((clsName)t)	{	}\
		~clsName##_t()	{	}\
		bool operator<(const clsName##_t &t1){return e<t1.e;}\
		bool operator<(const clsName &t1){return e<t1;}\
		bool operator<(int t1){return (int)e<t1;}\
		bool operator>(int t1){return (int)e>t1;}\
		friend inline bool operator<(const clsName##_t &a,const clsName##_t &t1){return a.e<t1.e;}\
		bool operator>(const clsName##_t &t1){return e>t1.e;}\
		bool operator>(const clsName &t1){return e>t1;}\
		bool operator==(const clsName##_t &t1){return e==t1.e;}\
		bool operator==(const clsName &t1){return e==t1;}\
		bool operator!=(const clsName##_t &t1){return e!=t1.e;}\
		bool operator!=(const clsName &t1){return e!=t1;}\
		clsName##_t operator|=(const clsName &t1){return ((int)e|(int)t1);}\
		clsName##_t operator&=(const clsName &t1){return (clsName##_t)((int)e&(int)t1);}\
		clsName##_t operator&=(int t1){return (clsName##_t)((int)e&t1);}\
		bool operator&( clsName &t1){return static_cast<std::underlying_type<clsName>::type>(e)&static_cast<std::underlying_type<clsName>::type>(t1);}\
		bool operator&( clsName##_t &t1){return static_cast<std::underlying_type<clsName>::type>(e)&static_cast<std::underlying_type<clsName>::type>(t1.e);}\
		bool operator&(const clsName##_t &t1){return static_cast<std::underlying_type<clsName>::type>(e)&static_cast<std::underlying_type<clsName>::type>(t1.e);}\
		operator clsName (){return e;}\
		operator std::underlying_type<clsName>::type() { return static_cast<std::underlying_type<clsName>::type>(e); }\
	}

		//operator int (){return (int)e;}\
