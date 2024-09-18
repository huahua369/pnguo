
#include <pch1.h>
#include <vkgui/pnguo.h>
#include <vkgui/tinysdl3.h>
#include <vkgui/vkrenderer.h>
int main()
{
	auto app = new_app();
	glm::ivec2 ws = { 1280,800 };
	const char* wtitle = (char*)u8"窗口1";
	form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, 0);
	auto sdldev = form0->get_dev();		// 获取SDL渲染器的vk设备
	vkdg_cx* vkd = new_vkdg(&sdldev);	// 创建vk渲染器 
	SDL_Texture* d3tex = 0;
	if (vkd) {
		load_gltf(vkd, R"(E:\model\sharp2.glb)");// 加载gltf
		//load_gltf(vkd, R"(E:\model\hero_alice_lobby.glb)");
		//load_gltf(vkd, R"(E:\model\spaceship.glb)");
		load_gltf(vkd, R"(E:\model\helicopter_space_ship.glb)");
		load_gltf(vkd, R"(E:\app\tools\pnguo\out\bin\media\Cauldron-Media\buster_drone\busterDrone.gltf)");
		load_gltf(vkd, R"(E:\model\mclaren_f1.glb)");
		vkd->resize(800, 600);						// 设置缓冲区大小
		auto vr = vkd->get_vkimage(0);
		if (vr.vkimageptr)
		{
			auto tex = form0->new_texture(vr.size.x, vr.size.y, vr.vkimageptr, 1);// 创建SDL的bgra纹理
			if (tex)
			{
				// 添加纹理到SDL窗口渲染 
				form0->set_texture_blend(tex, (int)BlendMode_e::normal, 0);
				form0->push_texture(tex, { 0,0,vr.size.x,vr.size.y }, { 100,100,vr.size.x,vr.size.y }, 0);
				d3tex = tex;
			}
		}
	}
	form0->up_cb = [=](float delta, int* ret)
		{
			vkd->update(form0->io);
			vkd->on_render();
		};

	run_app(app, 0);
	free_app(app);
	return 0;
}
