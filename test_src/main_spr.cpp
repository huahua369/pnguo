
#include <pch1.h>

#include <random>
#include <vkgui/win_core.h>
#include "win32msg.h"
#include <vkgui/event.h>
#include <vkgui/pnguo.h>
#include <vkgui/tinysdl3.h>
#include <vkgui/vkrenderer.h>
#include <vkgui/page.h>
#include <vkgui/mapView.h>

#include "mshell.h"


auto fontn = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman,Malgun Gothic";

void menu_m(form_x* form0, std::function<void(int idx)> cb)
{
	auto mainmenu = new plane_cx();
	form0->bind(mainmenu, 1);	// 绑定主菜单到窗口
	auto p = mainmenu;
	//p->set_rss(5);
	p->add_familys(fontn, 0);
	//p->draggable = true; //可拖动✔
	p->set_color({ 0,1,0,0xff000000 });
	// 主菜单
	std::vector<std::string> mvs = { (char*)u8"文件",(char*)u8"编辑",(char*)u8"视图",(char*)u8"工具",(char*)u8"帮助" };
	p->fontsize = 16;

	{
		glm::ivec2  fs = form0->get_size();
		if (fs.x & 1)
			fs.x++;
		if (fs.y & 1)
			fs.y++;
		p->set_size({ fs.x, p->fontsize * 2 });
	}
	p->set_pos({});
	glm::vec2 cs = { 1500,1600 };
	auto vs = p->get_size();

	p->set_view(vs, cs);
	p->update_cb = [=](float dt)
		{
			bool r = false;
			if (form0)
			{
				glm::ivec2 ps = p->get_size(), fs = form0->get_size();
				if (fs.x & 1)
					fs.x++;
				if (fs.y & 1)
					fs.y++;
				if (ps.x != fs.x)
				{
					p->set_size({ fs.x, p->fontsize * 2 });
					r = true;
				}
			}
			return r;
		};

	menu_cx* mc = new menu_cx();	// 菜单管理
	mc->set_main(form0);
	mc->add_familys(fontn);

	std::vector<std::string> mvs0 = { (char*)u8"🍇打开(O)",(char*)u8"🍑菜单",(char*)u8"🍍菜单1" };
	std::vector<std::string> mvs1 = { (char*)u8"🍇子菜单",(char*)u8"🍑菜单2",(char*)u8"🍍菜单12" };
	int cidx = 1;
	// 创建菜单
	auto pm31 = mc->new_menu(-1, 30, mvs1, [=](mitem_t* pm, int type, int idx)
		{
			if (type == 1)
			{
				printf("click:%d\t%d\n", type, idx);
			}
			//else
			//	printf("move:%d\t%d\n", type, idx);
		});
	auto pm3 = mc->new_menu(-1, 30, mvs0, [=](mitem_t* pm, int type, int idx)
		{
			if (type == 1)
			{
				if (cb)cb(idx);
				printf("click:%d\t%d\n", type, idx);
			}
			else
			{
				//printf("move:%d\t%d\n", type, idx);
			}
		});
	pm3->set_child(pm31, 2);
	//pm3->show({ 100,100 }); // 显示菜单

	for (auto& it : mvs)
	{
		auto cbt = p->add_cbutton(it.c_str(), { 60,26 }, (int)uType::info);
		cbt->effect = uTheme::light;
		cbt->hscroll = {};
		cbt->rounding = 0;
		cbt->light = 0.1;

		cbt->click_cb = [=](void* ptr, int clicks)
			{
				printf("%s\n", cbt->str.c_str());
			};
		cbt->mevent_cb = [=](void* pt, int type, const glm::vec2& mps)
			{
				static void* enterst = 0;
				auto cp = (color_btn*)pt;
				auto t = (event_type2)type;
				switch (t)
				{
				case event_type2::on_down:
				{
					auto cps = cp->get_pos();
					cps.y += cp->size.y + cp->thickness;
					pm3->hide(true);
					pm3->show(cps);
					hide_tooltip(form0);
					form0->uptr = 0;
				}
				break;
				case event_type2::on_enter:
				{
					enterst = pt;
					if (pm3->get_visible()) {
						auto cps = cp->get_pos();
						cps.y += cp->size.y + cp->thickness;
						pm3->hide(true);
						pm3->show(cps);
					}
				}
				break;
				case event_type2::on_hover:
				{
					// 0.5秒触发悬停事件
					style_tooltip stp = {};
					stp.family = fontn;
					stp.fonst_size = 14;
					glm::vec2 cps = mps;
					cps.y += 20;
					if (enterst == pt) {
						if (form0->uptr != pt)
						{
							//show_tooltip(form0, (char*)u8"提示信息！", cps, &stp);
							form0->uptr = pt;
						}
					}
				}
				break;
				case event_type2::on_leave:
				{
					if (enterst == pt) {
						hide_tooltip(form0);
						form0->uptr = 0;
					}
				}
				break;
				default:
					break;
				}
			};
	}
}
struct spr_info_t
{
	char tag[4] = {};
	uint32_t version;
	uint32_t filesize;
	uint32_t hash;
	char id[2];
	uint16_t dir;
	uint16_t frame;
	uint16_t width;
	uint16_t height;
	uint16_t kx;
	uint16_t ky;
	uint16_t image_res_nums;
};
struct frames_t
{
	int16_t imageidx, pos_x, pos_y, width, height, key_x, key_y;
};
struct header_t
{
	frames_t* frames;
	int sprtype;
};
image_ptr_t* new_image2spr(const std::string& fn) {
	image_ptr_t* r = 0;
	hz::mfile_t m;
	auto pd = m.open_d(fn, 0);
	if (pd)
	{
		auto sp = (spr_info_t*)pd;
		std::vector<frames_t> f;
		header_t h = {};
		std::vector<int> irn;
		int c = sp->frame * sp->dir;
		auto fp = pd + sizeof(spr_info_t);
		auto fpp = (frames_t*)fp;
		h.frames = fpp;
		for (size_t i = 0; i < c; i++)
		{
			f.push_back(*fpp); fpp++;
		}
		fp += sizeof(frames_t) * c;
		h.sprtype = *(int*)fp;
		fp += sizeof(int);
		int* ip = (int*)fp;
		for (size_t i = 0; i < sp->image_res_nums; i++)
		{
			irn.push_back(*ip); ip++;
		}
		fp += sp->image_res_nums * 4;
		auto kd = fp - pd;

		char pngd[4] = { 0x89,0x50,0x4e,0x47 };
		int* pngi = (int*)pngd;
		auto length = m.size() / 4;
		char* d = pd + 16 + irn[0];
		size_t ls = irn.size() == 1 ? sp->filesize - irn[0] : irn[1]; //24439
		if (d)
			r = stbimage_load::new_load(d, ls);
	}
	return r;
}
image_ptr_t* new_image2spr(char* pd, size_t fs) {
	image_ptr_t* r = 0;
	if (pd)
	{
		auto sp = (spr_info_t*)pd;
		std::vector<frames_t> f;
		header_t h = {};
		std::vector<int> irn;
		int c = sp->frame * sp->dir;
		auto fp = pd + sizeof(spr_info_t);
		auto fpp = (frames_t*)fp;
		h.frames = fpp;
		for (size_t i = 0; i < c; i++)
		{
			f.push_back(*fpp); fpp++;
		}
		fp += sizeof(frames_t) * c;
		h.sprtype = *(int*)fp;
		fp += sizeof(int);
		int* ip = (int*)fp;
		for (size_t i = 0; i < sp->image_res_nums; i++)
		{
			irn.push_back(*ip); ip++;
		}
		fp += sp->image_res_nums * 4;
		auto kd = fp - pd;

		char pngd[4] = { 0x89,0x50,0x4e,0x47 };
		int* pngi = (int*)pngd;
		auto length = fs / 4;
		char* d = pd + 16 + irn[0];
		size_t ls = irn.size() == 1 ? sp->filesize - irn[0] : irn[1]; //24439
		if (d)
			r = stbimage_load::new_load(d, ls);
	}
	return r;
}
void load_spr(form_x* form0, const std::string& fn) {
	auto img = new_image2spr(fn);
	if (img)
	{
		auto tex = form0->new_texture(img->width, img->height, 0, img->data, img->stride);
		if (tex)
			form0->push_texture(tex, { 0,0,img->width,img->height }, { 10,100,img->width,img->height }, 1);
	}
}

#include <curl/curl.h>

class mcurl_cx
{
public:
	struct tNode
	{
		char* fpd;
		size_t startPos;
		size_t endPos;
		void* curl;
		int tid;
	};
	std::vector<std::thread> tv;
	std::atomic_int cnt = 0;
	std::vector<char> data;
public:
	mcurl_cx();
	~mcurl_cx();

	bool downLoad(int threadNum, const std::string& Url);
private:
	int64_t getDownloadFileLenth(const char* url);

};

mcurl_cx::mcurl_cx()
{
}

mcurl_cx::~mcurl_cx()
{
}


static size_t writeFunc(void* ptr, size_t size, size_t nmemb, void* userdata)
{
	mcurl_cx::tNode* node = (mcurl_cx::tNode*)userdata;
	size_t written = 0;
	if (node->startPos + size * nmemb <= node->endPos)
	{
		auto p = node->fpd + node->startPos;
		memcpy(p, ptr, size * nmemb);
		node->startPos += size * nmemb;
		written = nmemb;
	}
	else
	{
		auto p = node->fpd + node->startPos;
		memcpy(p, ptr, node->endPos - node->startPos + 1);
		node->startPos = node->endPos;
		written = nmemb;
	}
	return written;
}

int progressFunc(void* ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded)
{
	mcurl_cx* p = (mcurl_cx*)ptr;
	int percent = 0;
	if (totalToDownload > 0)
	{
		percent = (int)(nowDownloaded / totalToDownload * 100);
	}

	if (p && percent > p->cnt)
	{
		p->cnt = percent;
		//printf((char*)u8"下载进度%0d%%\n", percent);
	}
	return 0;
}

/************************************************************************/
/* 获取要下载的远程文件的大小 											*/
/************************************************************************/
int64_t mcurl_cx::getDownloadFileLenth(const char* url)
{
	double downloadFileLenth = 0;
	CURL* handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_HEADER, 1);	//只需要header头
	curl_easy_setopt(handle, CURLOPT_NOBODY, 1);	//不需要body
	if (curl_easy_perform(handle) == CURLE_OK)
	{
		curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &downloadFileLenth);
	}
	else
	{
		downloadFileLenth = -1;
	}
	return downloadFileLenth;
}


bool mcurl_cx::downLoad(int threadNum, const std::string& Url)
{
	auto fileLength = getDownloadFileLenth(Url.c_str());

	if (fileLength <= 0)
	{
		printf("get the file length error...");
		return false;
	}
	data.resize(fileLength);

	long partSize = fileLength / threadNum;

	for (int i = 0; i <= threadNum; i++)
	{
		tNode* pNode = new tNode();

		if (i < threadNum)
		{
			pNode->startPos = i * partSize;
			pNode->endPos = (i + 1) * partSize - 1;
		}
		else
		{
			if (fileLength % threadNum != 0)
			{
				pNode->startPos = i * partSize;
				pNode->endPos = fileLength - 1;
			}
			else
				break;
		}

		CURL* curl = curl_easy_init();

		pNode->curl = curl;
		pNode->fpd = data.data();

		char range[64] = { 0 };
		snprintf(range, sizeof(range), "%lld-%lld", pNode->startPos, pNode->endPos);

		// Download pacakge
		curl_easy_setopt(curl, CURLOPT_URL, Url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)pNode);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressFunc);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 5L);
		curl_easy_setopt(curl, CURLOPT_RANGE, range);

		std::thread a([=]() {
			if (pNode)
			{
				int res = curl_easy_perform(pNode->curl);
				if (res != 0)
				{

				}
				curl_easy_cleanup(pNode->curl);
				printf("thred %ld exit\n", pNode->tid);
				delete pNode;
			}
			});
		tv.push_back(std::move(a));
	}

	for (auto& it : tv) {
		it.join();
	}

	printf("download succed......\n");
	return true;
}

void dw_get(const char* url) {
	CURL* curl;
	CURLcode res;
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
		struct curl_slist* headers = NULL;
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		res = curl_easy_perform(curl);
	}
	curl_easy_cleanup(curl);
}

image_ptr_t* spr2png(const std::string& url, const std::string& fn)
{
	image_ptr_t* p = 0;
	char pngd[4] = { 0x89,0x50,0x4e,0x47 };
	int pngi = *(int*)pngd;
	{
		mcurl_cx fbg;
		fbg.downLoad(1, url);
		//if (*(int*)fbg.data.data() == pngi)
		{
			p = new_image2spr(fbg.data.data(), fbg.data.size());
			if (fn.size() && p)
			{
				hz::mfile_t m;
				auto pd = m.new_m(fn, fbg.data.size());
				if (pd) {
					memcpy(pd, fbg.data.data(), fbg.data.size());
					m.flush();
				}
				auto fnn = fn + ".png";
				save_img_png(p, fnn.c_str());
			}
		}
	}
	return p;
}
struct imgdepth_t
{
	image_ptr_t* dst = 0;
	std::vector<uint16_t> imgdepth;
	int width, height;
	int dx, dy;
};
imgdepth_t pal_img(const std::string& palfn, const std::string& fn, const std::string& ofn, const std::string& gray = "", imgdepth_t* tds = 0) {

	auto palp = stbimage_load::new_load(palfn.c_str(), 0);
	auto img = stbimage_load::new_load(fn.c_str(), 0);
	auto img_gray = stbimage_load::new_load(gray.c_str(), 0);
	std::vector<uint32_t> imgdata;
	imgdepth_t ret = {};
	imgdata.resize(img->width * img->height);
	memcpy(imgdata.data(), img->data, img->width * img->height * 4);
	auto pal = (uint32_t*)palp->data;
	std::set<int> as;
	size_t i = 0;
	auto gd = img_gray ? img_gray->data : nullptr;
	auto td = tds ? tds->imgdepth.data() : nullptr;
	int ic = 0, y = 0;
	uint32_t* ot = 0;
	if (tds && tds->dst)
	{
		ot = tds->dst->data;
	}
	for (auto& c : imgdata)
	{
		uint8_t* c8 = (uint8_t*)&c;
		auto r = c8[0];//g
		auto a = gd ? ((uint8_t*)gd)[1] : c8[3];//a
		if (gd)
			gd++;
		auto d = c8[1] * 256.0 * 256.0 + c8[2] * 256;
		//auto d = c8[1] + c8[2];
		bool dd = true;
		ret.imgdepth.push_back(d);
		if (d > 0) {
			d = d;
		}
		int x = ic;
		int xc = 0;
		ic++;
		if (td)
		{
			xc = ((y + tds->dy) * (tds->width)) + x + tds->dx;
			//if (d > 0 && td[xc] > 0) 
			{
				if (d < td[xc])
					dd = 0;
			}
		}
		if (ic == img->width)
		{
			ic = 0; y++;
		}
		uint8_t* g8 = (uint8_t*)&pal[r];
		int trans = 0;
		if (a > 0 && dd)
		{
			c8[0] = (uint8_t)(*g8++);
			c8[1] = (uint8_t)(*g8++);
			c8[2] = (uint8_t)(*g8++);
			c8[3] = (uint8_t)(255 - a);
			if (ot)
			{
				ot[xc] = c;
			}
			//glm::vec4 palColor = { g8[0] / 255.0,g8[1] / 255.0,g8[2] / 255.0,g8[3] / 255.0 };
			//glm::vec4 cur_color = palColor, color = {};
			//glm::vec3 color0 = palColor;  cur_color.w *= 0.5; cur_color.w += a * 255.0;
			//c8[0] = (uint8_t)(cur_color.x * 255.0);
			//c8[1] = (uint8_t)(cur_color.y * 255.0);
			//c8[2] = (uint8_t)(cur_color.z * 255.0);
			//c8[3] = (uint8_t)(cur_color.w * 255.0);
			as.insert(a);
		}
		else { c = 0; }
	}
	memcpy(img->data, imgdata.data(), img->width * img->height * 4);
	ret.width = img->width;
	ret.height = img->height;
	save_img_png(img, ofn.c_str());
	stbimage_load::free_img(palp);
	//stbimage_load::free_img(img);
	stbimage_load::free_img(img_gray);
	ret.dst = img;
	return ret;
}

void new_ui(form_x* form0, vkdg_cx* vkd) {
#if 0
	std::vector<const char*> urls = {
		"https://xyq.res.netease.com/pc/zt/20201104113047/img/f_bg_12_85a17f0c.png",
		"https://xyq.gsf.netease.com/static_h5/shape/char/0756/stand_d0.json",
		"https://xyq.gsf.netease.com/static_h5/shape/char/0756/stand.pal.bmp",
		"https://xyq.gsf.netease.com/static_h5/shape/char/0756/stand_d0.png",
		"https://xyq.gsf.netease.com/static_h5/shape/char/0756/stand_d0.graypng",

		"https://xyq.gsf.netease.com/avtres_hd_full_dir/char/p2/ca499bbd.spr",
		"https://xyq.gsf.netease.com/avtres_hd_full_dir/e132912/p2/ca499bbd.spr",
		"https://xyq.gsf.netease.com/avtres_hd_full_dir/e16416/p2/ca499bbd.spr",

		"https://xyq.gsf.netease.com/avtres_hd_full_dir/char/p2/5e2fe923.spr",
		"https://xyq.gsf.netease.com/avtres_hd_full_dir/e132912/p2/5e2fe923.spr",
		"https://xyq.gsf.netease.com/avtres_hd_full_dir/e16416/p2/5e2fe923.spr",
		"https://xyq.gsf.netease.com/h5avtres/1.25/e198160/p753/stand_d5.json",
		"https://xyq.gsf.netease.com/h5avtres/1.25/e198160/p753/stand_d5.png",
		"https://xyq.gsf.netease.com/h5avtres/1.25/e198160/p753/stand_d5.graypng",
		"https://xyq.gsf.netease.com/static_h5/shape/char/0757/stand_d5.png",
		"https://xyq.gsf.netease.com/static_h5/shape/char/0757/stand_d5.graypng",
		"https://xyq.gsf.netease.com/static_h5/shape/char/0757/stand_d5.json",
	};
	int xt = 0;

	char pngd[4] = { 0x89,0x50,0x4e,0x47 };
	int pngi = *(int*)pngd;
	for (auto url : urls) {
		mcurl_cx fbg;
		std::string k = "temp/xy/spd_" + std::to_string(xt++);
		fbg.downLoad(1, url);
		fbg.data;
		if (*(int*)fbg.data.data() == pngi)
		{
			k += ".png";
		}
		hz::mfile_t m;
		auto pd = m.new_m(k, fbg.data.size());
		if (pd) {
			memcpy(pd, fbg.data.data(), fbg.data.size());
			m.flush();
		}
	}
	// spr转png
	std::vector<image_ptr_t*> v;
	for (size_t i = 5; i < 11; i++)
	{
		std::string fn = "temp/xy/spd_" + std::to_string(i);
		auto img = new_image2spr(fn);
		fn += ".png";
		save_img_png(img, fn.c_str());
		v.push_back(img);
	}
#endif
	std::vector<image_ptr_t*> v;
	{
		std::string fn = "temp/xy1/1008.pal.bmp";
		std::string fn1 = "temp/xy/stand.pal.bmp";
		auto img = stbimage_load::new_load(fn.c_str(), 0);
		v.push_back(img);
	}
	//for (size_t i = 5; i < 11; i++)
	//{
	//	std::string fn = "temp/xy/spd_" + std::to_string(i) + ".png";
	//	auto img = stbimage_load::new_load(fn.c_str(), 0);
	//	v.push_back(img);
	//}
	//auto pimg = spr2png("https://xyq.gsf.netease.com/avtres_hd_full_dir/char/p2/ca499bbd.spr", "temp/xy1/rchar.spr");
	//pimg = spr2png("https://xyq.gsf.netease.com/avtres_hd_full_dir/e1008/p2/ca499bbd.spr", "temp/xy1/re1008.spr");
	//pimg = spr2png("https://xyq.gsf.netease.com/avtres_hd_full_dir/e16416/p2/ca499bbd.spr", "temp/xy1/re16416.spr");
	//pimg = spr2png("https://xyq.gsf.netease.com/avtres_hd_full_dir/e24608/p2/ca499bbd.spr", "temp/xy1/e24608.spr");

	std::string fn = "temp/xy1/re1008.spr.png";
	auto sd = pal_img("temp/xy1/stand.pal.bmp", "temp/xy1/stand_d02.png", "temp/xy1/stand_d0_d.png", "temp/xy1/stand_d0.gray.png");
	sd.dx = 84;
	sd.dy = 0;
	auto sd1 = pal_img("temp/xy1/1008.pal.bmp", "temp/xy1/re1008.spr.png", "temp/xy1/re1008.png", "", &sd);
	save_img_png(sd.dst, "temp/xy1/stand_test.png");
	exit(0);
	menu_m(form0, [=](int idx) {
		auto fnv = hz::browse_openfile((char*)u8"打开spr文件", "", "*.spr", form0->get_nptr(), true);
		for (auto& it : fnv) {
			load_spr(form0, it);
		}
		});
	auto p = new plane_cx();
	uint32_t pbc = -1;
	p->set_color({ 0x80ff802C,1,5,pbc });
	form0->bind(p);	// 绑定到窗口  
	p->set_rss(5);
	p->_lms = { 6,6 };
	p->add_familys(fontn, 0);
	p->draggable = true; //可拖动
	p->set_size({ 1320,1660 });
	p->set_pos({ 1000,100 });
	p->on_click = [](plane_cx* p, int state, int clicks) {};
	p->fontsize = 16;
	int width = 16;
	int rcw = 14;
	{
		// 设置带滚动条
		p->set_scroll(width, rcw, { 0, 0 }, { 2,0 }, { 0,2 });
		p->set_scroll_hide(1);
	}
	p->set_view({ 320,660 }, { 600, 660 });

}

#ifdef _WIN32

static std::string GetCPUNameString()
{
	int nIDs = 0;
	int nExIDs = 0;

	char strCPUName[0x40] = { };

	std::array<int, 4> cpuInfo;
	std::vector<std::array<int, 4>> extData;

	__cpuid(cpuInfo.data(), 0);

	// Calling __cpuid with 0x80000000 as the function_id argument
	// gets the number of the highest valid extended ID.
	__cpuid(cpuInfo.data(), 0x80000000);

	nExIDs = cpuInfo[0];
	for (int i = 0x80000000; i <= nExIDs; ++i)
	{
		__cpuidex(cpuInfo.data(), i, 0);
		extData.push_back(cpuInfo);
	}

	// Interpret CPU strCPUName string if reported
	if (nExIDs >= 0x80000004)
	{
		memcpy(strCPUName, extData[2].data(), sizeof(cpuInfo));
		memcpy(strCPUName + 16, extData[3].data(), sizeof(cpuInfo));
		memcpy(strCPUName + 32, extData[4].data(), sizeof(cpuInfo));
	}

	return strlen(strCPUName) != 0 ? strCPUName : "UNAVAILABLE";
}
#else
static std::string GetCPUNameString()
{
	return "UNAVAILABLE";
}

#endif
void show_cpuinfo(form_x* form0)
{
	cpuinfo_t cpuinfo = get_cpuinfo();
	if (!form0)return;
	auto p = new plane_cx();
	uint32_t pbc = 0xf02c2c2c;
	p->set_color({ 0x80ff802C,1,5,pbc });
	form0->bind(p);	// 绑定到窗口  
	p->set_rss(5);
	p->_lms = { 8,8 };
	p->add_familys(fontn, 0);
	p->draggable = true; //可拖动
	p->set_size({ 420,660 });
	p->set_pos({ 100,100 });
	p->on_click = [](plane_cx* p, int state, int clicks) {};
	p->fontsize = 16;
	int width = 16;
	int rcw = 14;
	{
		// 设置带滚动条
		p->set_scroll(width, rcw, { 0, 0 }, { 2,0 }, { 0,2 });
		p->set_scroll_hide(1);
	}
	// 视图大小，内容大小
	p->set_view({ 420,660 }, { 420, 660 });
	glm::vec2 bs = { 150,22 };
	glm::vec2 bs1 = { 50,22 };
	std::vector<std::string> ncsstr = { (char*)u8"CPU信息","NumLogicalCPUCores","CPUCacheLineSize","SystemRAM","SIMDAlignment" };
	std::vector<std::string> boolstr = { "AltiVec","MMX","SSE","SSE2","SSE3","SSE41","SSE42","AVX","AVX2","AVX512F","ARMSIMD","NEON","LSX","LASX" };

	static std::vector<color_btn*> lbs;
	bs.x = 300;
	uint32_t txtcolor = 0xfff2f2f2;// 0xff7373ff;
	int64_t ds[] = { 0, cpuinfo.NumLogicalCPUCores,cpuinfo.CPUCacheLineSize,cpuinfo.SystemRAM ,cpuinfo.SIMDAlignment };
	{
		int i = 0;
		for (auto& it : ncsstr)
		{
			std::string txt = vkr::format("%-20s: %lld", it.c_str(), ds[i]);
			auto tc = txtcolor;
			if (i == 0) {
				txt = it + ": " + GetCPUNameString();
				tc = 0xffF6801F;
			}
			i++;
			auto kcb = p->add_label(txt, bs, 0);
			kcb->text_color = tc;
			kcb->_disabled_events = true;
			lbs.push_back(kcb);
		}
	}
	bool* bps = &cpuinfo.AltiVec;
	bs.x = 160;
	for (size_t i = 0; i < boolstr.size(); i++)
	{
		auto& it = boolstr[i];
		auto kcb = p->add_label(it.c_str(), bs, 0);
		{
			kcb->_disabled_events = true;
			kcb->text_color = txtcolor;
			auto sw1 = (switch_tl*)p->add_switch(bs1, it.c_str(), bps[i]);
			sw1->_disabled_events = true;
			sw1->get_pos();
			kcb = p->add_label("", bs, 0);
			kcb->_disabled_events = true;
		}
	}
}



void test_img() {

	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr.png", 0);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr10.jpg", 10);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr30.jpg", 30);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr60.jpg", 60);
	hz::get_fullscreen_image(0, 0, 0, "temp/fuckstr80.jpg", 80);
}

int main()
{
#ifdef _DEBUG
	system("rd /s /q E:\\temcpp\\SymbolCache\\tcmp.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\vkcmp.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\cedit.pdb");
	system("rd /s /q E:\\temcpp\\SymbolCache\\p86.pdb");
#endif  
	auto app = new_app();

	uint32_t* cc = get_wcolor();
	for (size_t i = 0; i < 16; i++)
	{
		auto str = get_wcname(i, 0);
		printf("\x1b[01;3%dm%s\x1b[0m\n", (int)i % 8, str);
	}
	//testnjson();
	glm::ivec2 ws = { 1280,860 };
	const char* wtitle = (char*)u8"窗口0";
	const char* wtitle1 = (char*)u8"窗口1";

	form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable /*| ef_borderless*/ | ef_transparent);
	//form_x* form1 = (form_x*)new_form(app, wtitle1, ws.x, ws.y, -1, -1, ef_vulkan | ef_resizable);

	new_ui(form0, 0);
	show_cpuinfo(form0);
	// 运行消息循环
	run_app(app, 0);
	free_app(app);
	return 0;
}
