﻿/*
	2d渲染器
	{
		"atlas":[数据索引],
		"ske":[数据索引],
		"drawable":[{"atlas","ske",scale,mix}],
		"data_view":[{"offset","length","type可选0二进制1字符串"}]
	}
*/
#ifndef SPSDL3_H
#define SPSDL3_H

#include <string>
#include <vector>
#include <map>
#include <algorithm>

class sp_drawable
{
public:
	struct sp_obj_c {
		void* drawable = 0;
		void* atlas = 0;
		bool visible = true;
	};
	void* renderer = 0;
	std::vector<sp_obj_c> drawables;
	struct able_t
	{
		std::string atlas;
		std::string ske;
		float scale = 0.5f;
		float defaultMix = 0.2f;
	};
	std::vector<able_t> vable;
public:
	sp_drawable();
	~sp_drawable();
	void set_renderer(void* p);
	// package_file可用于输出到打包
	void add(const std::string& atlas, const std::string& ske, float scale = 0.5f, float defaultMix = 0.2f, const std::string& package_file = "");
	// 从打包文件加载
	void add_pkg_data(const char* data, size_t len, float scale = 0.5f, float defaultMix = 0.2f);
	void add_pkg(const std::string& pkgfn, float scale = 0.5f, float defaultMix = 0.2f);
	void dispose_sp(size_t idx);
	void animationstate_set_animationbyname(size_t idx, int trackIndex, const char* animationName, int loop);
	void animationstate_add_animationbyname(size_t idx, int trackIndex, const char* animationName, int loop, float delay);
	void set_pos(size_t idx, int x, int y);
	void get_anim_name(size_t idx, std::vector<char*>* v);
	void update_draw(double deltaTime);

private:

};

class drawable2d_cx
{
public:
	struct obj_c {
		void* drawable = 0;
		void* skeletonData = 0;
		void* animationStateData = 0;
		bool visible = true;
	};
	void* atlas = 0;
	void* renderer = 0;
	std::vector<obj_c> vable;
public:
	drawable2d_cx();
	~drawable2d_cx();

private:

};



#endif // !SPSDL3_H