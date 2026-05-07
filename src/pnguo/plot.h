#pragma once
#include <stdint.h>
#include <stdbool.h>

#define KPLOT_H

struct 	kpair {
	double	 x;
	double	 y;
};

enum	kplottype {
	KPLOT_POINTS,
	KPLOT_MARKS,
	KPLOT_LINES,
	KPLOT_LINESPOINTS,
	KPLOT_LINESMARKS
};

enum	ksmthtype {
	KSMOOTH_NONE,
	KSMOOTH_MOVAVG,
	KSMOOTH_CDF,
	KSMOOTH_PMF
};

enum	kplotstype {
	KPLOTS_SINGLE,
	KPLOTS_YERRORLINE,
	KPLOTS_YERRORBAR
};

enum	kplotctype {
	KPLOTCTYPE_DEFAULT = 0,
	KPLOTCTYPE_PALETTE,
	KPLOTCTYPE_PATTERN,
	KPLOTCTYPE_RGBA
};


struct	kplotccfg {
	enum kplotctype	 type;
	size_t		 palette;
	VkvgPattern pattern;
	double		 rgba[4];
};

struct 	kplotfont {
	int   slant;
	int  weight;
	const char* family;
	double		     sz;
	struct kplotccfg     clr;
};

struct	kplotticln {
	double		  sz;
	double		  len;
#define	KPLOT_DASH_MAX	  8
	float	  	  dashes[KPLOT_DASH_MAX];
	size_t		  dashesz;
	double	 	  dashoff;
	struct kplotccfg  clr;
};

struct	kplotpoint {
	double		  sz;
	double		  radius;
	float	  	  dashes[KPLOT_DASH_MAX];
	size_t		  dashesz;
	double	 	  dashoff;
	struct kplotccfg  clr;
};

struct	kplotline {
	double		  sz;
	float	  	  dashes[KPLOT_DASH_MAX];
	size_t		  dashesz;
	double	 	  dashoff;
	vkvg_line_join_t join;
	struct kplotccfg  clr;
};

struct	ksmthcfg {
	size_t		  movsamples;
};

struct	kdatacfg {
	struct kplotline  line;
	struct kplotpoint point;
};

struct	kplotcfg {
	struct kplotccfg* clrs;
	size_t		  clrsz;
	double		  marginsz;
#define	MARGIN_LEFT	  0x01
#define	MARGIN_RIGHT	  0x02
#define	MARGIN_TOP	  0x04
#define	MARGIN_BOTTOM	  0x08
#define	MARGIN_ALL	  0xf
	unsigned int	  margin;
	struct kplotline  borderline;
	double		  bordersz;
#define	BORDER_LEFT	  0x01
#define	BORDER_RIGHT	  0x02
#define	BORDER_TOP	  0x04
#define	BORDER_BOTTOM	  0x08
#define	BORDER_ALL	  0xf
	unsigned int	  border;
	size_t		  xtics;
	size_t		  ytics;
	struct kplotticln ticline;
#define	TIC_LEFT_IN	  0x01
#define	TIC_LEFT_OUT	  0x02
#define	TIC_RIGHT_IN	  0x04
#define	TIC_RIGHT_OUT	  0x08
#define	TIC_TOP_IN	  0x10
#define	TIC_TOP_OUT	  0x20
#define	TIC_BOTTOM_IN	  0x40
#define	TIC_BOTTOM_OUT	  0x80
	unsigned int	  tic;
	double		  xticlabelrot;
	void		(*xticlabelfmt)(double, char*, size_t);
	void		(*yticlabelfmt)(double, char*, size_t);
	double		  yticlabelpad;
	double		  xticlabelpad;
	struct kplotfont  ticlabelfont;
#define	TICLABEL_LEFT	  0x01
#define	TICLABEL_RIGHT	  0x02
#define	TICLABEL_TOP	  0x04
#define	TICLABEL_BOTTOM	  0x08
	unsigned int	  ticlabel;
#define	GRID_X 		  0x01
#define GRID_Y 		  0x02
#define GRID_ALL 	  0x03
	unsigned int 	  grid;
	struct kplotline  gridline;
	double		  xaxislabelpad;
	double		  yaxislabelpad;
	const char* xaxislabel;
	const char* x2axislabel;
	const char* yaxislabel;
	const char* y2axislabel;
	struct kplotfont  axislabelfont;
	double		  xaxislabelrot;
	double		  yaxislabelrot;
#define	EXTREMA_XMIN	  0x01
#define	EXTREMA_XMAX	  0x02
#define	EXTREMA_YMIN	  0x04
#define	EXTREMA_YMAX	  0x08
	unsigned int	  extrema;
	double		  extrema_xmin;
	double		  extrema_xmax;
	double		  extrema_ymin;
	double		  extrema_ymax;
};

struct	kplotctx {
	VkvgContext cr;
	double		  h;
	double		  w;
	struct kpair	  minv;
	struct kpair	  maxv;
	struct kplotcfg	  cfg;
	struct kpair	  offs;
	struct kpair	  dims;
};

struct 	kdata;
struct	kplot;



void		 kdata_destroy(struct kdata*);
int		 kdata_get(const struct kdata*, size_t, struct kpair*);

int		 kdata_array_add(struct kdata*, size_t, double);
struct kdata* kdata_array_alloc(const struct kpair*, size_t);
int		 kdata_array_fill(struct kdata*, void*,
	void (*)(size_t, struct kpair*, void*));
int		 kdata_array_fill_ydoubles(struct kdata*, const double*);
int		 kdata_array_fill_ysizes(struct kdata*, const size_t*);
int		 kdata_array_set(struct kdata*, size_t, double, double);

int		 kdata_bucket_add(struct kdata*, size_t, double);
struct kdata* kdata_bucket_alloc(size_t, size_t);
int		 kdata_bucket_set(struct kdata*, size_t, double, double);

struct kdata* kdata_buffer_alloc(size_t);
int		 kdata_buffer_copy(struct kdata*, const struct kdata*);

int		 kdata_hist_add(struct kdata*, double, double);
struct kdata* kdata_hist_alloc(double, double, size_t);
int		 kdata_hist_set(struct kdata*, double, double);

struct kdata* kdata_mean_alloc(struct kdata*);
int		 kdata_mean_attach(struct kdata*, struct kdata*);

struct kdata* kdata_stddev_alloc(struct kdata*);
int		 kdata_stddev_attach(struct kdata*, struct kdata*);

struct kdata* kdata_vector_alloc(size_t);
int		 kdata_vector_append(struct kdata*, double, double);
int		 kdata_vector_set(struct kdata*, size_t, double, double);

double		 kdata_pmfmean(const struct kdata*);
double		 kdata_pmfvar(const struct kdata*);
double		 kdata_pmfstddev(const struct kdata*);

size_t		 kdata_xmax(const struct kdata*, struct kpair*);
double		 kdata_xmean(const struct kdata*);
size_t		 kdata_xmin(const struct kdata*, struct kpair*);

double		 kdata_xstddev(const struct kdata*);
size_t		 kdata_ymax(const struct kdata*, struct kpair*);
double		 kdata_ymean(const struct kdata*);
double		 kdata_ystddev(const struct kdata*);
size_t		 kdata_ymin(const struct kdata*, struct kpair*);

void		 kdatacfg_defaults(struct kdatacfg*);
void		 kplotcfg_defaults(struct kplotcfg*);
int		 kplotcfg_default_palette(struct kplotccfg**, size_t*);
void		 ksmthcfg_defaults(struct ksmthcfg*);
void		 kplotfont_defaults(struct kplotfont*);

struct kplot* kplot_alloc(const struct kplotcfg*);
int		 kplot_detach(struct kplot*, const struct kdata*);
int		 kplot_attach_data(struct kplot*, struct kdata*,
	enum kplottype, const struct kdatacfg*);
int		 kplot_attach_smooth(struct kplot*, struct kdata*,
	enum kplottype, const struct kdatacfg*,
	enum ksmthtype, const struct ksmthcfg*);
int		 kplot_attach_datas(struct kplot*, size_t,
	struct kdata**, const enum kplottype*,
	const struct kdatacfg* const*,
	enum kplotstype);
void		 kplotctx_draw(struct kplotctx*, struct kplot*,
	double, double, VkvgContext);
int		 kplotctx_translate(const struct kplotctx*, double,
	double, double*, double*);
void		 kplot_draw(struct kplot*, double, double, VkvgContext);
void		 kplot_free(struct kplot*);
int		 kplot_get_datacfg(struct kplot*, size_t,
	struct kdatacfg**, size_t*);
struct kplotcfg* kplot_get_plotcfg(struct kplot*);


struct	kdatahist {
	double		 rmin; /* minimum inclusive */
	double		 rmax; /* maximum non-inclusive */
};

struct	kdatabucket {
	size_t		 rmin; /* minimum inclusive */
	size_t		 rmax; /* maximum non-inclusive */
};

struct	kdatamean {
	size_t* ns; /* number of bucket modifications */
};

struct	kdatastddev {
	size_t* ns; /* number of bucket modifications */
	double* m1s; /* incremental mean */
	double* m2s; /* incremental variance parameter */
};

struct	kdatavector {
	size_t		 stepsz; /* vector increase slush size */
	size_t		 pairbufsz; /* allocated buffer size */
};

enum	kdatatype {
	KDATA_ARRAY,
	KDATA_BUCKET,
	KDATA_BUFFER,
	KDATA_HIST,
	KDATA_MEAN,
	KDATA_STDDEV,
	KDATA_VECTOR
};

typedef	int (*ksetfunc)(struct kdata*, size_t, double, double);

/*
 * A dependant tacks on to a data source and is notified (by way of
 * "func") whenever the value of a bucket has changed.
 */
struct	kdep {
	struct kdata* dep;
	ksetfunc	  func;
};

/*
 * A data source can either be "real" (in the sense of being modified by
 * the calling code) or a "dependant" (in the sense of being updated
 * from another data source).
 */
struct	kdata {
	struct kpair* pairs; /* data pairs */
	size_t		 pairsz; /* number of pairs */
	size_t		 refs; /* >0 references to data */
	struct kdep* deps; /* dependants */
	size_t		 depsz; /* number of dependants */
	enum kdatatype	 type;
	union {
		struct kdatahist	hist;
		struct kdatavector	vector;
		struct kdatabucket	bucket;
		struct kdatamean	mean;
		struct kdatastddev	stddev;
	} d;
};

struct	kplotdat {
	struct kdata** datas; /* referenced data */
	size_t		  datasz; /* number of data sets */
	struct kdatacfg* cfgs; /* plot configurations */
	enum kplottype* types; /* plot types */
	enum kplotstype	  stype; /* multiplot type */
	enum ksmthtype	  smthtype; /* smoothing type */
	struct ksmthcfg	  smth; /* smooth configuration */
	double		  sum; /* used for KSMOOTH_CDF */
};

struct	kplot {
	struct kplotdat* datas; /* data sets per plot */
	size_t		 datasz; /* number of data sets */
	struct kplotcfg	 cfg; /* configuration */
};

int	 kdata_dep_add(struct kdata*, struct kdata*, ksetfunc);
int	 kdata_dep_run(struct kdata*, size_t);
int	 kdata_set(struct kdata*, size_t, double, double);

void	 kplotctx_border_init(struct kplotctx*);
void	 kplotctx_grid_init(struct kplotctx*);
void	 kplotctx_margin_init(struct kplotctx*);
void	 kplotctx_tic_init(struct kplotctx*);
void	 kplotctx_label_init(struct kplotctx*);

double	 kplotctx_line_fix(const struct kplotctx*, double, double);

void	 kplotctx_font_init(struct kplotctx*, struct kplotfont*);
void	 kplotctx_line_init(struct kplotctx*, struct kplotline*);
void	 kplotctx_point_init(struct kplotctx*, struct kplotpoint*);
void	 kplotctx_ticln_init(struct kplotctx*, struct kplotticln*);

void	 kplotccfg_init_palette(struct kplotccfg*, size_t);
