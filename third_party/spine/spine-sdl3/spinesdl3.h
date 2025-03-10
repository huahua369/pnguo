/*
	2d渲染器

*/
#ifndef SPSDL3_H
#define SPSDL3_H

#include <string>
#include <vector>
#include <algorithm>

class sp_drawable
{
public:
	struct sp_obj_c {
		void* drawable = 0;
		void* atlas = 0;
		void* skeletonData = 0;
		void* animationStateData = 0;
	};
 
	void* renderer = 0;
	std::vector<sp_obj_c> drawables;
public:
	sp_drawable();
	~sp_drawable();
	void set_renderer(void* p);
	void add(const std::string& atlas, const std::string& ske, float scale = 0.5f, float defaultMix = 0.2f);
	void dispose_sp(size_t idx);
	void animationstate_set_animationbyname(size_t idx, int trackIndex, const char* animationName, int loop);
	void animationstate_add_animationbyname(size_t idx, int trackIndex, const char* animationName, int loop, float delay);
	void set_pos(size_t idx, int x, int y);
	void get_anim_name(size_t idx, std::vector<char*>* v);
	void update_draw(double deltaTime);
private:

};

#endif // !SPSDL3_H