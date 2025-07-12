
#include <pch1.h>

#include <random>
#include <pnguo/win_core.h>
#include "win32msg.h"
#include <pnguo/event.h>
#define GUI_STATIC_LIB
#include <pnguo/pnguo.h>
#include <pnguo/tinysdl3.h>
#include <pnguo/vkrenderer.h>
#include <pnguo/page.h>
#include <pnguo/mapView.h> 
#include <cairo/cairo.h>

auto fontn = (char*)u8"新宋体,Segoe UI Emoji,Times New Roman,Malgun Gothic";

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
struct iv4
{
	int x, y, z, w;
};
image_ptr_t* new_image2spr(const std::string& fn) {
	image_ptr_t* r = 0;
	hz::mfile_t m;
	auto pd = m.open_d(fn, 0);
	if (pd)
	{
		auto sp = (spr_info_t*)pd;
		std::vector<frames_t> f;
		std::vector<iv4> f4;
		header_t h = {};
		std::vector<int> irn;
		int c = sp->frame * sp->dir;
		auto fp = pd + sizeof(spr_info_t);
		auto fpp = (frames_t*)fp;
		h.frames = fpp;
		for (size_t i = 0; i < c; i++)
		{
			f4.push_back({ fpp->pos_x,fpp->pos_y,fpp->width,fpp->height });
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
		//if (tex)
		//	form0->push_texture(tex, { 0,0,img->width,img->height }, { 10,100,img->width,img->height }, 1);
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
struct img_rp_t
{
	image_ptr_t* dst = 0;
	image_ptr_t* src = 0;
	std::string palfn, fn, ofn, gray;
	std::string depthfn;
	std::vector<double> imgdepth;
	int width, height;
	int dx, dy, z, wa;
};
// 获取图像xy坐标深度，aa为替换透明度
double get_depth(image_ptr_t* p, int x, int y, int aa) {
	size_t dp = x + y * p->width;
	int a = 0; double d = 0.0;
	if (dp < (p->width* p->height))
	{
		uint8_t* c8 = (uint8_t*)&p->data[dp];
		auto r = c8[0];
		auto g = c8[1];
		auto b = c8[2];
		a = aa > 0 ? aa : c8[3];
		d = g * 256.0 * 256.0 + b * 256;
	}
	return a > 0 ? d : 0;
}
void set_px(image_ptr_t* p, int x, int y, uint32_t c) {
	size_t dp = x + y * p->width;
	if (dp < p->width * p->height)
	{
		uint32_t* c2 = (uint32_t*)&p->data[dp];
		*c2 = c;
	}
}
void pal_img(img_rp_t* rp)
{
	auto palp = stbimage_load::new_load(rp->palfn.c_str(), 0);		// 调色板
	auto img = stbimage_load::new_load(rp->fn.c_str(), 0);			// 待处理的图像
	auto img_gray = stbimage_load::new_load(rp->gray.c_str(), 0);	// 单独透明度
	auto idepth = stbimage_load::new_load(rp->depthfn.c_str(), 0);	// 深度图


	uint32_t rtem1[256] = {};
	memcpy(rtem1, palp->data, 4 * 256);

	std::vector<uint32_t> imgdata;
	imgdata.resize(img->width * img->height);
	memcpy(imgdata.data(), img->data, img->width * img->height * 4);
	auto pal = (uint32_t*)palp->data;
	size_t i = 0;
	auto gd = img_gray ? img_gray->data : nullptr;
	auto td = idepth ? idepth->data : nullptr;
	int ic = 0, y = 0;
	uint32_t* ot = 0;
	if (rp->dst) { ot = rp->dst->data; }
	for (size_t y = 0; y < img->height; y++)
	{
		for (size_t x = 0; x < img->width; x++)
		{
			auto& c = imgdata[y * img->width + x];
			uint8_t* c8 = (uint8_t*)&c;
			auto r = c8[0];//r是索引
			auto a = gd ? ((uint8_t*)gd)[1] : c8[3];//判断是否用单独的透明度比如坐骑
			if (a > 0 && rp->wa > 0)
			{
				a += rp->wa - a;
			}
			if (gd) gd++;
			auto d = get_depth(img, x, y, a);
			bool dd = true;
			int xc = 0;
			d += rp->z;
			if (td)
			{
				auto tdd = get_depth(idepth, x + rp->dx, y + rp->dy, 0);
				if (d < tdd)//判断深度
					dd = 0;
			}
			rp->imgdepth.push_back(d);
			auto g8 = *(uint32_t*)&pal[r];
			int trans = 0;
			if (a > 0 && dd)
			{
				c = g8;					//设置索引色
				c8[3] = (uint8_t)(a);	// 替换透明度
				if (rp->dst)
				{
					set_px(rp->dst, x + rp->dx, y + rp->dy, c);
				}
			}
			else { c = 0; }
		}
	}
	memcpy(img->data, imgdata.data(), img->width * img->height * 4);
	rp->width = img->width;
	rp->height = img->height;
	save_img_png(img, rp->ofn.c_str());
	stbimage_load::free_img(palp);
	//stbimage_load::free_img(img);
	stbimage_load::free_img(img_gray);
	rp->src = img;
}

struct librpspr_lib
{
	void* ptr = 0;
	// 创建渲染器，类型是rgba，可以提前设置data做缓冲区，uint32数组或uint8
	int (*new_rp)(int w, int h, int64_t* ret, int bgrtex) = 0;
	// 删除渲染器
	void (*free_rp)(int64_t p) = 0;
	// 渲染完可以获取渲染的数据，ret的大小要跟渲染器一样
	int (*get_image)(void* p, uint8_t* ret) = 0;
	int (*rp_clear)(void* p, uint32_t c, double depth) = 0;
	// 新api。以下文件名支持网址、本地文件
	// 添加spr和对应的调色板文件名，调色板偏移(有的可能要负数)。返回序号draw_spr用
	int (*add_spr)(void* p, const char* fn, const char* palfn, int pal_pos) = 0;
	// 获取spr帧数
	int (*get_spr_frame_count)(void* p, int idx) = 0;
	// 保存spr的图片到png文件
	int (*save_spr2png)(void* p, int idx, const char* fn) = 0;
	// 加载坐骑/附件/足迹光环等，文件名用分号分隔json;png;graypng;pal.bmp顺序不限。坐骑深度信息没有.pal.bmp就拼三种文件名即可
	int (*add_part)(void* p, const char* uris) = 0;
	int (*set_ride)(void* p, const char* fly, int d, const char* fnm, const char* fntcp) = 0;
	int (*set_addon)(void* p, const char* addons) = 0;

	// 渲染顺序排序ride\spr\part\addon
	int (*draw_ride)(void* p, int frame, int* pos) = 0;
	int (*draw_spr)(void* p, int idx, int frame, int* pos, int z) = 0;
	int (*draw_part)(void* p, int idx, int frame, int* pos, int z) = 0;
	int (*draw_addon_flr)(void* p, int frame, int* pos) = 0;
	double (*get_max_depth)(void* p) = 0;
	int (*draw_spr_ps)(void* p, int idx, int frame, int* dst, int z, int ps) = 0;
public:
	librpspr_lib();
	~librpspr_lib();
	bool load();
};
librpspr_lib::librpspr_lib() {
	load();
}
librpspr_lib::~librpspr_lib() {
	if (ptr)hz::shared_destroy(ptr);
	delete ptr; ptr = 0;
}
bool librpspr_lib::load()
{
	if (ptr)return true;
	auto k = hz::shared_load(R"(rpspr.dll)");
	static const char* ccfn[] = { "ptr_null",
	"new_rp","free_rp","get_image","rp_clear","add_spr","get_spr_frame_count","save_spr2png"
	,"add_part","set_ride","set_addon","draw_ride","draw_spr","draw_part","draw_addon_flr","get_max_depth","draw_spr_ps"
	};
	if (k)
	{
		ptr = k;
		hz::shared_get(k, ccfn, (void**)&ptr, sizeof(ccfn) / sizeof(char*));
	}
	return ptr && new_rp && free_rp;
}
class dspr_cx
{
public:
	librpspr_lib* so = 0;
	void* ctx = 0;
	std::vector<glm::ivec4> sprv;
	std::vector<uint32_t> imgdata;
	cairo_surface_t* img = 0;

	// 坐骑的渲染坐标
	glm::ivec2 ride_pos = {};
	glm::ivec2 csize = {};
public:
	dspr_cx();
	~dspr_cx();

	void init(int w, int h);
	void clear();
	void add_spr(const std::string& sprfn, const std::string& palbmp, int pal_pos);
	//不同坐骑可能要设置不同值
	void set_spr_pos(uint32_t idx, int x, int y, int z = 950000);
	void set_ride(const std::string& fly, int d, const std::string& addon, const std::string& fnm_dir, const std::string& fntcp_dir);
	void draw(int i, int x, int y);
private:

};

dspr_cx::dspr_cx()
{
	so = new librpspr_lib();
}

dspr_cx::~dspr_cx()
{
	if (img)
		free_image_cr(img);
	img = 0;
	if (so)
	{
		if (ctx)
			so->free_rp((int64_t)ctx);
		delete so; so = 0;
	}
}

void dspr_cx::init(int w, int h)
{
	bool tex_bgr = 1;
	csize = { w,h };
	if (ctx)
		so->free_rp((int64_t)ctx);
	so->new_rp(w, h, (int64_t*)&ctx, tex_bgr);
	glm::ivec2 csize = { w,h };
	imgdata.resize(w * h);
	if (img)
		free_image_cr(img);
	img = 0;
	img = new_image_cr(csize, imgdata.data());
	sprv.clear();
}


//在__init__函数调用 add_spr\set_ride
void dspr_cx::add_spr(const std::string& sprfn, const std::string& palbmp, int pal_pos)
{
	auto w = so->add_spr(ctx, sprfn.c_str(), palbmp.c_str(), pal_pos);
	sprv.push_back({ 0,0,0,w });
}

void dspr_cx::set_ride(const std::string& fly, int d, const std::string& addon, const std::string& fnm_dir, const std::string& fntcp_dir)
{
	// 动作类型，方向，坐骑深度目录(需要有这三种文件json;png;graypng)，坐骑tcp目录（json;png;graypng;pal.bmp）
	//so->set_ride(ctx, "stand", 0, R"(https://xyq.gsf.netease.com/h5avtres/1.25/e135776/p722/)", R"(https://xyq.gsf.netease.com/static_h5/shape/char/0722/)");
	so->set_ride(ctx, fly.c_str(), d, fnm_dir.c_str(), fntcp_dir.c_str());
	// 附件，路径和坐骑tcp一样，（json;png;graypng;pal.bmp）保存在addon_f之类文件夹。支持fblr四种要有相应文件夹
	so->set_addon(ctx, addon.c_str());
}

void dspr_cx::set_spr_pos(uint32_t idx, int x, int y, int z)
{
	if (idx < sprv.size())
	{
		sprv[idx].x = x;
		sprv[idx].y = y;
		sprv[idx].z = z;
	}
}

void dspr_cx::draw(int i, int x, int y)
{
	glm::ivec2 pos = { x,y };
	auto rpos = pos + ride_pos;
	//渲染调用 
	so->rp_clear(ctx, 0xff000000, 0.0);
	so->draw_ride(ctx, i, (int*)&rpos);//写入坐骑原始深度\渲染坐骑和附件b
	auto maxd = so->get_max_depth(ctx);
	//渲染角色/装备等
	static int ps[10] = {};
	int q = 0;
	for (auto& it : sprv)
	{
		glm::ivec2 sp = it;
		sp += pos;
		if (it.w > 0)
			so->draw_spr_ps(ctx, it.w, i, (int*)&sp, it.z, ps[q]);
		q++;
	}
	auto maxd0 = so->get_max_depth(ctx);
	// 渲染坐骑挂件
	so->draw_addon_flr(ctx, i, (int*)&rpos);
	so->get_image(ctx, (uint8_t*)imgdata.data());
}
class spr_dev
{
public:
	dspr_cx* sp = 0;
	std::string cstr;
	int fp = 0;
	double dt = 0.0;
	double dt0 = 0.125;
public:
	spr_dev();
	~spr_dev();
	void set_text(const std::string& str);

	bool update(float t);
	void draw(cairo_t* cr, const glm::vec2& dps);
private:

};

spr_dev::spr_dev()
{
	sp = new dspr_cx();
}

spr_dev::~spr_dev()
{
}

void spr_dev::set_text(const std::string& str)
{
	try
	{
		njson nc = njson::parse(str);
		auto kstr = nc.dump(2);
		if (kstr != cstr && sp)
		{
			cstr = kstr;

			glm::ivec2 csize = hz::toiVec2(nc["csize"]);
			auto ride_pos = hz::toiVec2(nc["ride_pos"]);
			auto fly = hz::toStr(nc["fly"]);
			auto dir = hz::toInt(nc["dir"]);
			auto addon = hz::toStr(nc["addon"]);
			auto ride_dir = hz::toStr(nc["ride_dir"]);
			auto ride_tcp_dir = hz::toStr(nc["ride_tcp_dir"]);
			auto spr_dir = hz::toStr(nc["spr_dir"]);
			auto& cspr = nc["spr"];
			sp->init(csize.x, csize.y);
			sp->ride_pos = ride_pos;
			sp->set_ride(fly, dir, addon, ride_dir, ride_tcp_dir);
			int i = 0;
			for (auto& c0 : cspr)
			{
				std::string palurl = R"(https://xyq.gsf.netease.com/static_h5/pal/equip/)";
				auto pos = hz::toiVec3(c0["pos"]);
				auto pal = hz::toiVec2(c0["pal"]);
				auto uri = spr_dir + hz::toStr(c0["name"]);
				sp->add_spr((char*)uri.c_str(), palurl + std::to_string(pal.x) + ".pal.bmp", pal.y);
				sp->set_spr_pos(i, pos.x, pos.y, pos.z);
				i++;
			}
		}
	}
	catch (const std::exception& e)
	{
		printf("json error:\t%s", e.what());
	}
}

bool spr_dev::update(float t)
{
	dt += t;
	bool r = false;
	if (dt > dt0) {
		dt = 0;
		if (sp) {
			sp->draw(fp, 0, 0);
			fp++;
			if (fp > 100000)fp = 0;
			r = true;
		}
	}
	return r;
}

void spr_dev::draw(cairo_t* cr, const glm::vec2& dps)
{
	cairo_as _ss_(cr);
	cairo_translate(cr, dps.x, dps.y);
	image_b pimg = {};
	pimg.image = sp->img;
	pimg.rc = { 0,0,sp->csize };
	// 图形通用软渲染接口
	draw_ge(cr, &pimg, 1);
}

void build_spr_ui(form_x* form0)
{
	std::string tp = hz::get_temp_path();
	glm::ivec2 csize = { 800,600 };
	njson c;
	c["csize"] = { 800,600 };
	c["fly"] = "stand";
	c["dir"] = 0;
	c["addon"] = "flr";
	c["ride_dir"] = R"(https://xyq.gsf.netease.com/h5avtres/1.25/e135776/p722/)";
	c["ride_tcp_dir"] = R"(https://xyq.gsf.netease.com/static_h5/shape/char/0722/)";
	c["spr_dir"] = R"(https://xyq.gsf.netease.com/avtres_hd_full_dir/)";
	c["spr_dir"] = (char*)u8R"(E:\d3m\rpspr\rpspr\out\xybin\肤色\飞燕女\站立\)";
	c["ride_pos"] = { 250,280 };
	auto& cspr = c["spr"];
	for (size_t i = 0; i < 1; i++)
	{
		njson c0;
		c0["pos"] = { 230,210,950000 };
		c0["pal"] = { 24608,192 };//下衣64 头饰128 特殊192 其他0
		c0["name"] = R"(e24608/p2/8379f964.spr)";
		c0["name"] = R"(7(196201c2).spr)";
		cspr.push_back(c0);
	}
	glm::ivec2 size = { 1024,800 };
	glm::ivec2 cview = { 10240,10240 };
	auto p = new plane_cx();
	uint32_t pbc = 0xff2c2c2c;// 背景色
	p->set_color({ 0x80ff802C,1,5,pbc });
	p->set_rss(5);
	p->_lms = { 8,8 };
	form0->bind(p);	// 绑定到窗口  
	p->add_familys(fontn, 0);
	//p->add_familys(fontn1, 0);
	p->draggable = false; //可拖动
	p->fontsize = 16;
	p->set_size(size);
	p->set_pos({ 1,30 });
	int width = 16;
	int rcw = 14;
	{
		// 设置带滚动条
		//p->set_scroll(width, rcw, { 4, 4 }, { 2,0 }, { 0,2 });
		//p->set_scroll_hide(1);
		//p->set_view(size, cview);
	}
	auto spredit = p->add_input("", { 1000,300 }, 0);
	auto cstr = c.dump(2);
	static spr_dev* sp = new spr_dev();
	cstr = (char*)u8"abc设置\n123\n发的发awdf";
	sp->set_text(cstr);
	spredit->set_round_path(0.2);
	spredit->set_text(cstr.c_str(), cstr.size());
	spredit->changed_cb = [=](edit_tl* ptr) {
		};
	auto btn0 = p->add_cbutton((char*)u8"修改", { 100,30 }, 0);
	btn0->click_cb = [=](void* p, int css) {
		auto str = spredit->_text;
		sp->set_text(str);
		spredit->set_text(sp->cstr.c_str(), sp->cstr.size());
		};

	auto dpx1 = p->push_dragpos({ 140,360 });// , { 300,600 });// 增加一个拖动坐标 
	p->update_cb = [=](float delta)
		{
			return sp->update(delta);
		};
	p->draw_front_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
		};
	p->draw_back_cb = [=](cairo_t* cr, const glm::vec2& scroll)
		{
			auto dps = p->get_dragpos(dpx1);//获取拖动时的坐标
			sp->draw(cr, dps);

		};

}

void test_spr(form_x* form0, vkdg_cx* vkd) {
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

	{
		std::string fn = "temp/xy1/re1008.spr";
		auto img = new_image2spr(fn);
	}
	img_rp_t rw = {}, zq = {};
	zq.palfn = "temp/xy1/stand.pal.bmp"; zq.fn = "temp/xy1/stand_d0.png"; zq.ofn = "temp/xy1/stand_d0_d.png"; zq.gray = "temp/xy1/stand_d0.gray.png";
	{
		image_ptr_t* dst = 0;
		std::string palfn, fn, ofn, gray;
		std::string depthfn;
		std::vector<double> imgdepth;
		int width, height;
		int dx, dy, z;
	};
	rw.palfn = "temp/xy1/1008.pal.bmp"; rw.fn = "temp/xy1/re1008.spr.png"; rw.ofn = "temp/xy1/re1008.png"; rw.depthfn = "temp/xy1/stand_d0.png";
	rw.dx = 80 - 3;
	rw.dy = 26 - 27;
	rw.wa = 255;
	pal_img(&zq);	// 处理坐骑
	rw.dst = zq.src;
	pal_img(&rw);
	save_img_png(rw.dst, "temp/xy1/stand_test.png");
	//menu_m(form0, [=](int idx) {
	//	auto fnv = hz::browse_openfile((char*)u8"打开spr文件", "", "*.spr", form0->get_nptr(), true);
	//	for (auto& it : fnv) {
	//		load_spr(form0, it);
	//	}
	//	}); 

}

void show_gui(form_x* form0)
{
	if (!form0)return;
	glm::ivec2 size = { 1024,800 };
	glm::ivec2 cview = { 10240,10240 };
	auto p = new plane_cx();
	uint32_t pbc = 0xf02c2c2c;
	p->set_color({ 0x80ff802C,1,5,pbc });
	form0->bind(p);	// 绑定到窗口  
	p->set_rss(5);
	p->_lms = { 8,8 };
	p->add_familys(fontn, 0);
	p->draggable = false; //可拖动
	p->set_size(size);
	p->set_pos({ 30,50 }); 
	p->fontsize = 16;
}
int main()
{
	auto app = new_app();
	glm::ivec2 ws = { 1280,860 };
	const char* wtitle = (char*)u8"窗口0";
	const char* wtitle1 = (char*)u8"窗口1";
	form_x* form0 = (form_x*)new_form(app, wtitle, ws.x, ws.y, -1, -1, 0);
	//test_spr(form0, 0);
	show_gui(form0);
	// 运行消息循环
	run_app(app, 0);
	free_app(app);
	return 0;
}
