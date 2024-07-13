
#include "pch1.h"

#include "stlrw.h"
#include "vkgui/mapView.h"

#if 1
// 计算法向量
glm::vec3 normal_v3(glm::vec3* dv)
{
	glm::vec3 a = dv[0], b = dv[1], c = dv[2];
	auto ba = b - a;
	auto ca = c - a;
	auto cr = cross(ba, ca);
	auto n = normalize(cr);
	return n;
}
float normal_v2(glm::vec2* dv)
{
	glm::vec3 a = glm::vec3(dv[0], 0), b = glm::vec3(dv[1], 0), c = glm::vec3(dv[2], 0);
	auto ba = b - a;
	auto ca = c - a;
	auto cr = cross(ba, ca);
	auto n = normalize(cr);
	return n.z;
}

std::string getLine(char*& _pos, int64_t& size)
{
	std::string line;
	char* p = _pos;
	auto p1 = p;
	for (; size > 0 && (*p1 != '\n'); p1++, size--)
	{
	}
	line.append(p, p1);
	if (size > 0) {
		size--;
		_pos = p1 + 1;
	}
	return line;
}
std::string getLine1(char*& _pos, int64_t size)
{
	std::string line;
	char* p = _pos + size;
	char* p1 = p;
	do {
		for (; size > 0 && (*p1 != '\n'); p1--, size--)
		{
		}
		if (p1 + 1 != p)
			line.assign(p1 + 1, p + 1);
		p1--; p = p1;
	} while (line.empty());
	_pos = p1;
	return line;
}









stl3d_cx::stl3d_cx()
{
}
stl3d_cx::stl3d_cx(const std::string& fn)
{
	load(fn);
}

stl3d_cx::~stl3d_cx()
{
}
// 添加三角面
int stl3d_cx::add(glm::vec3* p, int n, int resn)
{
	int ret = 0;
	auto ps = faces.size();
	if (resn > 0)
		faces.reserve(ps + resn);
	if (p && n >= 3)
	{
		auto c = n / 3;
		auto v3 = sizeof(glm::vec3) * 3;
		glm::vec3 pss = pos;
		size_t nc = (pos.x != 0 || pos.y != 0 || pos.z != 0) ? 3 : 0;
		for (int i = 0; i < c; i++)
		{
			stl_face_t v = {};
			auto t = &v;
			memcpy(t->v, p, v3);
			for (size_t x = 0; x < nc; x++)
			{
				auto& it = t->v[x];
				it += pss;
			}
			auto n3 = normal2(*t);
			if (glm::isnan(n3.x) || glm::isnan(n3.y) || glm::isnan(n3.z))
			{
				t++;
				ret--;
			}
			else {
				faces.push_back(v);
			}
			p += 3;
		}
	}
	return ret;
}
// 添加三角面
int stl3d_cx::add(glm::vec3* p, int n, glm::vec3 pos)
{
	int ret = 0;
	if (p && n >= 3)
	{
		auto v3 = sizeof(glm::vec3) * 3;
		auto ps = faces.size();
		auto c = n / 3;
		faces.reserve(ps + c);
		glm::vec3 pss = pos;
		size_t nc = (pos.x != 0 || pos.y != 0 || pos.z != 0) ? 3 : 0;
		for (int i = 0; i < c; i++)
		{
			stl_face_t v = {};
			auto t = &v;
			memcpy(t->v, p, v3);
			for (size_t x = 0; x < nc; x++)
			{
				auto& it = t->v[x];
				it += pss;
			}
			auto n3 = normal2(*t);
			if (glm::isnan(n3.x) || glm::isnan(n3.y) || glm::isnan(n3.z))
			{
				t++;
				ret--;
			}
			else {
				faces.push_back(v);
			}
			p += 3;
		}
	}
	return ret;
}
int stl3d_cx::add(glm::vec2* p, int n, glm::vec3 pos)
{
	int ret = 0;
	if (p && n >= 3)
	{
		auto ps = faces.size();
		auto c = n / 3;
		faces.reserve(ps + c);
		glm::vec3 pss = pos;
		size_t nc = (pos.x != 0 || pos.y != 0 || pos.z != 0) ? 3 : 0;
		for (int i = 0; i < c; i++)
		{
			stl_face_t v = {};
			auto t = &v;
			for (size_t x = 0; x < 3; x++) {
				t->v[x] = { p[x], 0 };
			}
			for (size_t x = 0; x < nc; x++)
			{
				auto& it = t->v[x];
				it += pss;
			}
			auto n3 = normal2(*t);
			if (glm::isnan(n3.x) || glm::isnan(n3.y) || glm::isnan(n3.z))
			{
				t++; ret--;
			}
			else {
				faces.push_back(v);
			}
			p += 3;
		}
	}
	return ret;
}
void stl3d_cx::add(stl3d_cx* p, glm::vec3 pos)
{
	if (p)
	{
		auto ps = faces.size();
		auto ps1 = p->faces.size();
		faces.resize(ps + ps1);
		auto t = faces.data();
		t += ps;
		memcpy(t, p->faces.data(), ps1 * sizeof(faces[0]));
		size_t nc = (pos.x != 0 || pos.y != 0 || pos.z != 0) ? 3 : 0;
		for (size_t i = 0; i < ps1; i++)
		{
			for (size_t x = 0; x < nc; x++)
			{
				auto& it = t[i].v[x];
				it += pos;
			}
			normal2(t[i]);
		}
	}
}

void stl_face_t::set_pos(const glm::vec3& ps)
{
	v[0] += ps;
	v[1] += ps;
	v[2] += ps;
}
glm::vec3 stl3d_cx::normal2(stl_face_t& d)
{
	d.normal = normal_v3(d.v);
	return d.normal;
}
#ifdef min
#undef min
#undef max
#endif
void stl3d_cx::get_bounding()
{
	glm::vec3 mmin = glm::vec3(INT_MAX, INT_MAX, INT_MAX);
	glm::vec3 mmax = glm::vec3(-INT_MAX, -INT_MAX, -INT_MAX);
	for (auto& it : faces)
	{
		for (size_t i = 0; i < 3; i++)
		{
			auto v = it.v[i];
			mmin.x = std::min(v.x, mmin.x);
			mmin.y = std::min(v.y, mmin.y);
			mmin.z = std::min(v.z, mmin.z);
			mmax.x = std::max(v.x, mmax.x);
			mmax.y = std::max(v.y, mmax.y);
			mmax.z = std::max(v.z, mmax.z);
		}
	}
	bounding[0] = mmin;
	bounding[1] = mmax;
	size = mmax - mmin;
}

void stl3d_cx::box_scale(const glm::vec3& s) {
	if (s.x > 0 && s.y > 0 && s.z > 0)
	{
		glm::vec3 mmin = glm::vec3(INT_MAX, INT_MAX, INT_MAX);
		glm::vec3 mmax = glm::vec3(-INT_MAX, -INT_MAX, -INT_MAX);
		for (auto& it : faces)
		{
			for (size_t i = 0; i < 3; i++)
			{
				auto& v = it.v[i];
				v *= s;
				mmin.x = std::min(v.x, mmin.x);
				mmin.y = std::min(v.y, mmin.y);
				mmin.z = std::min(v.z, mmin.z);
				mmax.x = std::max(v.x, mmax.x);
				mmax.y = std::max(v.y, mmax.y);
				mmax.z = std::max(v.z, mmax.z);
			}
			normal2(it);
		}
		bounding[0] = mmin;
		bounding[1] = mmax;
		size = mmax - mmin;
	}
}
void stl3d_cx::box_scale_i(const glm::ivec3& s)
{
	if (!(size.x > 0 && size.y > 0 && size.z > 0))
	{
		get_bounding();
	}
	glm::vec3 ks = s;
	ks /= size;
	if (ks.x > 0 && ks.y > 0 && ks.z > 0)
	{
		box_scale(ks);
	}
}


void stl3d_cx::load(const std::string& fn)
{
	hz::mfile_t ad;
	char* d = ad.open_d(fn, true);
	auto cfs = ad.get_size();
	if (d && cfs > 15)
	{
		std::string ks(d, cfs);
		auto fmt = get_stlFormat(d, ad.get_size());
		if (fmt == 1)
		{
			load_ascii(d, cfs);
		}
		else if (fmt == 2)
		{
			load_binary(d, cfs);
		}
		neighbors_start.resize(faces.size());
	}
}

int stl3d_cx::get_stlFormat(const char* data, int64_t size)
{
	int r = 0;
	do
	{
		if (!data)break;
		auto t0 = data + 80;
		uint32_t* ns = (uint32_t*)t0;
		if (*ns > 0)
		{
			if (*ns * 50 + 84 == size)
			{
				r = 2; break;
			}
		}
		std::string as[] = { "solid","endsolid" };
		char* t = (char*)data; int64_t n = size;
		auto h = getLine(t, n);
		int x = 0;
		for (; h.size() && x < 2;)
		{
			h.resize(as[x].size());
			if (h == as[x])
			{
				x++;
				if (x == 1)
					h = getLine1(t, n);
			}
			else {
				break;
			}
		}
		if (x == 2)
		{
			r = 1; break;
		}
	} while (0);
	return r;
}

int stl3d_cx::save(const std::string& fn, int fmt)
{
	return fmt == 1 ? save_ascii(fn) : save_binary(fn);
}
int stl3d_cx::save_ascii(const std::string& fn)
{
	hz::mfile_t ad;
	static const char* fmtf[2] = { " facet normal % 12e % 12e % 12e\n   outer loop\n\0","     vertex % 12e % 12e % 12e\n\0" };
	static const char* fmt[2] = { " facet normal %g %g %g\n   outer loop\n\0","     vertex %g %g %g\n\0" };
	//static const char* fmt[2] = { " facet normal %.2f %.2f %.2f\n   outer loop\n\0","     vertex %.2f %.2f %.2f\n\0" };
	if (ad.open_m(fn, false, false))
	{
		std::string buf("solid\n");
		char sval[128] = {};
		do {
			int nb = faces.size();
			for (int i = 0; i < nb; i++)
			{
				auto it = faces[i];
				it.set_pos(pos);
				auto& normale = it.normal;
				sprintf(sval, fmt[0], normale.x, normale.y, normale.z);
				buf.append(sval);
				auto& v = it.v;
				for (int j = 0; j < 3; j++)
				{
					auto node = v[j];
					sprintf(sval, fmt[1], node.x, node.y, node.z);
					buf.append(sval);
				}
				buf.append("   endloop\n endfacet\n");//, 21);
			}
			buf.append("endsolid\n", 9);
		} while (0);
		auto bs = buf.size();
		if (bs)
		{
			ad.ftruncate_m(bs);
			auto w = ad.map(bs, 0);
			if (w)
			{
				memcpy(w, buf.c_str(), bs);
				ad.flush(0, bs);
				return bs;
			}
		}
	}
	return 0;
}
int stl3d_cx::save_binary(const std::string& fn)
{
	hz::mfile_t ad;
	if (ad.open_m(fn, false, false))
	{
		//std::string buf;
		std::vector<char> buf;
		do {
			auto nb = faces.size();
			auto as = 84 + nb * 50;
			buf.resize(as);
			char* t = (char*)buf.data();
			memset(t, 0x20, 80);
			auto nt = (uint32_t*)(t + 80);
			*nt = nb;
			t += 84;
			for (int i = 0; i < nb; i++)
			{
				auto it = faces[i];
				it.set_pos(pos);
				memcpy(t, &it, 50);
				t += 50;
			}
		} while (0);
		auto bs = buf.size();
		if (bs)
		{
			ad.ftruncate_m(bs);
			auto w = ad.map(bs, 0);
			if (w)
			{
				memcpy(w, buf.data(), bs);
				ad.flush(0, bs);
				return bs;
			}
		}
	}
	return 0;
}
int stl3d_cx::save_binary(std::vector<char>& buf)
{
	do {
		auto nb = faces.size();
		auto as = 84 + nb * 50;
		buf.resize(as);
		char* t = (char*)buf.data();
		memset(t, 0x20, 80);
		auto nt = (uint32_t*)(t + 80);
		*nt = nb;
		t += 84;
		for (int i = 0; i < nb; i++)
		{
			auto it = faces[i];
			it.set_pos(pos);
			memcpy(t, &it, 50);
			t += 50;
		}
	} while (0);
	return buf.size();
}


void stl3d_cx::load_ascii(char* data, int64_t cfs)
{
	int nl = 0;
	for (int64_t i = 0; i < cfs; i++)
		if (data[i] == '\n')nl++;

	char* t = (char*)data; int64_t n = cfs;
	auto h = getLine(t, n);
	auto d = h.c_str();
	auto br = nl / 7;//ASCII_LINES_PER_FACET
	if (nl > 0)
		faces.reserve(nl);
	int hr = 0;
	// main reading
	for (int i = 0; i < br && n>0; i++)
	{
		stl_face_t a = {};
		h = getLine(t, n);
		d = h.c_str();
		// skipping the facet normal
		auto& normal = a.normal;
		hr = sscanf(d, "%*s %*s %f %f %f\n", &normal[0], &normal[1], &normal[2]);
		h = getLine(t, n);
		d = h.c_str();
		// skip the keywords "outer loop"
		hr = sscanf(d, "%*s %*s");
		// reading nodes
		for (int x = 0; x < 3; x++)
		{
			// reading vertex
			h = getLine(t, n);
			d = h.c_str();
			hr = sscanf(d, "%*s %f %f %f\n", &a.v[x][0], &a.v[x][1], &a.v[x][2]);
		}
		h = getLine(t, n);
		d = h.c_str();
		// skip the keywords "endloop"
		hr = sscanf(d, "%*s");
		h = getLine(t, n);
		d = h.c_str();
		// skip the keywords "endfacet"
		hr = sscanf(d, "%*s");
		faces.push_back(a);
	}

}
void stl3d_cx::load_binary(char* d, int64_t cfs)
{
	int fsize = 50;
	auto fts = sizeof(stl_face_t);
	auto t0 = d + 80;
	uint32_t* ns = (uint32_t*)t0;
	if (*ns > 0)
	{
		t0 += sizeof(uint32_t);
		faces.resize(*ns);
		for (size_t i = 0; i < *ns; i++)
		{
			memcpy(&faces[i], t0, fsize);
			t0 += fsize;
		}
	}
}
#endif