/*
 plot库
*/
#include "pch1.h"
#include <cstdint>
#include <cmath>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "render.h"
#include "plot.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif // !M_PI_2


#define MUL_NO_OVERFLOW	((size_t)1 << (sizeof(size_t) * 4))
void*
reallocarray(void* optr, size_t nmemb, size_t size)
{
	if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
		nmemb > 0 && SIZE_MAX / nmemb < size) {
		errno = ENOMEM;
		return NULL;
	}
	return realloc(optr, size * nmemb);
}

static void
kplotdat_free(struct kplotdat* p)
{
	size_t	 i;

	if (NULL == p)
		return;

	for (i = 0; i < p->datasz; i++) {
		kdata_destroy(p->datas[i]);
		if (KPLOTCTYPE_PATTERN == p->cfgs[i].line.clr.type)
			vkvg_pattern_destroy(p->cfgs[i].line.clr.pattern);
		if (KPLOTCTYPE_PATTERN == p->cfgs[i].point.clr.type)
			vkvg_pattern_destroy(p->cfgs[i].point.clr.pattern);
	}

	free(p->datas);
	free(p->cfgs);
	free(p->types);
}

struct kplot*
	kplot_alloc(const struct kplotcfg* cfg)
{
	struct kplot* p;
	size_t		 i;

	p = (kplot*)calloc(1, sizeof(struct kplot));

	if (NULL == p)
		return(NULL);

	if (NULL == cfg)
		kplotcfg_defaults(&p->cfg);
	else
		p->cfg = *cfg;

	/* Refernece all patterns. */

	if (KPLOTCTYPE_PATTERN == p->cfg.borderline.clr.type)
		vkvg_pattern_reference
		(p->cfg.borderline.clr.pattern);
	if (KPLOTCTYPE_PATTERN == p->cfg.ticline.clr.type)
		vkvg_pattern_reference
		(p->cfg.ticline.clr.pattern);
	if (KPLOTCTYPE_PATTERN == p->cfg.gridline.clr.type)
		vkvg_pattern_reference
		(p->cfg.gridline.clr.pattern);
	if (KPLOTCTYPE_PATTERN == p->cfg.ticlabelfont.clr.type)
		vkvg_pattern_reference
		(p->cfg.ticlabelfont.clr.pattern);
	if (KPLOTCTYPE_PATTERN == p->cfg.axislabelfont.clr.type)
		vkvg_pattern_reference
		(p->cfg.axislabelfont.clr.pattern);

	if (0 == p->cfg.clrsz)
		return(p);

	/*
	 * If we pass an array of colour settings, then we want to
	 * duplicate the array instead of copying it wholesale, as the
	 * caller may free it in the meantime.
	 * In doing so, we need to reference the Cairo patterns.
	 */
	p->cfg.clrs = (kplotccfg*)calloc(p->cfg.clrsz, sizeof(struct kplotccfg));

	if (NULL == p->cfg.clrs) {
		p->cfg.clrsz = 0;
		kplot_free(p);
		return(NULL);
	}

	memcpy(p->cfg.clrs, cfg->clrs,
		p->cfg.clrsz * sizeof(struct kplotccfg));

	for (i = 0; i < p->cfg.clrsz; i++)
		if (KPLOTCTYPE_PATTERN == p->cfg.clrs[i].type)
			vkvg_pattern_reference(p->cfg.clrs[i].pattern);

	return(p);
}

struct kplotcfg*
	kplot_get_plotcfg(struct kplot* p)
{

	return(&p->cfg);
}

static void
kplot_data_remove_all(struct kplot* p)
{
	size_t	 i;

	for (i = 0; i < p->datasz; i++)
		kplotdat_free(&p->datas[i]);

	free(p->datas);
	p->datas = NULL;
	p->datasz = 0;
}

void
kplot_free(struct kplot* p)
{
	size_t	 i;

	if (NULL == p)
		return;

	kplot_data_remove_all(p);

	if (KPLOTCTYPE_PATTERN == p->cfg.borderline.clr.type)
		vkvg_pattern_destroy
		(p->cfg.borderline.clr.pattern);
	if (KPLOTCTYPE_PATTERN == p->cfg.ticline.clr.type)
		vkvg_pattern_destroy
		(p->cfg.ticline.clr.pattern);
	if (KPLOTCTYPE_PATTERN == p->cfg.gridline.clr.type)
		vkvg_pattern_destroy
		(p->cfg.gridline.clr.pattern);
	if (KPLOTCTYPE_PATTERN == p->cfg.ticlabelfont.clr.type)
		vkvg_pattern_destroy
		(p->cfg.ticlabelfont.clr.pattern);
	if (KPLOTCTYPE_PATTERN == p->cfg.axislabelfont.clr.type)
		vkvg_pattern_destroy
		(p->cfg.axislabelfont.clr.pattern);

	for (i = 0; i < p->cfg.clrsz; i++)
		if (KPLOTCTYPE_PATTERN == p->cfg.clrs[i].type)
			vkvg_pattern_destroy(p->cfg.clrs[i].pattern);

	free(p->cfg.clrs);
	free(p->datas);
	free(p);
}

void
ksmthcfg_defaults(struct ksmthcfg* p)
{

	p->movsamples = 3;
}

int
kplot_detach(struct kplot* p, const struct kdata* d)
{
	size_t		 i, j;
	struct kplotdat* dat;
	void* pp;

	/*
	 * Search for the data plot.
	 * We look in all plot sources, so if this is just one of a
	 * multiplot, we'll still remove it.
	 */
	for (i = 0; i < p->datasz; i++) {
		dat = &p->datas[i];
		for (j = 0; j < dat->datasz; j++)
			if (dat->datas[j] == d)
				break;
		if (j < dat->datasz)
			break;
	}
	/* Not found... */
	if (i == p->datasz)
		return(0);

	/* Free the found data plot source. */
	kplotdat_free(&p->datas[i]);

	/*
	 * Move data above the copied region to replace the current
	 * region.
	 * This preserves the order of plot sets.
	 */
	memmove(&p->datas[i], &p->datas[i + 1],
		(p->datasz - i - 1) *
		sizeof(struct kplotdat));
	p->datasz--;
	pp = reallocarray(p->datas,
		p->datasz, sizeof(struct kplotdat));
	if (NULL == pp) {
		/* This really, really shouldn't happen. */
		return(0);
	}
	p->datas = (struct kplotdat*)pp;
	return(1);
}

void kplotccfg_init_palette(struct kplotccfg* c, size_t palette)
{

	switch (c->type) {
	case (KPLOTCTYPE_DEFAULT):
		c->type = KPLOTCTYPE_PALETTE;
		c->palette = palette;
		break;
	case (KPLOTCTYPE_PATTERN):
		assert(NULL != c->pattern);
		vkvg_pattern_reference(c->pattern);
		break;
	default:
		break;
	}
}

static int
kplotdat_attach(struct kplot* p, size_t sz, struct kdata** d,
	const struct kdatacfg* const* cfg,
	const enum kplottype* types, enum kplotstype stype,
	enum ksmthtype smthtype, const struct ksmthcfg* smth)
{
	void* pp;
	size_t		 i;
	struct kdatacfg* dcfg;

	pp = reallocarray(p->datas,
		p->datasz + 1, sizeof(struct kplotdat));
	if (NULL == pp)
		return(0);
	p->datas = (struct kplotdat*)pp;

	p->datas[p->datasz].datas =
		(struct kdata**)calloc(sz, sizeof(struct kdata*));
	if (NULL == p->datas[p->datasz].datas)
		return(0);
	p->datas[p->datasz].cfgs =
		(struct kdatacfg*)calloc(sz, sizeof(struct kdatacfg));
	if (NULL == p->datas[p->datasz].cfgs)
		return(0);
	p->datas[p->datasz].types =
		(enum kplottype*)calloc(sz, sizeof(enum kplottype));
	if (NULL == p->datas[p->datasz].types)
		return(0);

	for (i = 0; i < sz; i++) {
		p->datas[p->datasz].datas[i] = d[i];
		p->datas[p->datasz].types[i] = types[i];
		dcfg = &p->datas[p->datasz].cfgs[i];
		if (NULL == cfg || NULL == cfg[i])
			kdatacfg_defaults(dcfg);
		else
			*dcfg = *cfg[i];
		kplotccfg_init_palette
		(&dcfg->point.clr, p->datasz);
		kplotccfg_init_palette
		(&dcfg->line.clr, p->datasz);
		d[i]->refs++;
	}

	p->datas[p->datasz].smthtype = smthtype;
	if (NULL != smth) {
		p->datas[p->datasz].smth = *smth;
		/* Make sure we're odd around the sample. */
		if (0 == (2 % p->datas[p->datasz].smth.movsamples))
			p->datas[p->datasz].smth.movsamples++;
	}
	else
		ksmthcfg_defaults(&p->datas[p->datasz].smth);
	p->datas[p->datasz].datasz = sz;
	p->datas[p->datasz].stype = stype;
	p->datasz++;
	return(1);
}

int
kplot_get_datacfg(struct kplot* p, size_t pos,
	struct kdatacfg** datas, size_t* datasz)
{

	*datas = NULL;
	*datasz = 0;

	if (pos >= p->datasz)
		return(0);

	*datas = p->datas[pos].cfgs;
	*datasz = p->datas[pos].datasz;
	return(1);
}

int
kplot_attach_smooth(struct kplot* p, struct kdata* d,
	enum kplottype t, const struct kdatacfg* cfg,
	enum ksmthtype smthtype, const struct ksmthcfg* smth)
{

	return(kplotdat_attach(p, 1, &d, &cfg,
		&t, KPLOTS_SINGLE, smthtype, smth));
}

int
kplot_attach_data(struct kplot* p, struct kdata* d,
	enum kplottype t, const struct kdatacfg* cfg)
{

	return(kplotdat_attach(p, 1, &d, &cfg,
		&t, KPLOTS_SINGLE, KSMOOTH_NONE, NULL));
}

int
kplot_attach_datas(struct kplot* p, size_t sz,
	struct kdata** d, const enum kplottype* t,
	const struct kdatacfg* const* cfg, enum kplotstype st)
{

	if (sz < 2)
		return(0);
	return(kplotdat_attach(p, sz, d,
		cfg, t, st, KSMOOTH_NONE, NULL));
}


// kdata

void
kdata_destroy(struct kdata* d)
{
	size_t	 i;

	if (NULL == d)
		return;

	assert(d->refs > 0);
	if (--d->refs > 0)
		return;

	switch (d->type) {
	case (KDATA_MEAN):
		free(d->d.mean.ns);
		break;
	case (KDATA_STDDEV):
		free(d->d.stddev.ns);
		free(d->d.stddev.m1s);
		free(d->d.stddev.m2s);
		break;
	default:
		break;
	}

	/* Destroy dependeants along with ourselves. */
	for (i = 0; i < d->depsz; i++)
		kdata_destroy(d->deps[i].dep);

	free(d->deps);
	free(d->pairs);
	free(d);
}

void
kdatacfg_defaults(struct kdatacfg* cfg)
{

	memset(cfg, 0, sizeof(struct kdatacfg));
	cfg->point.radius = 3.0;
	cfg->point.sz = 2.0;
	cfg->point.clr.type = KPLOTCTYPE_DEFAULT;
	cfg->line.sz = 2.0;
	cfg->line.join = VKVG_LINE_JOIN_ROUND;
	cfg->line.clr.type = KPLOTCTYPE_DEFAULT;
}

/*
 * We've modified a value at (pair) position "pos".
 * Pass this through to the underlying functional sources, if any.
 */
int
kdata_dep_run(struct kdata* data, size_t pos)
{
	size_t	 i;
	int	 rc;
	double	 x, y;

	x = data->pairs[pos].x;
	y = data->pairs[pos].y;

	for (rc = 1, i = 0; 0 != rc && i < data->depsz; i++)
		rc = data->deps[i].func
		(data->deps[i].dep, pos, x, y);

	return(rc);
}

/*
 * Add a functional kdata source "data" (e.g., stddev) to another kdata
 * source "dep" as a dependent.
 * All a source's dependents are updated with each modification of the
 * source's internal pair values.
 */
int
kdata_dep_add(struct kdata* data, struct kdata* dep, ksetfunc fp)
{
	void* p;

	p = reallocarray(dep->deps,
		dep->depsz + 1, sizeof(struct kdep));
	if (NULL == p)
		return(0);
	dep->deps = (kdep*)p;
	dep->deps[dep->depsz].dep = data;
	dep->deps[dep->depsz].func = fp;
	dep->depsz++;

	/* While the parent exists, we must exist. */
	data->refs++;
	return(1);
}

double
kdata_pmfvar(const struct kdata* data)
{
	double	 ysum, mean, var;
	size_t	 i;

	if (0 == data->pairsz)
		return(0.0);

	for (ysum = 0.0, i = 0; i < data->pairsz; i++)
		ysum += data->pairs[i].y;

	if (ysum == 0.0)
		return(0.0);

	for (mean = 0.0, i = 0; i < data->pairsz; i++)
		mean += data->pairs[i].y / ysum * data->pairs[i].x;

	for (var = 0.0, i = 0; i < data->pairsz; i++)
		var += data->pairs[i].y / ysum *
		(data->pairs[i].x - mean) *
		(data->pairs[i].x - mean);

	return(var);
}

double
kdata_pmfstddev(const struct kdata* data)
{

	return(sqrt(kdata_pmfvar(data)));
}

double
kdata_pmfmean(const struct kdata* data)
{
	double	 ysum, sum;
	size_t	 i;

	if (0 == data->pairsz)
		return(0.0);

	for (ysum = 0.0, i = 0; i < data->pairsz; i++)
		ysum += data->pairs[i].y;

	if (ysum == 0.0)
		return(0.0);

	for (sum = 0.0, i = 0; i < data->pairsz; i++)
		sum += data->pairs[i].y / ysum * data->pairs[i].x;

	return(sum);
}

double
kdata_xmean(const struct kdata* data)
{
	double	 sum;
	size_t	 i;

	if (0 == data->pairsz)
		return(0.0);
	for (sum = 0.0, i = 0; i < data->pairsz; i++)
		sum += data->pairs[i].x;
	return(sum / (double)data->pairsz);
}

double
kdata_ymean(const struct kdata* data)
{
	double	 sum;
	size_t	 i;

	if (0 == data->pairsz)
		return(0.0);
	for (sum = 0.0, i = 0; i < data->pairsz; i++)
		sum += data->pairs[i].y;
	return(sum / (double)data->pairsz);
}

double
kdata_xstddev(const struct kdata* data)
{
	double	 sum, mean;
	size_t	 i;

	if (0 == data->pairsz)
		return(0.0);
	mean = kdata_xmean(data);
	for (sum = 0.0, i = 0; i < data->pairsz; i++)
		sum += (data->pairs[i].x - mean) *
		(data->pairs[i].x - mean);
	return(sqrt(sum / (double)data->pairsz));
}

double
kdata_ystddev(const struct kdata* data)
{
	double	 sum, mean;
	size_t	 i;

	if (0 == data->pairsz)
		return(0.0);
	mean = kdata_xmean(data);
	for (sum = 0.0, i = 0; i < data->pairsz; i++)
		sum += (data->pairs[i].y - mean) *
		(data->pairs[i].y - mean);
	return(sqrt(sum / (double)data->pairsz));
}

size_t
kdata_xmax(const struct kdata* d, struct kpair* kp)
{
	size_t	 	i, max;
	struct kpair	pair;

	if (0 == d->pairsz)
		return(-1);

	max = 0;
	pair = d->pairs[max];
	for (i = 1; i < d->pairsz; i++)
		if (d->pairs[i].x > pair.x) {
			pair = d->pairs[i];
			max = i;
		}
	if (NULL != kp)
		*kp = pair;
	return(max);
}

size_t
kdata_xmin(const struct kdata* d, struct kpair* kp)
{
	size_t	 	i, min;
	struct kpair	pair;

	if (0 == d->pairsz)
		return(-1);

	min = 0;
	pair = d->pairs[min];
	for (i = 1; i < d->pairsz; i++)
		if (d->pairs[i].x < pair.x) {
			pair = d->pairs[i];
			min = i;
		}
	if (NULL != kp)
		*kp = pair;
	return(min);
}

size_t
kdata_ymax(const struct kdata* d, struct kpair* kp)
{
	size_t	 	i, max;
	struct kpair	pair;

	if (0 == d->pairsz)
		return(-1);

	max = 0;
	pair = d->pairs[max];
	for (i = 1; i < d->pairsz; i++)
		if (d->pairs[i].y > pair.y) {
			pair = d->pairs[i];
			max = i;
		}
	if (NULL != kp)
		*kp = pair;
	return(max);
}

size_t
kdata_ymin(const struct kdata* d, struct kpair* kp)
{
	size_t	 	i, min;
	struct kpair	pair;

	if (0 == d->pairsz)
		return(-1);

	min = 0;
	pair = d->pairs[min];
	for (i = 1; i < d->pairsz; i++)
		if (d->pairs[i].y < pair.y) {
			pair = d->pairs[i];
			min = i;
		}
	if (NULL != kp)
		*kp = pair;
	return(min);
}

int
kdata_get(const struct kdata* d, size_t pos, struct kpair* kp)
{

	if (pos >= d->pairsz)
		return(0);
	*kp = d->pairs[pos];
	return(1);
}

int
kdata_set(struct kdata* d, size_t pos, double x, double y)
{

	if (pos >= d->pairsz)
		return(0);
	d->pairs[pos].x = x;
	d->pairs[pos].y = y;
	return(d->depsz ? kdata_dep_run(d, pos) : 1);
}

// !kdata

// array

struct kdata*
	kdata_array_alloc(const struct kpair* np, size_t npsz)
{
	struct kdata* d;
	size_t		 i;

	if (NULL == (d = (struct kdata*)calloc(1, sizeof(struct kdata))))
		return(NULL);

	d->pairsz = npsz;
	d->pairs = (struct kpair*)calloc(d->pairsz, sizeof(struct kpair));
	if (NULL == d->pairs) {
		free(d);
		return(NULL);
	}

	if (NULL == np)
		for (i = 0; i < d->pairsz; i++)
			d->pairs[i].x = i;
	else
		memcpy(d->pairs, np, d->pairsz * sizeof(struct kpair));

	d->refs = 1;
	d->type = KDATA_ARRAY;
	return(d);
}

int
kdata_array_fill_ysizes(struct kdata* d, const size_t* v)
{
	size_t		i;
	int		rc = 1;

	if (KDATA_ARRAY != d->type)
		return(0);

	if (d->depsz)
		for (i = 0; 0 != rc && i < d->pairsz; i++)
			rc = kdata_set(d, i, d->pairs[i].x, v[i]);
	else
		for (i = 0; i < d->pairsz; i++)
			d->pairs[i].y = v[i];

	return(rc);
}

int
kdata_array_fill_ydoubles(struct kdata* d, const double* v)
{
	size_t		i;
	int		rc = 1;

	if (KDATA_ARRAY != d->type)
		return(0);

	if (d->depsz)
		for (i = 0; 0 != rc && i < d->pairsz; i++)
			rc = kdata_set(d, i, d->pairs[i].x, v[i]);
	else
		for (i = 0; i < d->pairsz; i++)
			d->pairs[i].y = v[i];

	return(rc);
}

int kdata_array_fill(struct kdata* d, void* arg, void (*fp)(size_t, struct kpair*, void*))
{
	size_t		i;
	int		rc = 1;
	struct kpair	kp;

	if (KDATA_ARRAY != d->type)
		return(0);

	/* Act directly on the data if not having deps. */
	if (d->depsz)
		for (i = 0; 0 != rc && i < d->pairsz; i++) {
			(*fp)(i, &kp, arg);
			rc = kdata_set(d, i, kp.x, kp.y);
		}
	else
		for (i = 0; i < d->pairsz; i++)
			(*fp)(i, &d->pairs[i], arg);

	return(rc);
}

static int
kdata_array_checkrange(const struct kdata* d, size_t v)
{

	return(KDATA_ARRAY == d->type && v < d->pairsz);
}

int
kdata_array_add(struct kdata* d, size_t v, double val)
{
	double	 x, y;

	if (!kdata_array_checkrange(d, v))
		return(0);
	x = d->pairs[v].x;
	y = d->pairs[v].y + val;
	return(kdata_set(d, v, x, y));
}

int
kdata_array_set(struct kdata* d, size_t v, double x, double y)
{

	if (!kdata_array_checkrange(d, v))
		return(0);
	return(kdata_set(d, v, x, y));
}

// !array

struct kdata*
	kdata_bucket_alloc(size_t rmin, size_t rmax)
{
	struct kdata* d;
	size_t		 i;

	if (NULL == (d = (struct kdata*)calloc(1, sizeof(struct kdata))))
		return(NULL);

	d->refs = 1;
	d->pairsz = rmax - rmin;
	d->pairs = (struct kpair*)calloc(d->pairsz, sizeof(struct kpair));
	if (NULL == d->pairs) {
		free(d);
		return(NULL);
	}

	for (i = 0; i < d->pairsz; i++)
		d->pairs[i].x = rmin + i;

	d->type = KDATA_BUCKET;
	d->d.bucket.rmin = rmin;
	d->d.bucket.rmax = rmax;
	return(d);
}

static int
kdata_bucket_checkrange(const struct kdata* d, size_t v)
{

	return(KDATA_BUCKET == d->type &&
		v >= d->d.bucket.rmin && v < d->d.bucket.rmax);
}

int
kdata_bucket_set(struct kdata* d, size_t v, double x, double y)
{

	if (!kdata_bucket_checkrange(d, v))
		return(0);
	return(kdata_set(d, v - d->d.bucket.rmin, x, y));
}

int
kdata_bucket_add(struct kdata* d, size_t v, double val)
{
	double 	 x, y;

	if (!kdata_bucket_checkrange(d, v))
		return(0);
	x = d->pairs[v - d->d.bucket.rmin].x;
	y = d->pairs[v - d->d.bucket.rmin].y + val;
	return(kdata_set(d, v - d->d.bucket.rmin, x, y));
}

struct kdata*
	kdata_buffer_alloc(size_t hint)
{
	struct kdata* d;

	if (NULL == (d = (struct kdata*)calloc(1, sizeof(struct kdata))))
		return(NULL);

	d->pairsz = hint;
	d->pairs = (struct kpair*)calloc(d->pairsz, sizeof(struct kpair));
	if (NULL == d->pairs) {
		free(d);
		return(NULL);
	}

	d->refs = 1;
	d->type = KDATA_BUFFER;
	return(d);
}

int
kdata_buffer_copy(struct kdata* dst, const struct kdata* src)
{
	void* p;
	size_t	 i;
	int	 rc = 1;

	if (KDATA_BUFFER != dst->type)
		return(0);

	/*
	 * FIXME: use a pairbufsz-type of construct.
	 * We're not tied to any particular buffer size, so this should
	 * grow and shrink efficiently.
	 * Obviously, the current method is not efficient.
	 */
	if (src->pairsz > dst->pairsz) {
		dst->pairsz = src->pairsz;
		p = reallocarray(dst->pairs,
			dst->pairsz, sizeof(struct kpair));
		if (NULL == p)
			return(0);
		dst->pairs = (struct kpair*)p;
	}
	dst->pairsz = src->pairsz;

	if (dst->depsz)
		for (i = 0; 0 != rc && i < dst->pairsz; i++)
			rc = kdata_set(dst, i,
				src->pairs[i].x,
				src->pairs[i].y);
	else
		memcpy(dst->pairs, src->pairs,
			dst->pairsz * sizeof(struct kpair));

	return(rc);
}


struct kdata*
	kdata_hist_alloc(double rmin, double rmax, size_t bins)
{
	struct kdata* d;
	size_t		 i;

	assert(rmax > rmin);

	if (NULL == (d = (struct kdata*)calloc(1, sizeof(struct kdata))))
		return(NULL);

	d->refs = 1;
	d->pairsz = bins;
	d->pairs = (struct kpair*)calloc(d->pairsz, sizeof(struct kpair));
	if (NULL == d->pairs) {
		free(d);
		return(NULL);
	}

	for (i = 0; i < bins; i++)
		d->pairs[i].x = rmin +
		i / (double)bins * (rmax - rmin);

	d->type = KDATA_HIST;
	d->d.hist.rmin = rmin;
	d->d.hist.rmax = rmax;
	return(d);
}

static size_t
kdata_hist_checkrange(const struct kdata* d, double v)
{
	double	 frac;
	size_t 	 bucket;

	if (KDATA_HIST != d->type)
		return(-1);
	else if (v < d->d.hist.rmin)
		return(-1);
	else if (v >= d->d.hist.rmax)
		return(-1);

	frac = (v - d->d.hist.rmin) /
		(d->d.hist.rmax - d->d.hist.rmin);
	assert(frac >= 0.0 && frac < 1.0);
	bucket = floor((double)d->pairsz * frac);

	if ((size_t)bucket == d->pairsz - 1) {
		assert(d->pairs[bucket].x <= v);
	}
	else {
		assert(d->pairs[bucket].x <= v);
		assert(d->pairs[bucket + 1].x >= v);
	}

	return(bucket);
}

int
kdata_hist_add(struct kdata* d, double v, double val)
{
	size_t 	 bucket;
	double	 x, y;

	if ((bucket = kdata_hist_checkrange(d, v)) < 0)
		return(0);
	x = d->pairs[bucket].x;
	y = d->pairs[bucket].y + val;
	return(kdata_set(d, bucket, x, y));
}

int
kdata_hist_set(struct kdata* d, double v, double y)
{
	size_t 	 bucket;
	double 	 x;

	if ((bucket = kdata_hist_checkrange(d, v)) < 0)
		return(0);
	x = d->pairs[bucket].x;
	return(kdata_set(d, bucket, x, y));
}

static int
kdata_mean_set(struct kdata* d, size_t pos, double x, double y)
{
	double	 delta, delta_n, newy;
	void* p;

	assert(KDATA_MEAN == d->type);

	if (pos >= d->pairsz) {
		/*
		 * A note on this.
		 * Our only growable data source is the vector, which
		 * can only grow one at a time.
		 * Thus, if we attach to a vector, we'll never exceed
		 * this.
		 * If we have non-monotonically increasing data source
		 * sizes, this will need to be addressed.
		 * FIXME: this is very inefficient!
		 */
		assert(pos == d->pairsz);
		d->pairsz = pos + 1;
		p = reallocarray(d->pairs,
			d->pairsz, sizeof(struct kpair));
		if (NULL == p)
			return(0);
		d->pairs = (struct kpair*)p;
		p = reallocarray(d->d.mean.ns,
			d->pairsz, sizeof(size_t));
		if (NULL == p)
			return(0);
		d->d.mean.ns = (size_t*)p;
	}

	d->d.mean.ns[pos]++;
	delta = y - d->pairs[pos].y;
	delta_n = delta / (double)d->d.mean.ns[pos];
	newy = d->pairs[pos].y + delta_n;
	return(kdata_set(d, pos, x, newy));
}

struct kdata*
	kdata_mean_alloc(struct kdata* dep)
{
	struct kdata* d;
	size_t		 i;

	if (NULL == (d = (struct kdata*)calloc(1, sizeof(struct kdata))))
		return(NULL);

	d->refs = 1;
	d->type = KDATA_MEAN;
	if (NULL == dep)
		return(d);

	d->pairsz = dep->pairsz;
	d->pairs = (struct kpair*)calloc(d->pairsz, sizeof(struct kpair));
	d->d.mean.ns = (size_t*)calloc(d->pairsz, sizeof(size_t));
	if (NULL == d->pairs || NULL == d->d.mean.ns) {
		free(d->pairs);
		free(d->d.mean.ns);
		free(d);
		return(NULL);
	}
	kdata_dep_add(d, dep, kdata_mean_set);

	for (i = 0; i < dep->pairsz; i++)
		d->pairs[i].x = dep->pairs[i].x;

	return(d);
}

int
kdata_mean_attach(struct kdata* d, struct kdata* dep)
{
	void* p;
	size_t	 i;

	if (KDATA_MEAN != d->type)
		return(0);
	if (NULL == dep)
		return(1);

	if (d->pairsz < dep->pairsz) {
		p = reallocarray(d->pairs,
			dep->pairsz, sizeof(struct kpair));
		if (NULL == p)
			return(0);
		d->pairs = (struct kpair*)p;
		/* FIXME: don't loop, just do the math. */
		for (i = d->pairsz; i < dep->pairsz; i++)
			memset(&d->pairs[i], 0, sizeof(struct kpair));
		p = reallocarray(d->d.mean.ns,
			dep->pairsz, sizeof(size_t));
		if (NULL == p)
			return(0);
		d->d.mean.ns = (size_t*)p;
		for (i = d->pairsz; i < dep->pairsz; i++)
			d->d.mean.ns[i] = 0;
		d->pairsz = dep->pairsz;
		for (i = 0; i < dep->pairsz; i++)
			d->pairs[i].x = dep->pairs[i].x;
	}

	kdata_dep_add(d, dep, kdata_mean_set);
	return(1);
}

static int
kdata_stddev_set(struct kdata* d, size_t pos, double x, double y)
{
	double	 delta, delta_n, term1, newy;
	void* p;
	size_t	 n1;

	assert(KDATA_STDDEV == d->type);

	if (pos >= d->pairsz) {
		/*
		 * A note on this.
		 * Our only growable data source is the vector, which
		 * can only grow one at a time.
		 * Thus, if we attach to a vector, we'll never exceed
		 * this.
		 * If we have non-monotonically increasing data source
		 * sizes, this will need to be addressed.
		 * FIXME: this is very inefficient!
		 */
		assert(pos == d->pairsz);
		d->pairsz = pos + 1;
		p = reallocarray(d->pairs,
			d->pairsz, sizeof(struct kpair));
		if (NULL == p)
			return(0);
		d->pairs = (struct kpair*)p;
		p = reallocarray(d->d.stddev.ns,
			d->pairsz, sizeof(size_t));
		if (NULL == p)
			return(0);
		d->d.stddev.ns = (size_t*)p;
		p = reallocarray(d->d.stddev.m2s,
			d->pairsz, sizeof(double));
		if (NULL == p)
			return(0);
		d->d.stddev.m2s = (double*)p;
		p = reallocarray(d->d.stddev.m1s,
			d->pairsz, sizeof(double));
		if (NULL == p)
			return(0);
		d->d.stddev.m1s = (double*)p;
	}

	n1 = d->d.stddev.ns[pos]++;
	delta = y - d->d.stddev.m1s[pos];
	delta_n = delta / (double)d->d.stddev.ns[pos];
	term1 = delta * delta_n * (double)n1;

	d->d.stddev.m1s[pos] += delta_n;
	d->d.stddev.m2s[pos] += term1;
	if (d->d.stddev.ns[pos] < 2)
		newy = 0.0;
	else
		newy = sqrt(d->d.stddev.m2s[pos] /
			((double)d->d.stddev.ns[pos] - 1.0));

	return(kdata_set(d, pos, x, newy));
}

struct kdata*
	kdata_stddev_alloc(struct kdata* dep)
{
	struct kdata* d;
	size_t		 i;

	if (NULL == (d = (struct kdata*)calloc(1, sizeof(struct kdata))))
		return(NULL);

	d->refs = 1;
	d->type = KDATA_STDDEV;
	if (NULL == dep)
		return(d);

	d->pairsz = dep->pairsz;
	d->pairs = (struct kpair*)calloc(d->pairsz, sizeof(struct kpair));
	d->d.stddev.ns = (size_t*)calloc(d->pairsz, sizeof(size_t));
	d->d.stddev.m1s = (double*)calloc(d->pairsz, sizeof(double));
	d->d.stddev.m2s = (double*)calloc(d->pairsz, sizeof(double));
	if (NULL == d->pairs ||
		NULL == d->d.stddev.ns ||
		NULL == d->d.stddev.m1s ||
		NULL == d->d.stddev.m2s) {
		free(d->pairs);
		free(d->d.stddev.ns);
		free(d->d.stddev.m1s);
		free(d->d.stddev.m2s);
		free(d);
		return(NULL);
	}
	kdata_dep_add(d, dep, kdata_stddev_set);

	for (i = 0; i < dep->pairsz; i++)
		d->pairs[i].x = dep->pairs[i].x;

	return(d);
}

int
kdata_stddev_attach(struct kdata* d, struct kdata* dep)
{
	void* p;
	size_t	 i;

	if (KDATA_STDDEV != d->type)
		return(0);
	if (NULL == dep)
		return(1);

	if (d->pairsz < dep->pairsz) {
		p = reallocarray(d->pairs,
			dep->pairsz, sizeof(struct kpair));
		if (NULL == p)
			return(0);
		d->pairs = (struct kpair*)p;
		/* FIXME: don't loop, just do the math. */
		for (i = d->pairsz; i < dep->pairsz; i++)
			memset(&d->pairs[i], 0, sizeof(struct kpair));

		p = reallocarray(d->d.stddev.ns,
			dep->pairsz, sizeof(size_t));
		if (NULL == p)
			return(0);
		d->d.stddev.ns = (size_t*)p;
		for (i = d->pairsz; i < dep->pairsz; i++)
			d->d.stddev.ns[i] = 0;

		p = reallocarray(d->d.stddev.m1s,
			dep->pairsz, sizeof(double));
		if (NULL == p)
			return(0);
		d->d.stddev.m1s = (double*)p;
		for (i = d->pairsz; i < dep->pairsz; i++)
			d->d.stddev.m1s[i] = 0.0;

		p = reallocarray(d->d.stddev.m2s,
			dep->pairsz, sizeof(double));
		if (NULL == p)
			return(0);
		d->d.stddev.m2s = (double*)p;
		for (i = d->pairsz; i < dep->pairsz; i++)
			d->d.stddev.m2s[i] = 0.0;

		d->pairsz = dep->pairsz;
		for (i = 0; i < dep->pairsz; i++)
			d->pairs[i].x = dep->pairs[i].x;
	}

	kdata_dep_add(d, dep, kdata_stddev_set);
	return(1);
}

struct kdata*
	kdata_vector_alloc(size_t step)
{
	struct kdata* d;

	if (NULL == (d = (struct kdata*)calloc(1, sizeof(struct kdata))))
		return(NULL);

	d->refs = 1;
	d->type = KDATA_VECTOR;
	d->d.vector.stepsz = step;
	return(d);
}

int
kdata_vector_append(struct kdata* d, double x, double y)
{
	void* p;

	if (KDATA_VECTOR != d->type)
		return(0);

	if (d->pairsz + 1 >= d->d.vector.pairbufsz) {
		while (d->pairsz + 1 >= d->d.vector.pairbufsz)
			d->d.vector.pairbufsz += d->d.vector.stepsz;
		assert(d->d.vector.pairbufsz > d->pairsz + 1);
		p = reallocarray(d->pairs,
			d->d.vector.pairbufsz, sizeof(struct kpair));
		if (NULL == p)
			return(0);
		d->pairs = (struct kpair*)p;
	}

	d->pairsz++;
	return(kdata_set(d, d->pairsz - 1, x, y));
}

int
kdata_vector_set(struct kdata* d, size_t v, double x, double y)
{

	if (KDATA_VECTOR != d->type || v >= d->pairsz)
		return(0);
	return(kdata_set(d, v, x, y));
}


// draw
#if 1 

 /*
  * Simple function to check that the double-precision values in the
  * kpair are valid: normal (or 0.0) values.
  */
static inline int
kpair_vrfy(const struct kpair* data)
{

	if (0.0 != data->x && !isnormal(data->x))
		return(0);
	if (0.0 != data->y && !isnormal(data->y))
		return(0);
	return(1);
}

/*
 * Set the pair "kp" to the value at position "pos", which depends upon
 * the smoothing type (if stipulated).
 * The value "kp" SHOULD NOT be cleared between invocations, as some
 * data streams (e.g., KSMOOTH_CDF) accumulate it.
 */
static void
kpair_set(const struct kplotdat* d, size_t pos, struct kpair* kp)
{
	size_t		 j, sz, samps;
	size_t		 start;

	switch (d->smthtype) {
	case (KSMOOTH_CDF):
		kp->x = d->datas[0]->pairs[pos].x;
		kp->y += d->datas[0]->pairs[pos].y / d->sum;
		break;
	case (KSMOOTH_PMF):
		kp->x = d->datas[0]->pairs[pos].x;
		kp->y = d->datas[0]->pairs[pos].y / d->sum;
		break;
	case (KSMOOTH_MOVAVG):
		*kp = d->datas[0]->pairs[pos];
		samps = d->smth.movsamples / 2;
		start = pos - samps;
		sz = pos + samps;
		if (start < 0 || sz >= d->datas[0]->pairsz)
			break;
		for (kp->y = 0.0, j = start; j <= sz; j++) {
			if (!kpair_vrfy(&d->datas[0]->pairs[j]))
				break;
			kp->y += d->datas[0]->pairs[j].y;
		}
		kp->y /= (double)d->smth.movsamples;
		if (j <= sz)
			*kp = d->datas[0]->pairs[pos];
		break;
	default:
		*kp = d->datas[0]->pairs[pos];
		break;
	}
}

/*
 * Accumulate extrema where we add and subtract the second data source
 * from the first, e.g., in a graph with mean and standard deviation.
 */
static void
kdata_extrema_yerr(struct kplotdat* d, struct kplotctx* ctx)
{
	size_t	 	 i, sz;
	struct kpair* p, * err;

	assert(d->datasz > 1);
	p = d->datas[0]->pairs;
	err = d->datas[1]->pairs;

	/* Truncate to the smaller of the two pair lengths. */
	sz = d->datas[0]->pairsz < d->datas[1]->pairsz ?
		d->datas[0]->pairsz : d->datas[1]->pairsz;

	for (i = 0; i < sz; i++) {
		/* Both must be valid. */
		if (!(kpair_vrfy(&p[i]) && kpair_vrfy(&err[i])))
			continue;

		/*
		 * Since the error can be negative, check in both
		 * directions from the basis point.
		 */
		if (p[i].x < ctx->minv.x)
			ctx->minv.x = p[i].x;
		if (p[i].x > ctx->maxv.x)
			ctx->maxv.x = p[i].x;
		if (p[i].y - err[i].y < ctx->minv.y)
			ctx->minv.y = p[i].y - err[i].y;
		if (p[i].y + err[i].y < ctx->minv.y)
			ctx->minv.y = p[i].y + err[i].y;
		if (p[i].y - err[i].y > ctx->maxv.y)
			ctx->maxv.y = p[i].y - err[i].y;
		if (p[i].y + err[i].y > ctx->maxv.y)
			ctx->maxv.y = p[i].y + err[i].y;
	}
}

/*
 * Accumulate extrema of a single data source.
 */
static void
kdata_extrema_single(struct kplotdat* d, struct kplotctx* ctx)
{
	size_t	 	 i;
	double		 max;
	struct kpair	 kp;

	max = -DBL_MAX;
	d->sum = 0.0;
	memset(&kp, 0, sizeof(struct kpair));
	for (i = 0; i < d->datas[0]->pairsz; i++) {
		if (!kpair_vrfy(&d->datas[0]->pairs[i]))
			continue;
		kpair_set(d, i, &kp);
		if (KSMOOTH_CDF == d->smthtype)
			d->sum += d->datas[0]->pairs[i].y;
		if (KSMOOTH_PMF == d->smthtype) {
			d->sum += d->datas[0]->pairs[i].y;
			if (d->datas[0]->pairs[i].y > max)
				max = d->datas[0]->pairs[i].y;
		}
		if (kp.x < ctx->minv.x)
			ctx->minv.x = kp.x;
		if (kp.x > ctx->maxv.x)
			ctx->maxv.x = kp.x;
		switch (d->smthtype) {
		case (KSMOOTH_CDF):
		case (KSMOOTH_PMF):
			break;
		default:
			if (kp.y < ctx->minv.y)
				ctx->minv.y = kp.y;
			if (kp.y > ctx->maxv.y)
				ctx->maxv.y = kp.y;
			break;
		}
	}
	if (KSMOOTH_CDF == d->smthtype) {
		if (0.0 < ctx->minv.y)
			ctx->minv.y = 0.0;
		if (1.0 > ctx->maxv.y)
			ctx->maxv.y = 1.0;
	}
	else if (KSMOOTH_PMF == d->smthtype) {
		if (0.0 < ctx->minv.y)
			ctx->minv.y = 0.0;
		if (max / d->sum > ctx->maxv.y)
			ctx->maxv.y = max / d->sum;
	}
}


/*
 * Adjust a plot point to be within the graphing space.
 * The graphing space is the same for all data sources in the plot, so
 * we simply take the point and adjust it.
 * NOTE: this might fall outside of the drawable area.
 * That's ok: we'll discard it (if points) or clip it (lines).
 */
static inline void
kpoint_to_real(const struct kpair* data, struct kpair* real,
	const struct kpair* minv, const struct kpair* maxv,
	double w, double h)
{

	real->x = maxv->x == minv->x ? 0.0 :
		w * (data->x - minv->x) / (maxv->x - minv->x);
	real->y = maxv->y == minv->y ? h :
		h - h * (data->y - minv->y) / (maxv->y - minv->y);
}

/*
 * Verify that a given point is real (in terms of floating-point) and if
 * so, convert it to the plot space.
 */
static int
kplotctx_point_to_real(const struct kpair* data,
	struct kpair* real, const struct kplotctx* ctx)
{

	if (!kpair_vrfy(data))
		return(0);
	kpoint_to_real(data, real,
		&ctx->minv, &ctx->maxv, ctx->w, ctx->h);
	return(1);
}

/*
 * Draw a circle (arc) to the plot IFF it happens to fall within the
 * boundaries we set with the plot, otherwise do nothing.
 */
static void
kplot_arc(const struct kpair* kp,
	const struct kplotpoint* p, struct kplotctx* ctx)
{
	struct kpair	 pair;

	if (kp->x < ctx->minv.x || kp->x > ctx->maxv.x)
		return;
	if (kp->y < ctx->minv.y || kp->y > ctx->maxv.y)
		return;
	if (0 == kplotctx_point_to_real(kp, &pair, ctx))
		return;
	vkvg_arc(ctx->cr, pair.x, pair.y, p->radius, 0, 2 * M_PI);
	vkvg_stroke(ctx->cr);
}

static void
kplot_mark(const struct kpair* kp,
	const struct kplotpoint* p, struct kplotctx* ctx)
{
	struct kpair	 pair;

	if (kp->x < ctx->minv.x || kp->x > ctx->maxv.x)
		return;
	if (kp->y < ctx->minv.y || kp->y > ctx->maxv.y)
		return;
	if (0 == kplotctx_point_to_real(kp, &pair, ctx))
		return;

	vkvg_move_to(ctx->cr, pair.x - p->radius, pair.y - p->radius);
	vkvg_line_to(ctx->cr, pair.x + p->radius, pair.y + p->radius);
	vkvg_move_to(ctx->cr, pair.x - p->radius, pair.y + p->radius);
	vkvg_line_to(ctx->cr, pair.x + p->radius, pair.y - p->radius);
	vkvg_stroke(ctx->cr);
}

/*
 * When drawing points, arrange the drawing space.
 * It's the responsibility of kplot_arc() to avoid points that would be
 * drawn outside of this range.
 * You must call vkvg_restore(ctx->cr) to symmetrise.
 */
static void
ksubwin_points(struct kplotctx* ctx)
{

	vkvg_save(ctx->cr);
	vkvg_translate(ctx->cr, ctx->offs.x, ctx->offs.y);
}

/*
 * When drawing lines (or bars), create a subwindow large enough for
 * lines within the context dimensions.
 * Lines drawn outside of the subwindow will be clipped.
 * You must call vkvg_restore(ctx->cr) to symmetrise.
 */
static void
ksubwin_lines(struct kplotctx* ctx, const struct kdatacfg* dat)
{
	double		 width;

	width = dat->line.sz / 2.0;
	vkvg_save(ctx->cr);
	vkvg_translate(ctx->cr,
		ctx->offs.x - width,
		ctx->offs.y - width);
	vkvg_rectangle(ctx->cr, 0, 0,
		ctx->dims.x + width * 2,
		ctx->dims.y + width * 2);
	vkvg_clip(ctx->cr);
	vkvg_translate(ctx->cr, width, width);
}

static size_t
kplotctx_draw_yerrline_start(struct kplotctx* ctx,
	const struct kplotdat* d, size_t* end)
{
	size_t	 start;

	/* Overlap between both point sets. */
	*end = d->datas[0]->pairsz < d->datas[1]->pairsz ?
		d->datas[0]->pairsz : d->datas[1]->pairsz;

	/* Skip past bad points to get to initial. */
	for (start = 0; start < *end; start++)
		if (kpair_vrfy(&d->datas[0]->pairs[start]) &&
			kpair_vrfy(&d->datas[1]->pairs[start]))
			return(start);

	return(*end);
}

static void
kplotctx_draw_yerrline_basepoints(struct kplotctx* ctx,
	size_t start, size_t end, const struct kplotdat* d)
{
	size_t		 i;

	ksubwin_points(ctx);
	kplotctx_point_init(ctx, &d->cfgs[0].point);
	for (i = start; i < end; i++) {
		if (!(kpair_vrfy(&d->datas[0]->pairs[i]) &&
			kpair_vrfy(&d->datas[1]->pairs[i])))
			continue;
		kplot_arc(&d->datas[0]->pairs[i],
			&d->cfgs[0].point, ctx);
	}
	vkvg_restore(ctx->cr);
}

static void
kplotctx_draw_yerrline_basemarks(struct kplotctx* ctx,
	size_t start, size_t end, const struct kplotdat* d)
{
	size_t		 i;

	ksubwin_points(ctx);
	kplotctx_point_init(ctx, &d->cfgs[0].point);
	for (i = start; i < end; i++) {
		if (!(kpair_vrfy(&d->datas[0]->pairs[i]) &&
			kpair_vrfy(&d->datas[1]->pairs[i])))
			continue;
		kplot_mark(&d->datas[0]->pairs[i],
			&d->cfgs[0].point, ctx);
	}
	vkvg_restore(ctx->cr);
}

static void
kplotctx_draw_yerrline_pairbars(struct kplotctx* ctx,
	size_t start, size_t end, const struct kplotdat* d)
{
	size_t	 	 i;
	struct kpair	 bot, top, pair;
	int		 rc;

	ksubwin_lines(ctx, &d->cfgs[1]);
	kplotctx_line_init(ctx, &d->cfgs[1].line);
	for (i = start; i < end; i++) {
		if (!(kpair_vrfy(&d->datas[0]->pairs[i]) &&
			kpair_vrfy(&d->datas[1]->pairs[i])))
			continue;

		bot.x = top.x = d->datas[0]->pairs[i].x;
		bot.y = d->datas[0]->pairs[i].y -
			d->datas[1]->pairs[i].y;
		top.y = d->datas[0]->pairs[i].y +
			d->datas[1]->pairs[i].y;

		rc = kplotctx_point_to_real(&bot, &pair, ctx);
		assert(0 != rc);
		vkvg_move_to(ctx->cr, pair.x, pair.y);

		rc = kplotctx_point_to_real(&top, &pair, ctx);
		assert(0 != rc);
		vkvg_line_to(ctx->cr, pair.x, pair.y);
	}
	vkvg_stroke(ctx->cr);
	vkvg_restore(ctx->cr);
}

static void
kplotctx_draw_yerrline_pairpoints(struct kplotctx* ctx,
	size_t start, size_t end, const struct kplotdat* d)
{
	size_t	 	 i;
	struct kpair	 orig;

	ksubwin_points(ctx);
	kplotctx_point_init(ctx, &d->cfgs[1].point);
	for (i = start; i < end; i++) {
		if (!(kpair_vrfy(&d->datas[0]->pairs[i]) &&
			kpair_vrfy(&d->datas[1]->pairs[i])))
			continue;
		orig.x = d->datas[0]->pairs[i].x;
		orig.y = d->datas[0]->pairs[i].y +
			d->datas[1]->pairs[i].y;
		kplot_arc(&orig, &d->cfgs[1].point, ctx);
	}

	kplotctx_point_init(ctx, &d->cfgs[1].point);
	for (i = start; i < end; i++) {
		if (!(kpair_vrfy(&d->datas[0]->pairs[i]) &&
			kpair_vrfy(&d->datas[1]->pairs[i])))
			continue;
		orig.x = d->datas[0]->pairs[i].x;
		orig.y = d->datas[0]->pairs[i].y -
			d->datas[1]->pairs[i].y;
		kplot_arc(&orig, &d->cfgs[1].point, ctx);
	}

	vkvg_restore(ctx->cr);
}

static void
kplotctx_draw_yerrline_pairmarks(struct kplotctx* ctx,
	size_t start, size_t end, const struct kplotdat* d)
{
	size_t	 	 i;
	struct kpair	 orig;

	ksubwin_points(ctx);
	kplotctx_point_init(ctx, &d->cfgs[1].point);
	for (i = start; i < end; i++) {
		if (!(kpair_vrfy(&d->datas[0]->pairs[i]) &&
			kpair_vrfy(&d->datas[1]->pairs[i])))
			continue;
		orig.x = d->datas[0]->pairs[i].x;
		orig.y = d->datas[0]->pairs[i].y +
			d->datas[1]->pairs[i].y;
		kplot_mark(&orig, &d->cfgs[1].point, ctx);
	}

	kplotctx_point_init(ctx, &d->cfgs[1].point);
	for (i = start; i < end; i++) {
		if (!(kpair_vrfy(&d->datas[0]->pairs[i]) &&
			kpair_vrfy(&d->datas[1]->pairs[i])))
			continue;
		orig.x = d->datas[0]->pairs[i].x;
		orig.y = d->datas[0]->pairs[i].y -
			d->datas[1]->pairs[i].y;
		kplot_mark(&orig, &d->cfgs[1].point, ctx);
	}

	vkvg_restore(ctx->cr);
}

static void
kplotctx_draw_yerrline_baselines(struct kplotctx* ctx,
	size_t start, size_t end, const struct kplotdat* d)
{
	size_t		 i;
	struct kpair	 pair;
	int		 rc;

	assert(d->datasz > 1);
	ksubwin_lines(ctx, &d->cfgs[0]);
	kplotctx_line_init(ctx, &d->cfgs[0].line);
	rc = kplotctx_point_to_real
	(&d->datas[0]->pairs[start], &pair, ctx);
	assert(0 != rc);
	vkvg_move_to(ctx->cr, pair.x, pair.y);
	for (i = start; i < end; i++) {
		if (!(kpair_vrfy(&d->datas[0]->pairs[i]) &&
			kpair_vrfy(&d->datas[1]->pairs[i])))
			continue;
		rc = kplotctx_point_to_real
		(&d->datas[0]->pairs[i], &pair, ctx);
		assert(0 != rc);
		vkvg_line_to(ctx->cr, pair.x, pair.y);
	}
	vkvg_stroke(ctx->cr);
	vkvg_restore(ctx->cr);
}

static void
kplotctx_draw_yerrline_pairlines(struct kplotctx* ctx,
	size_t start, size_t end, const struct kplotdat* d)
{
	struct kpair	 orig, pair;
	size_t		 i;
	int		 rc;

	ksubwin_lines(ctx, &d->cfgs[1]);
	kplotctx_line_init(ctx, &d->cfgs[1].line);
	orig.x = d->datas[0]->pairs[start].x;
	orig.y = d->datas[0]->pairs[start].y +
		d->datas[1]->pairs[start].y;
	rc = kplotctx_point_to_real(&orig, &pair, ctx);
	assert(0 != rc);
	vkvg_move_to(ctx->cr, pair.x, pair.y);
	for (i = start; i < end; i++) {
		if (!(kpair_vrfy(&d->datas[0]->pairs[i]) &&
			kpair_vrfy(&d->datas[1]->pairs[i])))
			continue;
		orig.x = d->datas[0]->pairs[i].x;
		orig.y = d->datas[0]->pairs[i].y +
			d->datas[1]->pairs[i].y;
		rc = kplotctx_point_to_real(&orig, &pair, ctx);
		assert(0 != rc);
		vkvg_line_to(ctx->cr, pair.x, pair.y);
	}
	vkvg_stroke(ctx->cr);

	kplotctx_line_init(ctx, &d->cfgs[1].line);
	orig.x = d->datas[0]->pairs[start].x;
	orig.y = d->datas[0]->pairs[start].y -
		d->datas[1]->pairs[start].y;
	kplotctx_point_to_real(&orig, &pair, ctx);
	vkvg_move_to(ctx->cr, pair.x, pair.y);
	for (i = start; i < end; i++) {
		if (!(kpair_vrfy(&d->datas[0]->pairs[i]) &&
			kpair_vrfy(&d->datas[1]->pairs[i])))
			continue;
		orig.x = d->datas[0]->pairs[i].x;
		orig.y = d->datas[0]->pairs[i].y -
			d->datas[1]->pairs[i].y;
		rc = kplotctx_point_to_real(&orig, &pair, ctx);
		assert(0 != rc);
		vkvg_line_to(ctx->cr, pair.x, pair.y);
	}
	vkvg_stroke(ctx->cr);
	vkvg_restore(ctx->cr);
}

static void
kplotctx_draw_lines(struct kplotctx* ctx, const struct kplotdat* d)
{
	size_t		 i;
	struct kpair	 kp, pair;
	int		 rc;

	ksubwin_lines(ctx, &d->cfgs[0]);
	memset(&kp, 0, sizeof(struct kpair));
	for (i = 0; i < d->datas[0]->pairsz; i++) {
		kpair_set(d, i, &kp);
		if (kplotctx_point_to_real(&kp, &pair, ctx))
			break;
	}

	if (i == d->datas[0]->pairsz)
		goto out;

	kplotctx_line_init(ctx, &d->cfgs[0].line);
	vkvg_move_to(ctx->cr, pair.x, pair.y);
	memset(&kp, 0, sizeof(struct kpair));
	for (; i < d->datas[0]->pairsz; i++) {
		if (!kpair_vrfy(&d->datas[0]->pairs[i]))
			continue;
		kpair_set(d, i, &kp);
		rc = kplotctx_point_to_real(&kp, &pair, ctx);
		if (!rc)
			continue;
		vkvg_line_to(ctx->cr, pair.x, pair.y);
	}
	vkvg_stroke(ctx->cr);
out:
	vkvg_restore(ctx->cr);
}

static void
kplotctx_draw_points(struct kplotctx* ctx, const struct kplotdat* d)
{
	size_t		 i;
	struct kpair	 kp;

	ksubwin_points(ctx);
	memset(&kp, 0, sizeof(struct kpair));
	kplotctx_point_init(ctx, &d->cfgs[0].point);
	for (i = 0; i < d->datas[0]->pairsz; i++) {
		if (!kpair_vrfy(&d->datas[0]->pairs[i]))
			continue;
		kpair_set(d, i, &kp);
		kplot_arc(&kp, &d->cfgs[0].point, ctx);
	}
	vkvg_restore(ctx->cr);
}

static void
kplotctx_draw_marks(struct kplotctx* ctx, const struct kplotdat* d)
{
	size_t		 i;
	struct kpair	 kp;

	ksubwin_points(ctx);
	memset(&kp, 0, sizeof(struct kpair));
	kplotctx_point_init(ctx, &d->cfgs[0].point);
	for (i = 0; i < d->datas[0]->pairsz; i++) {
		if (!kpair_vrfy(&d->datas[0]->pairs[i]))
			continue;
		kpair_set(d, i, &kp);
		kplot_mark(&kp, &d->cfgs[0].point, ctx);
	}
	vkvg_restore(ctx->cr);
}

void
kplotfont_defaults(struct kplotfont* font)
{

	memset(font, 0, sizeof(struct kplotfont));

	/* Point 12 size serif font. */
	font->family = "serif";
	font->sz = 16.0;
	font->slant = 0;// vkvg_FONT_SLANT_NORMAL;
	font->weight = 0;// vkvg_FONT_WEIGHT_NORMAL;
}

void
kplotcfg_defaults(struct kplotcfg* cfg)
{

	memset(cfg, 0, sizeof(struct kplotcfg));

	/* Five left and bottom grey tic labels. */
	kplotfont_defaults(&cfg->ticlabelfont);
	cfg->ticlabel = TICLABEL_LEFT | TICLABEL_BOTTOM;
	cfg->xticlabelpad = cfg->yticlabelpad = 15.0;
	cfg->xtics = cfg->ytics = 5;

	/* A bit of margin. */
	cfg->margin = MARGIN_ALL;
	cfg->marginsz = 15.0;

	/* Innie tics, grey. */
	cfg->tic = TIC_LEFT_IN | TIC_BOTTOM_IN;
	cfg->ticline.len = 5.0;
	cfg->ticline.sz = 1.0;

	/* Grid line: dotted, grey. */
	cfg->grid = GRID_ALL;
	cfg->gridline.sz = 1.0;
	cfg->gridline.dashes[0] = 1.0;
	cfg->gridline.dashes[1] = 4.0;
	cfg->gridline.dashesz = 2;

	/* Border line: solid, grey. */
	cfg->border = BORDER_LEFT | BORDER_BOTTOM;
	cfg->borderline.sz = 1.0;

	/* Black axis labels. */
	kplotfont_defaults(&cfg->axislabelfont);
	cfg->xaxislabelpad = cfg->yaxislabelpad = 15.0;
}

void
kplotctx_draw(struct kplotctx* ctx, struct kplot* p, double w,
	double h, VkvgContext cr)
{
	size_t	 	 i, start, end;
	struct kplotdat* d;
	struct kplotccfg defs[7];

	memset(ctx, 0, sizeof(struct kplotctx));

	ctx->w = w;
	ctx->h = h;
	ctx->cr = cr;
	ctx->minv.x = ctx->minv.y = DBL_MAX;
	ctx->maxv.x = ctx->maxv.y = -DBL_MAX;
	ctx->cfg = p->cfg;

	if (KPLOTCTYPE_DEFAULT == ctx->cfg.borderline.clr.type) {
		ctx->cfg.borderline.clr.type = KPLOTCTYPE_RGBA;
		ctx->cfg.borderline.clr.rgba[0] = 0.0;
		ctx->cfg.borderline.clr.rgba[1] = 0.0;
		ctx->cfg.borderline.clr.rgba[2] = 0.0;
		ctx->cfg.borderline.clr.rgba[3] = 1.0;
	}

	if (KPLOTCTYPE_DEFAULT == ctx->cfg.axislabelfont.clr.type) {
		ctx->cfg.axislabelfont.clr.type = KPLOTCTYPE_RGBA;
		ctx->cfg.axislabelfont.clr.rgba[0] = 0.0;
		ctx->cfg.axislabelfont.clr.rgba[1] = 0.0;
		ctx->cfg.axislabelfont.clr.rgba[2] = 0.0;
		ctx->cfg.axislabelfont.clr.rgba[3] = 1.0;
	}

	if (KPLOTCTYPE_DEFAULT == ctx->cfg.ticline.clr.type) {
		ctx->cfg.ticline.clr.type = KPLOTCTYPE_RGBA;
		ctx->cfg.ticline.clr.rgba[0] = 0.0;
		ctx->cfg.ticline.clr.rgba[1] = 0.0;
		ctx->cfg.ticline.clr.rgba[2] = 0.0;
		ctx->cfg.ticline.clr.rgba[3] = 1.0;
	}

	if (KPLOTCTYPE_DEFAULT == ctx->cfg.gridline.clr.type) {
		ctx->cfg.gridline.clr.type = KPLOTCTYPE_RGBA;
		ctx->cfg.gridline.clr.rgba[0] = 0.5;
		ctx->cfg.gridline.clr.rgba[1] = 0.5;
		ctx->cfg.gridline.clr.rgba[2] = 0.5;
		ctx->cfg.gridline.clr.rgba[3] = 1.0;
	}

	if (KPLOTCTYPE_DEFAULT == ctx->cfg.ticlabelfont.clr.type) {
		ctx->cfg.ticlabelfont.clr.type = KPLOTCTYPE_RGBA;
		ctx->cfg.ticlabelfont.clr.rgba[0] = 0.5;
		ctx->cfg.ticlabelfont.clr.rgba[1] = 0.5;
		ctx->cfg.ticlabelfont.clr.rgba[2] = 0.5;
		ctx->cfg.ticlabelfont.clr.rgba[3] = 1.0;
	}

	if (0 == ctx->cfg.clrsz) {
		ctx->cfg.clrs = defs;
		ctx->cfg.clrsz = 7;
		for (i = 0; i < ctx->cfg.clrsz; i++) {
			ctx->cfg.clrs[i].type = KPLOTCTYPE_RGBA;
			ctx->cfg.clrs[i].rgba[3] = 1.0;
		}
		ctx->cfg.clrs[0].rgba[0] = 0x94 / 255.0;
		ctx->cfg.clrs[0].rgba[1] = 0x04 / 255.0;
		ctx->cfg.clrs[0].rgba[2] = 0xd3 / 255.0;
		ctx->cfg.clrs[1].rgba[0] = 0x00 / 255.0;
		ctx->cfg.clrs[1].rgba[1] = 0x9e / 255.0;
		ctx->cfg.clrs[1].rgba[2] = 0x73 / 255.0;
		ctx->cfg.clrs[2].rgba[0] = 0x56 / 255.0;
		ctx->cfg.clrs[2].rgba[1] = 0xb4 / 255.0;
		ctx->cfg.clrs[2].rgba[2] = 0xe9 / 255.0;
		ctx->cfg.clrs[3].rgba[0] = 0xe6 / 255.0;
		ctx->cfg.clrs[3].rgba[1] = 0x9f / 255.0;
		ctx->cfg.clrs[3].rgba[2] = 0x00 / 255.0;
		ctx->cfg.clrs[4].rgba[0] = 0xf0 / 255.0;
		ctx->cfg.clrs[4].rgba[1] = 0xe4 / 255.0;
		ctx->cfg.clrs[4].rgba[2] = 0x42 / 255.0;
		ctx->cfg.clrs[5].rgba[0] = 0x00 / 255.0;
		ctx->cfg.clrs[5].rgba[1] = 0x72 / 255.0;
		ctx->cfg.clrs[5].rgba[2] = 0xb2 / 255.0;
		ctx->cfg.clrs[6].rgba[0] = 0xe5 / 255.0;
		ctx->cfg.clrs[6].rgba[1] = 0x1e / 255.0;
		ctx->cfg.clrs[6].rgba[2] = 0x10 / 255.0;
	}

	for (i = 0; i < p->datasz; i++) {
		d = &p->datas[i];
		switch (d->stype) {
		case (KPLOTS_YERRORBAR):
		case (KPLOTS_YERRORLINE):
			kdata_extrema_yerr(d, ctx);
			break;
		case (KPLOTS_SINGLE):
			kdata_extrema_single(d, ctx);
			break;
		}
	}

	if (EXTREMA_XMIN & ctx->cfg.extrema)
		ctx->minv.x = ctx->cfg.extrema_xmin;
	if (EXTREMA_YMIN & ctx->cfg.extrema)
		ctx->minv.y = ctx->cfg.extrema_ymin;
	if (EXTREMA_XMAX & ctx->cfg.extrema)
		ctx->maxv.x = ctx->cfg.extrema_xmax;
	if (EXTREMA_YMAX & ctx->cfg.extrema)
		ctx->maxv.y = ctx->cfg.extrema_ymax;

	if (ctx->minv.x > ctx->maxv.x)
		ctx->minv.x = ctx->maxv.x = 0.0;
	if (ctx->minv.y > ctx->maxv.y)
		ctx->minv.y = ctx->maxv.y = 0.0;

	kplotctx_margin_init(ctx);
	kplotctx_label_init(ctx);
	kplotctx_grid_init(ctx);
	kplotctx_border_init(ctx);
	kplotctx_tic_init(ctx);

	ctx->h = ctx->dims.y;
	ctx->w = ctx->dims.x;

	for (i = 0; i < p->datasz; i++) {
		d = &p->datas[i];
		switch (d->stype) {
		case (KPLOTS_SINGLE):
			switch (d->types[0]) {
			case (KPLOT_POINTS):
				kplotctx_draw_points(ctx, d);
				break;
			case (KPLOT_MARKS):
				kplotctx_draw_marks(ctx, d);
				break;
			case (KPLOT_LINES):
				kplotctx_draw_lines(ctx, d);
				break;
			case (KPLOT_LINESPOINTS):
				kplotctx_draw_points(ctx, d);
				kplotctx_draw_lines(ctx, d);
				break;
			case (KPLOT_LINESMARKS):
				kplotctx_draw_marks(ctx, d);
				kplotctx_draw_lines(ctx, d);
				break;
			default:
				abort();
				break;
			}
			break;
		case (KPLOTS_YERRORBAR):
		case (KPLOTS_YERRORLINE):
			start = kplotctx_draw_yerrline_start
			(ctx, d, &end);
			if (start == end)
				break;
			assert(d->datasz > 1);
			switch (d->types[0]) {
			case (KPLOT_POINTS):
				kplotctx_draw_yerrline_basepoints
				(ctx, start, end, d);
				break;
			case (KPLOT_MARKS):
				kplotctx_draw_yerrline_basemarks
				(ctx, start, end, d);
				break;
			case (KPLOT_LINES):
				kplotctx_draw_yerrline_baselines
				(ctx, start, end, d);
				break;
			case (KPLOT_LINESPOINTS):
				kplotctx_draw_yerrline_basepoints
				(ctx, start, end, d);
				kplotctx_draw_yerrline_baselines
				(ctx, start, end, d);
				break;
			case (KPLOT_LINESMARKS):
				kplotctx_draw_yerrline_basemarks
				(ctx, start, end, d);
				kplotctx_draw_yerrline_baselines
				(ctx, start, end, d);
				break;
			default:
				abort();
				break;
			}
			switch (p->datas[i].types[1]) {
			case (KPLOT_POINTS):
				kplotctx_draw_yerrline_pairpoints
				(ctx, start, end, d);
				break;
			case (KPLOT_MARKS):
				kplotctx_draw_yerrline_pairmarks
				(ctx, start, end, d);
				break;
			case (KPLOT_LINES):
				kplotctx_draw_yerrline_pairlines
				(ctx, start, end, d);
				break;
			case (KPLOT_LINESPOINTS):
				kplotctx_draw_yerrline_pairpoints
				(ctx, start, end, d);
				kplotctx_draw_yerrline_pairlines
				(ctx, start, end, d);
				break;
			case (KPLOT_LINESMARKS):
				kplotctx_draw_yerrline_pairmarks
				(ctx, start, end, d);
				kplotctx_draw_yerrline_pairlines
				(ctx, start, end, d);
				break;
			default:
				abort();
				break;
			}
			if (KPLOTS_YERRORBAR == d->stype)
				kplotctx_draw_yerrline_pairbars
				(ctx, start, end, d);
			break;
		default:
			break;
		}
	}
}

void
kplot_draw(struct kplot* p, double w, double h, VkvgContext cr)
{
	struct kplotctx	 ctx;

	kplotctx_draw(&ctx, p, w, h, cr);
}

int
kplotctx_translate(const struct kplotctx* ctx, double vkvg_x,
	double vkvg_y, double* data_x, double* data_y)
{
	double	n_x, n_y, range_x, range_y;

	/*
	 * Normalise coordinates in the unit interval. Don't allow
	 * division by zero.  Don't let through values outside of the
	 * mappable space.
	 */

	if (data_x != NULL) {
		if (fpclassify(ctx->dims.x) == FP_ZERO)
			return -1;
		n_x = (vkvg_x - ctx->offs.x) / ctx->dims.x;
		if (n_x < 0.0 || n_x > 1.0)
			return 0;
		range_x = ctx->cfg.extrema_xmax - ctx->cfg.extrema_xmin;
		*data_x = n_x * range_x + ctx->cfg.extrema_xmin;
	}

	if (data_x != NULL) {
		if (fpclassify(ctx->dims.y) == FP_ZERO)
			return -1;
		n_y = 1.0 - (vkvg_y - ctx->offs.y) / ctx->dims.y;
		if (n_y < 0.0 || n_y > 1.0)
			return 0;
		range_y = ctx->cfg.extrema_ymax - ctx->cfg.extrema_ymin;
		*data_y = n_y * range_y + ctx->cfg.extrema_ymin;
	}

	return 1;
}

int
kplotcfg_default_palette(struct kplotccfg** pp, size_t* szp)
{
	size_t		 i;

	*szp = 7;
	if (NULL == (*pp = (struct kplotccfg*)calloc(*szp, sizeof(struct kplotccfg))))
		return(0);

	for (i = 0; i < *szp; i++) {
		(*pp)[i].type = KPLOTCTYPE_RGBA;
		(*pp)[i].rgba[3] = 1.0;
	}

	(*pp)[0].rgba[0] = 0x94 / 255.0;
	(*pp)[0].rgba[1] = 0x04 / 255.0;
	(*pp)[0].rgba[2] = 0xd3 / 255.0;
	(*pp)[1].rgba[0] = 0x00 / 255.0;
	(*pp)[1].rgba[1] = 0x9e / 255.0;
	(*pp)[1].rgba[2] = 0x73 / 255.0;
	(*pp)[2].rgba[0] = 0x56 / 255.0;
	(*pp)[2].rgba[1] = 0xb4 / 255.0;
	(*pp)[2].rgba[2] = 0xe9 / 255.0;
	(*pp)[3].rgba[0] = 0xe6 / 255.0;
	(*pp)[3].rgba[1] = 0x9f / 255.0;
	(*pp)[3].rgba[2] = 0x00 / 255.0;
	(*pp)[4].rgba[0] = 0xf0 / 255.0;
	(*pp)[4].rgba[1] = 0xe4 / 255.0;
	(*pp)[4].rgba[2] = 0x42 / 255.0;
	(*pp)[5].rgba[0] = 0x00 / 255.0;
	(*pp)[5].rgba[1] = 0x72 / 255.0;
	(*pp)[5].rgba[2] = 0xb2 / 255.0;
	(*pp)[6].rgba[0] = 0xe5 / 255.0;
	(*pp)[6].rgba[1] = 0x1e / 255.0;
	(*pp)[6].rgba[2] = 0x10 / 255.0;

	return(1);
}

#endif // 1

static void
kplotctx_ccfg_init(struct kplotctx* ctx, struct kplotccfg* cfg)
{

	switch (cfg->type) {
	case (KPLOTCTYPE_PALETTE):
		vkvg_set_source_rgba(ctx->cr,
			ctx->cfg.clrs[cfg->palette % ctx->cfg.clrsz].rgba[0],
			ctx->cfg.clrs[cfg->palette % ctx->cfg.clrsz].rgba[1],
			ctx->cfg.clrs[cfg->palette % ctx->cfg.clrsz].rgba[2],
			ctx->cfg.clrs[cfg->palette % ctx->cfg.clrsz].rgba[3]);
		break;
	case (KPLOTCTYPE_PATTERN):
		vkvg_set_source(ctx->cr, cfg->pattern);
		break;
	case (KPLOTCTYPE_RGBA):
		vkvg_set_source_rgba(ctx->cr, cfg->rgba[0],
			cfg->rgba[1], cfg->rgba[2], cfg->rgba[3]);
		break;
	default:
		abort();
	}
}

void
kplotctx_ticln_init(struct kplotctx* ctx, struct kplotticln* line)
{

	kplotctx_ccfg_init(ctx, &line->clr);
	vkvg_set_line_width(ctx->cr, line->sz);
	vkvg_set_dash(ctx->cr, line->dashes,
		line->dashesz, line->dashoff);
}

void
kplotctx_font_init(struct kplotctx* ctx, struct kplotfont* font)
{

	kplotctx_ccfg_init(ctx, &font->clr);
	vkvg_select_font_face
	(ctx->cr, font->family/*,
		font->slant,
		font->weight*/);
	vkvg_set_font_size(ctx->cr, font->sz);
}

void
kplotctx_point_init(struct kplotctx* ctx, struct kplotpoint* pnt)
{

	kplotctx_ccfg_init(ctx, &pnt->clr);
	vkvg_set_line_width(ctx->cr, pnt->sz);
	vkvg_set_dash(ctx->cr, pnt->dashes,
		pnt->dashesz, pnt->dashoff);
}

void
kplotctx_line_init(struct kplotctx* ctx, struct kplotline* line)
{

	kplotctx_ccfg_init(ctx, &line->clr);
	vkvg_set_line_width(ctx->cr, line->sz);
	vkvg_set_dash(ctx->cr, line->dashes,
		line->dashesz, line->dashoff);
	vkvg_set_line_join(ctx->cr, line->join);
}

/*
 * Given a plotting context and a position for drawing a line, determine
 * whether we want to "fix" the line so that it's fine.
 * This is a foible of Cairo and drawing with doubles.
 */
double
kplotctx_line_fix(const struct kplotctx* ctx, double sz, double pos)
{
	double	 v;

	if (0 == (int)sz % 2)
		return(pos);
	v = pos - floor(pos);
	return(v < DBL_EPSILON ? pos + 0.5 : pos - v + 0.5);
}


void
kplotctx_border_init(struct kplotctx* ctx)
{
	double		 v;

	kplotctx_line_init(ctx, &ctx->cfg.borderline);

	if (BORDER_LEFT & ctx->cfg.border) {
		v = kplotctx_line_fix(ctx,
			ctx->cfg.borderline.sz, ctx->offs.x);
		vkvg_move_to(ctx->cr, v, ctx->offs.y);
		vkvg_rel_line_to(ctx->cr, 0.0, ctx->dims.y);
	}

	if (BORDER_RIGHT & ctx->cfg.border) {
		v = kplotctx_line_fix(ctx,
			ctx->cfg.borderline.sz,
			ctx->offs.x + ctx->dims.x);
		vkvg_move_to(ctx->cr, v, ctx->offs.y);
		vkvg_rel_line_to(ctx->cr, 0.0, ctx->dims.y);
	}

	if (BORDER_TOP & ctx->cfg.border) {
		v = kplotctx_line_fix(ctx,
			ctx->cfg.borderline.sz, ctx->offs.y);
		vkvg_move_to(ctx->cr, ctx->offs.x, v);
		vkvg_rel_line_to(ctx->cr, ctx->dims.x, 0.0);
	}

	if (BORDER_BOTTOM & ctx->cfg.border) {
		v = kplotctx_line_fix(ctx,
			ctx->cfg.borderline.sz,
			ctx->offs.y + ctx->dims.y);
		vkvg_move_to(ctx->cr, ctx->offs.x, v);
		vkvg_rel_line_to(ctx->cr, ctx->dims.x, 0.0);
	}

	vkvg_stroke(ctx->cr);
}

void
kplotctx_grid_init(struct kplotctx* ctx)
{
	double		 offs, v;
	size_t		 i;

	kplotctx_line_init(ctx, &ctx->cfg.gridline);

	if (GRID_X & ctx->cfg.grid)
		for (i = 0; i < ctx->cfg.xtics; i++) {
			offs = 1 == ctx->cfg.xtics ? 0.5 :
				i / (double)(ctx->cfg.xtics - 1);
			v = kplotctx_line_fix(ctx,
				ctx->cfg.gridline.sz,
				ctx->offs.x + offs * ctx->dims.x);
			vkvg_move_to(ctx->cr, v, ctx->offs.y);
			vkvg_rel_line_to(ctx->cr, 0.0, ctx->dims.y);
		}

	if (GRID_Y & ctx->cfg.grid)
		for (i = 0; i < ctx->cfg.ytics; i++) {
			offs = 1 == ctx->cfg.ytics ? 0.5 :
				i / (double)(ctx->cfg.ytics - 1);
			v = kplotctx_line_fix(ctx,
				ctx->cfg.gridline.sz,
				ctx->offs.y + offs * ctx->dims.y);
			vkvg_move_to(ctx->cr, ctx->offs.x, v);
			vkvg_rel_line_to(ctx->cr, ctx->dims.x, 0.0);
		}

	vkvg_stroke(ctx->cr);
}

void
kplotctx_tic_init(struct kplotctx* ctx)
{
	double		 offs, v;
	size_t		 i;

	kplotctx_ticln_init(ctx, &ctx->cfg.ticline);

	for (i = 0; i < ctx->cfg.xtics; i++) {
		offs = 1 == ctx->cfg.xtics ? 0.5 :
			i / (double)(ctx->cfg.xtics - 1);
		v = kplotctx_line_fix(ctx,
			ctx->cfg.ticline.sz,
			ctx->offs.x + offs * ctx->dims.x);
		if (TIC_BOTTOM_IN & ctx->cfg.tic) {
			vkvg_move_to(ctx->cr, v,
				ctx->offs.y + ctx->dims.y);
			vkvg_rel_line_to(ctx->cr,
				0.0, -ctx->cfg.ticline.len);
		}
		if (TIC_BOTTOM_OUT & ctx->cfg.tic) {
			vkvg_move_to(ctx->cr, v,
				ctx->offs.y + ctx->dims.y);
			vkvg_rel_line_to(ctx->cr,
				0.0, ctx->cfg.ticline.len);
		}
		if (TIC_TOP_IN & ctx->cfg.tic) {
			vkvg_move_to(ctx->cr, v, ctx->offs.y);
			vkvg_rel_line_to(ctx->cr,
				0.0, ctx->cfg.ticline.len);
		}
		if (TIC_TOP_OUT & ctx->cfg.tic) {
			vkvg_move_to(ctx->cr, v, ctx->offs.y);
			vkvg_rel_line_to(ctx->cr,
				0.0, -ctx->cfg.ticline.len);
		}
	}

	for (i = 0; i < ctx->cfg.ytics; i++) {
		offs = 1 == ctx->cfg.ytics ? 0.5 :
			i / (double)(ctx->cfg.ytics - 1);
		v = kplotctx_line_fix(ctx,
			ctx->cfg.ticline.sz,
			ctx->offs.y + offs * ctx->dims.y);
		if (TIC_LEFT_IN & ctx->cfg.tic) {
			vkvg_move_to(ctx->cr, ctx->offs.x, v);
			vkvg_rel_line_to(ctx->cr,
				ctx->cfg.ticline.len, 0.0);
		}
		if (TIC_LEFT_OUT & ctx->cfg.tic) {
			vkvg_move_to(ctx->cr, ctx->offs.x, v);
			vkvg_rel_line_to(ctx->cr,
				-ctx->cfg.ticline.len, 0.0);
		}
		if (TIC_RIGHT_IN & ctx->cfg.tic) {
			vkvg_move_to(ctx->cr,
				ctx->offs.x + ctx->dims.x, v);
			vkvg_rel_line_to(ctx->cr,
				-ctx->cfg.ticline.len, 0.0);
		}
		if (TIC_RIGHT_OUT & ctx->cfg.tic) {
			vkvg_move_to(ctx->cr,
				ctx->offs.x + ctx->dims.x, v);
			vkvg_rel_line_to(ctx->cr,
				ctx->cfg.ticline.len, 0.0);
		}
	}

	vkvg_stroke(ctx->cr);
}

void
kplotctx_margin_init(struct kplotctx* ctx)
{

	ctx->dims.x = ctx->w;
	ctx->dims.y = ctx->h;

	if (MARGIN_LEFT & ctx->cfg.margin) {
		ctx->dims.x -= ctx->cfg.marginsz;
		ctx->offs.x = ctx->cfg.marginsz;
	}
	if (MARGIN_RIGHT & ctx->cfg.margin)
		ctx->dims.x -= ctx->cfg.marginsz;

	if (MARGIN_TOP & ctx->cfg.margin) {
		ctx->dims.y -= ctx->cfg.marginsz;
		ctx->offs.y = ctx->cfg.marginsz;
	}
	if (MARGIN_BOTTOM & ctx->cfg.margin)
		ctx->dims.y -= ctx->cfg.marginsz;
}

#if 1

static void
bbox_extents(struct kplotctx* ctx, const char* v,
	double* h, double* w, double rot)
{
	vkvg_text_extents_t e;

	vkvg_text_extents(ctx->cr, v, &e);
	*h = fabs(e.width * sin(rot)) + fabs(e.height * cos(rot));
	*w = fabs(e.width * cos(rot)) + fabs(e.height * sin(rot));
}

void
kplotctx_label_init(struct kplotctx* ctx)
{
	char		buf[128];
	size_t		i;
	vkvg_text_extents_t e;
	double		maxh, maxw, offs, lastx,
		lasty, firsty, w, h;

	maxh = maxw = lastx = lasty = firsty = 0.0;

	/*
	 * First, acquire the maximum space that will be required for
	 * the vertical (left or right) or horizontal (top or bottom)
	 * tic labels.
	 */
	kplotctx_font_init(ctx, &ctx->cfg.ticlabelfont);

	for (i = 0; i < ctx->cfg.xtics; i++) {
		offs = 1 == ctx->cfg.xtics ? 0.5 :
			i / (double)(ctx->cfg.xtics - 1);

		/* Call out to xformat function. */
		if (NULL == ctx->cfg.xticlabelfmt)
			snprintf(buf, sizeof(buf), "%g",
				ctx->minv.x + offs *
				(ctx->maxv.x - ctx->minv.x));
		else
			(*ctx->cfg.xticlabelfmt)
			(ctx->minv.x + offs *
				(ctx->maxv.x - ctx->minv.x),
				buf, sizeof(buf));

		vkvg_text_extents(ctx->cr, buf, &e);

		/*
		 * Important: if we're on the last x-axis value, then
		 * save the width, because we'll check that the
		 * right-hand buffer zone accomodates for it.
		 * FIXME: only do this is TICLABEL_TOP, etc...
		 */
		if (i == ctx->cfg.xtics - 1 && ctx->cfg.xticlabelrot > 0.0)
			lastx = e.width * cos
			(M_PI * 2.0 -
				(M_PI_2 - ctx->cfg.xticlabelrot)) +
			e.height * sin((ctx->cfg.xticlabelrot));
		else if (i == ctx->cfg.xtics - 1)
			lastx = e.width / 2.0;

		/*
		 * If we're rotating, get our height by computing the
		 * sum of the vertical segments.
		 */
		if (ctx->cfg.xticlabelrot > 0.0)
			e.height = e.width * sin(ctx->cfg.xticlabelrot) +
			e.height * cos(M_PI * 2.0 -
				(M_PI_2 - ctx->cfg.xticlabelrot));

		if (e.height > maxh)
			maxh = e.height;
	}

	/* Now for the y-axis... */
	for (i = 0; i < ctx->cfg.ytics; i++) {
		offs = 1 == ctx->cfg.ytics ? 0.5 :
			i / (double)(ctx->cfg.ytics - 1);

		if (NULL == ctx->cfg.yticlabelfmt)
			snprintf(buf, sizeof(buf), "%g",
				ctx->minv.y + offs *
				(ctx->maxv.y - ctx->minv.y));
		else
			(*ctx->cfg.yticlabelfmt)
			(ctx->minv.y + offs *
				(ctx->maxv.y - ctx->minv.y),
				buf, sizeof(buf));

		vkvg_text_extents(ctx->cr, buf, &e);

		/*
		 * If we're the first or last tic label, record our
		 * height so that the plot is buffered and our label
		 * isn't cut off if there are no margins.
		 */
		if (i == 0)
			firsty = e.height / 2.0;
		if (i == ctx->cfg.ytics - 1)
			lasty = e.height / 2.0;

		if (e.width > maxw)
			maxw = e.width;
	}

	/*
	 * Take into account the axis labels.
	 * These sit to the bottom and left of the plot and its tic
	 * labels.
	 */
	kplotctx_font_init(ctx, &ctx->cfg.axislabelfont);

	if (NULL != ctx->cfg.xaxislabel) {
		bbox_extents(ctx, ctx->cfg.xaxislabel,
			&h, &w, ctx->cfg.xaxislabelrot);
		ctx->dims.y -= h + ctx->cfg.xaxislabelpad;
	}

	if (NULL != ctx->cfg.x2axislabel) {
		bbox_extents(ctx, ctx->cfg.x2axislabel,
			&h, &w, ctx->cfg.xaxislabelrot);
		ctx->offs.y += h + ctx->cfg.xaxislabelpad;
		ctx->dims.y -= h + ctx->cfg.xaxislabelpad;
	}

	if (NULL != ctx->cfg.yaxislabel) {
		bbox_extents(ctx, ctx->cfg.yaxislabel,
			&h, &w, ctx->cfg.yaxislabelrot);
		ctx->offs.x += w + ctx->cfg.yaxislabelpad;
		ctx->dims.x -= w + ctx->cfg.yaxislabelpad;
	}

	if (NULL != ctx->cfg.y2axislabel) {
		bbox_extents(ctx, ctx->cfg.y2axislabel,
			&h, &w, ctx->cfg.yaxislabelrot);
		ctx->dims.x -= w + ctx->cfg.yaxislabelpad;
	}

	if (TICLABEL_LEFT & ctx->cfg.ticlabel) {
		ctx->offs.x += maxw + ctx->cfg.yticlabelpad;
		ctx->dims.x -= maxw + ctx->cfg.yticlabelpad;
	}

	/*
	 * Now look at the tic labels.
	 * Start with the right label.
	 * Also check if our overflow for the horizontal axes into the
	 * right buffer zone exists.
	 */
	if (TICLABEL_RIGHT & ctx->cfg.ticlabel) {
		if (maxw + ctx->cfg.yticlabelpad > lastx)
			ctx->dims.x -= maxw + ctx->cfg.yticlabelpad;
		else
			ctx->dims.x -= lastx;
	}
	else if (lastx > 0.0)
		ctx->dims.x -= lastx;

	/*
	 * Like with TICLABEL_RIGHT, we accomodate for the topmost vertical
	 * axes bleeding into the horizontal axis area above.
	 */
	if (TICLABEL_TOP & ctx->cfg.ticlabel) {
		if (maxh + ctx->cfg.xticlabelpad > lasty) {
			ctx->offs.y += maxh + ctx->cfg.xticlabelpad;
			ctx->dims.y -= maxh + ctx->cfg.xticlabelpad;
		}
		else {
			ctx->offs.y += lasty;
			ctx->dims.y -= lasty;
		}
	}
	else if (lasty > 0.0) {
		ctx->offs.y += lasty;
		ctx->dims.y -= lasty;
	}

	if (TICLABEL_BOTTOM & ctx->cfg.ticlabel) {
		if (maxh + ctx->cfg.xticlabelpad > firsty)
			ctx->dims.y -= maxh + ctx->cfg.xticlabelpad;
		else
			ctx->dims.y -= firsty;
	}
	else if (firsty > 0.0)
		ctx->dims.y -= firsty;

	/*
	 * Now we actually want to draw the tic labels below the plot,
	 * now that we know what the plot dimensions are going to be.
	 * Start with the x-axis.
	 */
	kplotctx_font_init(ctx, &ctx->cfg.ticlabelfont);

	for (i = 0; i < ctx->cfg.xtics; i++) {
		offs = 1 == ctx->cfg.xtics ? 0.5 :
			i / (double)(ctx->cfg.xtics - 1);

		if (NULL == ctx->cfg.xticlabelfmt)
			snprintf(buf, sizeof(buf), "%g",
				ctx->minv.x + offs *
				(ctx->maxv.x - ctx->minv.x));
		else
			(*ctx->cfg.xticlabelfmt)
			(ctx->minv.x + offs *
				(ctx->maxv.x - ctx->minv.x),
				buf, sizeof(buf));

		vkvg_text_extents(ctx->cr, buf, &e);

		if (TICLABEL_BOTTOM & ctx->cfg.ticlabel) {
			if (ctx->cfg.xticlabelrot > 0.0) {
				vkvg_move_to(ctx->cr,
					ctx->offs.x +
					offs * ctx->dims.x,
					ctx->offs.y + ctx->dims.y +
					e.height * cos
					(M_PI * 2.0 -
						(M_PI_2 - ctx->cfg.xticlabelrot)) +
					ctx->cfg.xticlabelpad);
				vkvg_save(ctx->cr);
				vkvg_rotate(ctx->cr, ctx->cfg.xticlabelrot);
			}
			else
				vkvg_move_to(ctx->cr,
					ctx->offs.x + offs * ctx->dims.x -
					(e.width / 2.0),
					ctx->offs.y + ctx->dims.y +
					maxh + ctx->cfg.xticlabelpad);

			vkvg_show_text(ctx->cr, buf);
			if (ctx->cfg.xticlabelrot > 0.0)
				vkvg_restore(ctx->cr);
		}

		if (TICLABEL_TOP & ctx->cfg.ticlabel) {
			vkvg_move_to(ctx->cr,
				ctx->offs.x + offs * ctx->dims.x -
				(e.width / 2.0),
				ctx->offs.y - maxh);
			vkvg_show_text(ctx->cr, buf);
		}
	}

	/* Now move on to the y-axis... */
	for (i = 0; i < ctx->cfg.ytics; i++) {
		offs = 1 == ctx->cfg.ytics ? 0.5 :
			i / (double)(ctx->cfg.ytics - 1);

		if (NULL == ctx->cfg.yticlabelfmt)
			snprintf(buf, sizeof(buf), "%g",
				ctx->minv.y + offs *
				(ctx->maxv.y - ctx->minv.y));
		else
			(*ctx->cfg.yticlabelfmt)
			(ctx->minv.y + offs *
				(ctx->maxv.y - ctx->minv.y),
				buf, sizeof(buf));

		vkvg_text_extents(ctx->cr, buf, &e);

		if (TICLABEL_LEFT & ctx->cfg.ticlabel) {
			vkvg_move_to(ctx->cr,
				ctx->offs.x - e.width -
				ctx->cfg.yticlabelpad,
				(ctx->offs.y + ctx->dims.y) -
				(offs * ctx->dims.y) +
				(e.height / 2.0));
			vkvg_show_text(ctx->cr, buf);
		}
		if (TICLABEL_RIGHT & ctx->cfg.ticlabel) {
			vkvg_move_to(ctx->cr,
				ctx->offs.x + ctx->dims.x +
				ctx->cfg.yticlabelpad,
				(ctx->offs.y + ctx->dims.y) -
				(offs * ctx->dims.y) +
				(e.height / 2.0));
			vkvg_show_text(ctx->cr, buf);
		}
	}

	/*
	 * Now show the axis labels.
	 * These go after everything else has been computed, as we can
	 * just set them given the margin offset.
	 */
	kplotctx_font_init(ctx, &ctx->cfg.axislabelfont);

	if (NULL != ctx->cfg.xaxislabel) {
		bbox_extents(ctx, ctx->cfg.xaxislabel,
			&h, &w, ctx->cfg.xaxislabelrot);
		vkvg_save(ctx->cr);
		vkvg_translate(ctx->cr,
			ctx->offs.x + ctx->dims.x / 2.0,
			(MARGIN_BOTTOM & ctx->cfg.margin ?
				ctx->h - ctx->cfg.marginsz : ctx->h) - h / 2.0);
		vkvg_rotate(ctx->cr, ctx->cfg.xaxislabelrot);
		vkvg_text_extents(ctx->cr, ctx->cfg.xaxislabel, &e);
		w = -e.width / 2.0;
		h = e.height / 2.0;
		vkvg_translate(ctx->cr, w, h);
		vkvg_move_to(ctx->cr, 0.0, 0.0);
		vkvg_show_text(ctx->cr, ctx->cfg.xaxislabel);
		vkvg_restore(ctx->cr);
	}

	if (NULL != ctx->cfg.x2axislabel) {
		bbox_extents(ctx, ctx->cfg.x2axislabel,
			&h, &w, ctx->cfg.xaxislabelrot);
		vkvg_save(ctx->cr);
		vkvg_translate(ctx->cr,
			ctx->offs.x + ctx->dims.x / 2.0,
			(MARGIN_TOP & ctx->cfg.margin ?
				ctx->cfg.marginsz : 0.0) + h / 2.0);
		vkvg_rotate(ctx->cr, ctx->cfg.xaxislabelrot);
		vkvg_text_extents(ctx->cr, ctx->cfg.x2axislabel, &e);
		w = -e.width / 2.0;
		h = e.height / 2.0;
		vkvg_translate(ctx->cr, w, h);
		vkvg_move_to(ctx->cr, 0.0, 0.0);
		vkvg_show_text(ctx->cr, ctx->cfg.x2axislabel);
		vkvg_restore(ctx->cr);
	}

	if (NULL != ctx->cfg.yaxislabel) {
		bbox_extents(ctx, ctx->cfg.yaxislabel,
			&h, &w, ctx->cfg.yaxislabelrot);
		vkvg_save(ctx->cr);
		vkvg_translate(ctx->cr,
			(MARGIN_LEFT & ctx->cfg.margin ?
				ctx->cfg.marginsz : 0.0) + w / 2.0,
			ctx->offs.y + ctx->dims.y / 2.0);
		vkvg_rotate(ctx->cr, ctx->cfg.yaxislabelrot);
		vkvg_text_extents(ctx->cr, ctx->cfg.yaxislabel, &e);
		w = -e.width / 2.0;
		h = e.height / 2.0;
		vkvg_translate(ctx->cr, w, h);
		vkvg_move_to(ctx->cr, 0.0, 0.0);
		vkvg_show_text(ctx->cr, ctx->cfg.yaxislabel);
		vkvg_restore(ctx->cr);
	}

	if (NULL != ctx->cfg.y2axislabel) {
		bbox_extents(ctx, ctx->cfg.y2axislabel,
			&h, &w, ctx->cfg.yaxislabelrot);
		vkvg_save(ctx->cr);
		vkvg_translate(ctx->cr,
			(MARGIN_RIGHT & ctx->cfg.margin ?
				ctx->w - ctx->cfg.marginsz : ctx->w) - w / 2.0,
			ctx->offs.y + ctx->dims.y / 2.0);
		vkvg_rotate(ctx->cr, ctx->cfg.yaxislabelrot);
		vkvg_text_extents(ctx->cr, ctx->cfg.y2axislabel, &e);
		w = -e.width / 2.0;
		h = e.height / 2.0;
		vkvg_translate(ctx->cr, w, h);
		vkvg_move_to(ctx->cr, 0.0, 0.0);
		vkvg_show_text(ctx->cr, ctx->cfg.y2axislabel);
		vkvg_restore(ctx->cr);
	}
}

#endif // 1
