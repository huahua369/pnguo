// tria.cpp: 定义应用程序的入口点。
//

#include "tria.h"
#include "t3.h"
#include <clipper2/clipper.h> 
using namespace Clipper2Lib;

#include "mcut/mcut_cx.h"

#include "earcut.hpp"
namespace mapbox {
	namespace util {

		template <>
		struct nth<0, glm::vec2> {
			inline static auto get(const glm::vec2& t) {
				return t.x;
			};
		};
		template <>
		struct nth<1, glm::vec2> {
			inline static auto get(const glm::vec2& t) {
				return t.y;
			};
		};

	} // namespace util
} // namespace mapbox

using plane3_t = std::vector<glm::vec3>;
class polygon_ct
{
public:
	std::vector<PathsD*> _c;
public:
	polygon_ct();
	~polygon_ct();
	void make(PathsD* src, bool isin = false);
	size_t triangulate(plane3_t* opt, float z, bool pccw);
	size_t triangulate(mesh3_mt* opt, float z, bool pccw);
private:

};

polygon_ct::polygon_ct()
{
}

polygon_ct::~polygon_ct()
{
	for (auto it : _c)
	{
		if (it)
		{
			delete it;
		}
	}
	_c.clear();
}

bool in_polygon(const PathD& c, const PathD& src)
{
	int outsideCnt = 0;
	for (auto& pt : c)
	{
		PointInPolygonResult result = PointInPolygon(pt, src);
		if (result == PointInPolygonResult::IsInside) --outsideCnt;
		else if (result == PointInPolygonResult::IsOutside) ++outsideCnt;
		if (outsideCnt > 1) { return false; }
		else if (outsideCnt < -1) break;
	}
	return true;
}

void polygon_ct::make(PathsD* src, bool isin)
{
	std::map<int64_t, std::vector<PathsD*>> mpd;
	std::vector<PathD> c1;
	for (auto& it : *src)
	{
		// 判断是逆时针
		bool ccw = IsPositive(it);
		auto da = Area(it) * 100;
		int64_t a = da;
		if (a == 0)
		{
			continue;
		}
		if (isin)
		{
			if (a < 0)
			{
				auto p = new PathsD();
				p->push_back(std::move(it));
				mpd[a].push_back(p);
			}
			else
			{
				c1.push_back(std::move(it));
			}
		}
		else {
			if (a > 0)
			{
				auto p = new PathsD();
				p->push_back(std::move(it));
				mpd[a].push_back(p);
			}
			else
			{
				c1.push_back(std::move(it));
			}
		}
	}
	size_t xc = 0;
	for (auto& it : c1) {
		for (auto& [k, v] : mpd)
		{
			bool isb = false;
			for (auto ct : v)
			{
				if (in_polygon(it, ct->at(0)))//判断包含关系
				{
					ct->push_back(it); xc++; isb = true; break;
				}
			}
			if (isb)
				break;
		}
	}
	for (auto& [k, v] : mpd)
	{
		for (auto ct : v)
		{
			_c.push_back(ct);
		}
	}
}

void p2tri(plane3_t* opt, std::vector<std::vector<glm::vec2>>& tr, float z, bool pccw)
{
	using N = uint32_t;
	std::vector<N> indices = mapbox::earcut<N>(tr);
	auto length = indices.size();
	std::vector<glm::vec2> dt;
	dt.reserve(length);
	opt->reserve(opt->size() + length);
	for (auto& it : tr)
	{
		for (auto v : it)
			dt.push_back(v);
	}
	if (pccw)
	{
		for (size_t i = 0; i < length; i++)
		{
			int x = indices[i + 2];
			int x1 = indices[i + 1];
			int x2 = indices[i];
			opt->push_back({ dt[x] ,z });
			opt->push_back({ dt[x1] ,z });
			opt->push_back({ dt[x2] ,z });
			i += 2;
		}
	}
	else {
		for (size_t i = 0; i < length; i++)
		{
			int x = indices[i];
			int x1 = indices[i + 1];
			int x2 = indices[i + 2];
			opt->push_back({ dt[x] ,z });
			opt->push_back({ dt[x1] ,z });
			opt->push_back({ dt[x2] ,z });
			i += 2;
		}
	}

}
void p2tri_ind(mesh3_mt* opt, std::vector<std::vector<glm::vec2>>& tr, float z, bool pccw)
{
	using N = uint32_t;
	std::vector<N> indices = mapbox::earcut<N>(tr);
	auto length = indices.size();
	std::vector<glm::vec2> dt;
	dt.reserve(length);
	auto pss = opt->vertices.size();
	opt->vertices.reserve(opt->vertices.size());
	for (auto& it : tr)
	{
		for (auto v : it)
		{
			dt.push_back(v);
			opt->vertices.push_back(glm::vec3(v, z));
		}
	}
	if (pccw)
	{
		for (size_t i = 0; i < length; i++)
		{
			glm::ivec3 x = { indices[i + 2] + pss, indices[i + 1] + pss, indices[i] + pss };
			opt->indices.push_back(x);
			i += 2;
		}
	}
	else {
		for (size_t i = 0; i < length; i++)
		{
			glm::ivec3 x = { indices[i] + pss, indices[i + 1] + pss, indices[i + 2] + pss };
			opt->indices.push_back(x);
			i += 2;
		}
	}

}

size_t polygon_ct::triangulate(plane3_t* opt, float z, bool pccw)
{
	auto pos = opt->size();
	{
		//("earcut1");
		for (auto vt : _c)
		{
			std::vector<glm::vec2> nl;
			std::vector<std::vector<glm::vec2>> tr;
			std::vector<int> as, cs;
			for (auto& it : *vt)
			{
				nl.clear();
				// 判断是逆时针
				bool ccw = IsPositive(it);
				int a = Area(it);
				as.push_back(a);
				cs.push_back(ccw);
				for (auto& v : it)
				{
					nl.push_back({ v.x,v.y });
				}
				if (nl[0] != nl[nl.size() - 1])
					nl.push_back(nl[0]);
				tr.push_back(nl);
			}
			p2tri(opt, tr, z, pccw);
		}
	}
	return opt->size() - pos;
}
size_t polygon_ct::triangulate(mesh3_mt* opt, float z, bool pccw)
{
	auto pos = opt->indices.size();
	{
		//("earcut1");
		for (auto vt : _c)
		{
			std::vector<glm::vec2> nl;
			std::vector<std::vector<glm::vec2>> tr;
			std::vector<int> as, cs;
			for (auto& it : *vt)
			{
				nl.clear();
				// 判断是逆时针
				bool ccw = IsPositive(it);
				int a = Area(it);
				as.push_back(a);
				cs.push_back(ccw);
				for (auto& v : it)
				{
					nl.push_back({ v.x,v.y });
				}
				if (nl[0] != nl[nl.size() - 1])
					nl.push_back(nl[0]);
				tr.push_back(nl);
			}
			p2tri_ind(opt, tr, z, pccw);
		}
	}
	return opt->indices.size() - pos;
}

// 判断点是否在多边形内
int pnpoly_aos(int nvert, const float* verts, float testx, float testy) {
	int i, j;
	bool c = 0;
	for (i = 0, j = nvert - 1; i < nvert; j = i++) {
		if (((verts[i * 2 + 1] > testy) != (verts[j * 2 + 1] > testy)) &&
			(testx < (verts[j * 2] - verts[i * 2]) * (testy - verts[i * 2 + 1]) / (verts[j * 2 + 1] - verts[i * 2 + 1]) + verts[i * 2])) {
			c = !c;
		}
	}
	return c;
}
glm::vec2 pnpoly_aos(int nvert, const glm::vec2* verts, const glm::vec2& test) {
	int i, j;
	bool c = 0;
	float d = glm::distance2(test, verts[0]);
	for (i = 0, j = nvert - 1; i < nvert; j = i++) {
		if (((verts[i].y > test.y) != (verts[j].y > test.y)) &&
			(test.x < (verts[j].x - verts[i].x) * (test.y - verts[i].y) / (verts[j].y - verts[i].y) + verts[i].x)) {
			c = !c;
		}
		auto d1 = glm::distance2(test, verts[i]);
		if (d1 < d)
			d = d1;
	}
	return { c, d };
}

void triangle_to_mesh(const mesh3_mt& src_mesh, mesh_mt& dstMesh, const glm::mat4& src_nm = glm::identity<glm::mat4>())
{
	auto ps = dstMesh.vertex_coord.size() / 3;
	// vertices precision convention and copy
	dstMesh.vertex_coord.reserve(src_mesh.vertices.size() * 3);
	for (int i = 0; i < src_mesh.vertices.size(); ++i) {
		const glm::vec3 v = src_nm * glm::vec4(src_mesh.vertices[i], 1.0);
		dstMesh.vertex_coord.push_back(v[0]);
		dstMesh.vertex_coord.push_back(v[1]);
		dstMesh.vertex_coord.push_back(v[2]);
	}

	// faces copy
	dstMesh.face_indice.reserve(src_mesh.indices.size() * 3);
	dstMesh.face_size.reserve(src_mesh.indices.size());
	for (int i = 0; i < src_mesh.indices.size(); ++i) {
		const int& f0 = src_mesh.indices[i][0];
		const int& f1 = src_mesh.indices[i][1];
		const int& f2 = src_mesh.indices[i][2];
		dstMesh.face_indice.push_back(f0 + ps);
		dstMesh.face_indice.push_back(f1 + ps);
		dstMesh.face_indice.push_back(f2 + ps);

		dstMesh.face_size.push_back((uint32_t)3);
	}
}
mesh_mt triangle_to_mesh(const mesh3_mt& M)
{
	mesh_mt ot;
	triangle_to_mesh(M, ot);
	return ot;
}
int triangulates(std::vector<glm::vec2>* poly, int n, bool pccw, std::vector<glm::vec2>* p, int type)
{
	if (!p)return 0;
	auto& ms = *p;
	auto pos = ms.size();
	std::vector<std::vector<glm::vec2>>  tr, tr1;
	float z = 0.0;
	std::vector<PointD> pv;
	for (size_t i = 0; i < n; i++)
	{
		auto& tv1 = poly[i];
		pv.resize(tv1.size());
		for (size_t i = 0; i < tv1.size(); i++)
		{
			pv[i] = { tv1[i].x,tv1[i].y };
		}
		bool ccw = IsPositive(pv);
		if (ccw)
			tr1.push_back(tv1);
		else
		{
			tr.push_back(tv1);
		}
	}
	std::vector<glm::vec3> ms3;
	auto nc = tr.size();
	std::map<size_t, std::vector<std::vector<glm::vec2>>> shell;
	for (size_t i = 0; i < nc; i++)
	{
		shell[i].push_back(tr[i]);
	}
	for (auto& it : tr1)
	{
		float dis = -1;
		int64_t c = -1;
		std::vector<glm::vec2> disv;
		for (size_t i = 0; i < nc; i++)
		{
			auto n = tr[i].size();
			auto& st = tr[i];
			glm::vec2 ps = it[0];
			float dis1 = 0;
			auto bd = pnpoly_aos(n, st.data(), ps);// 返回是否在多边形内、最短距离
			disv.push_back(bd);
			if (bd.x > 0)
			{
				if (dis < 0 || bd.y < dis)
				{
					c = i;
					dis = bd.y;
				}
			}
		}
		if (c >= 0)
		{
			shell[c].push_back(it);//holes
		}
	}
	if (type == 1) {

		PathsD	solution;
		for (auto& [k, v] : shell)
		{
			solution.clear();
			for (auto& it : v) {
				if (it.size() > 2)
				{
					pv.resize(it.size());
					for (size_t i = 0; i < it.size(); i++)
					{
						pv[i] = { it[i].x,it[i].y };
					}
					solution.push_back(pv);
				}
			}
			if (v.size())
			{
				polygon_ct pct;
				pct.make(&solution, true);
				pct.triangulate(&ms3, 0, pccw);
			}
		}
	}
	else
	{
		for (auto& [k, v] : shell)
		{
			if (v.size())
				gp::constrained_delaunay_triangulation_v(&v, ms3, pccw, z);
		}
	}

	for (auto& it : ms3)
		ms.push_back(it);
	return 0;
}

int triangulates(std::vector<glm::vec2>* poly, int n, bool pccw, mesh_mt* opt, int type)
{
	if (!opt || !poly || n < 1)return 0;
	std::vector<glm::vec3> ms;
	auto pos = ms.size();
	std::vector<std::vector<glm::vec2>>  tr, tr1;
	float z = 0.0;
	std::vector<PointD> pv;
	for (size_t i = 0; i < n; i++)
	{
		auto& tv1 = poly[i];
		pv.resize(tv1.size());
		for (size_t i = 0; i < tv1.size(); i++)
		{
			pv[i] = { tv1[i].x,tv1[i].y };
		}
		bool ccw = IsPositive(pv);
		if (ccw)
			tr1.push_back(tv1);
		else
		{
			tr.push_back(tv1);
		}
	}
	mesh3_mt m3;
	std::vector<glm::vec3> ms3;
	auto nc = tr.size();
	std::map<size_t, std::vector<std::vector<glm::vec2>>> shell;
	for (size_t i = 0; i < nc; i++)
	{
		shell[i].push_back(tr[i]);
	}
	for (auto& it : tr1)
	{
		float dis = -1;
		int64_t c = -1;
		std::vector<glm::vec2> disv;
		for (size_t i = 0; i < nc; i++)
		{
			auto n = tr[i].size();
			auto& st = tr[i];
			glm::vec2 ps = it[0];
			float dis1 = 0;
			auto bd = pnpoly_aos(n, st.data(), ps);// 返回是否在多边形内、最短距离
			disv.push_back(bd);
			if (bd.x > 0)
			{
				if (dis < 0 || bd.y < dis)
				{
					c = i;
					dis = bd.y;
				}
			}
		}
		if (c >= 0)
		{
			shell[c].push_back(it);//holes
		}
	}
	if (type == 0)
	{
		for (auto& [k, v] : shell)
		{
			if (v.size())
				gp::constrained_delaunay_triangulation_v(&v, m3.vertices, m3.indices, pccw, z);
		}
	}
	else {
		PathsD	solution;
		for (auto& [k, v] : shell)
		{
			solution.clear();
			for (auto& it : v) {
				if (it.size() > 2)
				{
					pv.resize(it.size());
					for (size_t i = 0; i < it.size(); i++)
					{
						pv[i] = { it[i].x,it[i].y };
					}
					solution.push_back(pv);
				}
			}
			if (v.size())
			{
				polygon_ct pct;
				pct.make(&solution, true);
				pct.triangulate(&m3, 0, pccw);
			}
		}
	}
	triangle_to_mesh(m3, *opt);
	return 0;
}

int triangulates(std::vector<glm::vec2>* poly, int n, bool pccw, mesh3_mt* opt, int type)
{
	if (!opt || !poly || n < 1)return 0;
	std::vector<glm::vec3> ms;
	auto pos = ms.size();
	std::vector<std::vector<glm::vec2>>  tr, tr1;
	float z = 0.0;
	std::vector<PointD> pv;
	for (size_t i = 0; i < n; i++)
	{
		auto& tv1 = poly[i];
		pv.resize(tv1.size());
		for (size_t i = 0; i < tv1.size(); i++)
		{
			pv[i] = { tv1[i].x,tv1[i].y };
		}
		bool ccw = IsPositive(pv);
		if (ccw)
			tr1.push_back(tv1);
		else
		{
			tr.push_back(tv1);
		}
	}
	mesh3_mt m3;
	std::vector<glm::vec3> ms3;
	auto nc = tr.size();
	std::map<size_t, std::vector<std::vector<glm::vec2>>> shell;
	for (size_t i = 0; i < nc; i++)
	{
		shell[i].push_back(tr[i]);
	}
	for (auto& it : tr1)
	{
		float dis = -1;
		int64_t c = -1;
		std::vector<glm::vec2> disv;
		for (size_t i = 0; i < nc; i++)
		{
			auto n = tr[i].size();
			auto& st = tr[i];
			glm::vec2 ps = it[0];
			float dis1 = 0;
			auto bd = pnpoly_aos(n, st.data(), ps);// 返回是否在多边形内、最短距离
			disv.push_back(bd);
			if (bd.x > 0)
			{
				if (dis < 0 || bd.y < dis)
				{
					c = i;
					dis = bd.y;
				}
			}
		}
		if (c >= 0)
		{
			shell[c].push_back(it);//holes
		}
	}
	if (type == 0)
	{
		for (auto& [k, v] : shell)
		{
			if (v.size())
				gp::constrained_delaunay_triangulation_v(&v, m3.vertices, m3.indices, pccw, z);
		}
	}
	else {
		PathsD	solution;
		for (auto& [k, v] : shell)
		{
			solution.clear();
			for (auto& it : v) {
				if (it.size() > 2)
				{
					pv.resize(it.size());
					for (size_t i = 0; i < it.size(); i++)
					{
						pv[i] = { it[i].x,it[i].y };
					}
					solution.push_back(pv);
				}
			}
			if (v.size())
			{
				polygon_ct pct;
				pct.make(&solution, true);
				pct.triangulate(&m3, 0, pccw);
			}
		}
	}
	m3.vertices.swap(opt->vertices);
	m3.indices.swap(opt->indices);
	return 0;
}


#if 1
class m3_cx :public m3_t
{
public:
	std::vector<glm::vec2> data;
public:
	m3_cx();
	~m3_cx();

private:

};

m3_cx::m3_cx()
{
}

m3_cx::~m3_cx()
{
}

m3_t* triangulates(float* polys, int* n, int count, int pccw, int type)
{
	if (!polys || !n || count < 1)return nullptr;
	std::vector<std::vector<glm::vec2>> poly;
	std::vector<glm::vec2> p;
	poly.resize(count);
	auto v2 = (glm::vec2*)polys;
	for (size_t i = 0; i < count; i++)
	{
		auto& it = poly[i];
		it.reserve(n[i]);
		for (size_t x = 0; x < n[i]; x++)
		{
			it.push_back(*v2);
			v2++;
		}
	}
	int r = triangulates(poly.data(), count, pccw, &p, type);
	if (p.size())
	{
		auto pt = new m3_cx();
		pt->count = p.size();
		pt->data.swap(p);
		pt->dim = 2;
		pt->v2 = (float*)pt->data.data();
		return pt;
	}
	return nullptr;
}
void free_m3(m3_t* p) {
	if (p)
	{
		delete (m3_cx*)p;
	}
}
struct m3idx_cx
{
	m3idx_t o;
	mesh3_mt m3;
};

m3idx_t* triangulates_idx(float* polys, int* n, int count, int pccw, int type)
{
	if (!polys || !n || count < 1)return nullptr;
	std::vector<std::vector<glm::vec2>> poly;
	mesh3_mt p;
	poly.resize(count);
	auto v2 = (glm::vec2*)polys;
	for (size_t i = 0; i < count; i++)
	{
		auto& it = poly[i];
		it.reserve(n[i]);
		for (size_t x = 0; x < n[i]; x++)
		{
			it.push_back(*v2);
			v2++;
		}
	}
	int r = triangulates(poly.data(), count, pccw, &p, type);
	if (p.vertices.size() && p.indices.size())
	{
		auto pt = new m3idx_cx();
		pt->o.count = p.indices.size() * 3;
		pt->o.vcount = p.vertices.size();
		pt->m3.indices.swap(p.indices);
		pt->m3.vertices.swap(p.vertices);
		pt->o.indexs = (int*)pt->m3.indices.data();
		pt->o.v3 = (float*)pt->m3.vertices.data();
		return (m3idx_t*)pt;
	}
	return nullptr;
}
void free_m3idx(m3idx_t* p)
{
	if (p)
	{
		delete (m3idx_cx*)p;
	}
}

#endif // 1

// 计算区域面积，负数则环是逆时针
template<class T>
double area_ofringsigned(T* ring, size_t count)
{
	std::size_t rlen = count;
	if (rlen < 3 || !ring) {
		return 0.0;
	}

	double sum = 0.0;
	/*
	 * Based on the Shoelace formula.
	 * http://en.wikipedia.org/wiki/Shoelace_formula
	 */
	double x0 = ring[0].x;
	for (std::size_t i = 1; i < rlen - 1; i++) {
		double x = ring[i].x - x0;
		double y1 = ring[i + 1].y;
		double y2 = ring[i - 1].y;
		sum += x * (y2 - y1);
	}
	return sum / 2.0;
}

int main0()
{
	float polysa[] = {
		-1, -1,
		-1, 1,
		1, 1,
		1, -1,
		-0.2f, -0.2f,
		0.2f, -0.2f,
		0.2f, 0.2f,
		-0.2f, 0.2f,
	};
	float polys[] = {
		-2556.98,984.59,
		-2556.98,984.59,
		-2365.46,1446.46,
		-2365.46,1446.46,
		-2376.19,1420.59,
		-2546.25,1010.45
	};
	int n[] = { 6 };
	auto a = area_ofringsigned((glm::vec2*)polys, n[0]);
	int count = 1;
	std::vector<glm::vec2> v2, v20;
	std::vector<glm::vec3> v3;
	std::vector<int> v3_idx;
	auto m3 = triangulates(polys, n, count, 0, 1);
	if (m3 && m3->v2)
	{
		v2.resize(m3->count);
		memcpy(v2.data(), m3->v2, sizeof(float) * m3->count * 2);
	}
	auto m33 = triangulates_idx(polys, n, count, 0, 0);
	if (m33 && m33->v3)
	{
		v3.resize(m33->vcount);
		memcpy(v3.data(), m33->v3, sizeof(float) * m33->vcount * 3);
		v3_idx.resize(m33->count);
		memcpy(v3_idx.data(), m33->indexs, sizeof(int) * m33->count);
		v20.resize(m33->count);
		for (size_t i = 0; i < m33->count; i++)
		{
			v20[i] = v3[m33->indexs[i]];
		}
	}

	int inc = 0;
	for (size_t i = 0; i < m33->count; i++)
	{
		if (glm::distance(v2[i], v20[i]) > 0.001)
		{
			inc++;
		}
	}
	return 0;
}
