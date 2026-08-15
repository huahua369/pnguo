
struct VSInput
{
	float2 inPos;
	float2 inUV;
	float4 inColor;
};

struct VSOutput
{
	float4 pos : SV_Position;
	float2 UV : TEXCOORD0;
	float4 Src : TEXCOORD1;
	nointerpolation int   PatType : TEXCOORD2;
	nointerpolation float Opacity : TEXCOORD3;
	float2x3  Mat : TEXCOORD4;
};

struct uboGrad_
{
	float4	colors[32];
	float	stops[32];
	float4  cp[2];
	int4    m;
	float2  scale;
	uint	count;
	int		extend;
};

[[vk::constant_id(0)]] const int NUM_SAMPLES = 8;

struct PushConsts
{
	float2x3 mat;
	float2x3 matInv;
	float4 source;
	float2 size;
	int    fullScreenQuad_srcType;
	float  opacity;
};

#ifdef PUSH_CONST
[[vk::push_constant]] PushConsts pc;
#else
[[vk::binding(0, 1)]] ConstantBuffer<PushConsts> pc;
#endif
[[vk::binding(0, 3)]] ConstantBuffer<uboGrad_> uboGrad;
[[vk::binding(0, 2)]] Sampler2D source;

static const int FULLSCREEN_BIT = 0x10000000;
static const int SRCTYPE_MASK = 0x000000FF;
static const int SOLID = 0;
static const int SURFACE = 1;
static const int LINEAR = 2;
static const int RADIAL = 3;
static const int MESH = 4;
static const int RASTER_SOURCE = 5;
static const int SWEEP = 6;

[shader("vertex")]
VSOutput main(VSInput input)
{
	VSOutput output;
	output.PatType = pc.fullScreenQuad_srcType & SRCTYPE_MASK;
	output.Mat = pc.matInv;
	output.Src = output.PatType == SOLID ? input.inColor : pc.source;
	output.Opacity = pc.opacity;

	if ((pc.fullScreenQuad_srcType & FULLSCREEN_BIT) == FULLSCREEN_BIT)
	{
		output.pos = float4(input.inPos, 0.0, 1.0);
		output.UV = float2(0, 0);
		return output;
	}

	output.UV = input.inUV;

	float2 p = mul(pc.mat, float3(input.inPos, 1.0));
	output.pos = float4(p * 2.0 / pc.size - 1.0, 0.0, 1.0);
	return output;
}


float4 gpu_stop_color(int i, out float offset)
{
	offset = uboGrad.stops[i];
	return uboGrad.colors[i];
}

float gpu_extend_t(float t, int extend)
{
	if (extend == 1) {           /* HB_PAINT_EXTEND_REPEAT */
		return t - floor(t);
	}
	else if (extend == 2) {    /* HB_PAINT_EXTEND_REFLECT */
		float u = t - 2.0 * floor(t * 0.5);
		return u > 1.0 ? 2.0 - u : u;
	}
	return clamp(t, 0.0, 1.0);  /* PAD (default) */
}

float4 gpu_eval_stops(int stop_count, float t)
{
	float off_prev;
	float4 col_prev = gpu_stop_color(0, off_prev);
	if (t <= off_prev)
		return col_prev;
	for (int i = 1; i < stop_count; i++)
	{
		float off;
		float4 col = gpu_stop_color(i, off);
		if (t <= off)
		{
			float span = off - off_prev;
			float f = span > 1e-6 ? (t - off_prev) / span : 0.0;
			float4 pm = lerp(col_prev, col, f);
			return pm.a > 1e-6 ? pm : float4(0.0);
		}
		col_prev = col;
		off_prev = off;
	}
	return col_prev;
}

float2 gpu_apply_minv(int4 m, float2 v)
{
	float4 mf = float4(m) * (1.0 / 1024.0);
	return float2(mf.x * v.x + mf.y * v.y,
		mf.z * v.x + mf.w * v.y);
}

/* Sample a linear gradient whose param blob starts at @grad_base:
 *   texel 0: (p0_rendered.x, p0_rendered.y, d_canonical.x, d_canonical.y)
 *   texel 1: L^-1 as i16 Q10 (row-major)
 *   texels 2..: stops (2 texels each)
 * Evaluate t in untransformed space. */
float4 gpu_sample_linear(float2 renderCoord, float2 box, int stop_count, int extend)
{
	float4 cp = uboGrad.cp[0];
	float2 rCoord = renderCoord / box;
	float4 t0 = cp;
	float2 p0 = float2(t0.xy / box);
	float2 d = float2(t0.zw / box) - p0;
	float denom = dot(d, d);
	if (denom < 1e-6) return float4(0.0);
	int4 m = uboGrad.m;
	float2 p1 = renderCoord - t0.xy;
	float2 p = p1 / box;
	p = gpu_apply_minv(m, p);
	float t = dot(p, d) / denom;
	t = gpu_extend_t(t, extend);
	return gpu_eval_stops(stop_count, t);
}


float4 gpu_sample_radial(float2 renderCoord, float2 box, int stop_count, int extend)
{
	float4 cp[2] = uboGrad.cp;
	float2 rCoord = renderCoord / box;
	int4 m = uboGrad.m;
	float2 c0_r = cp[0].xy / box;
	float2 cd = cp[1].xy / box;
	cd -= c0_r;
	float r0 = cp[0].z / box.x;
	float r1 = cp[1].z / box.x;
	float dr = r1 - r0;
	float2 p1 = (renderCoord - cp[0].xy);
	float2 p = (p1 / box);
	p = gpu_apply_minv(m, p);

	float A = dot(cd, cd) - dr * dr;
	float B = -2.0 * (dot(p, cd) + r0 * dr);
	float C = dot(p, p) - r0 * r0;

	float t;
	if (abs(A) > 1e-6)
	{
		float disc = B * B - 4.0 * A * C;
		if (disc < 0.0) return float4(0.0);
		float sq = sqrt(disc);
		/* Prefer the larger root; fall back to the smaller if the
		 * larger gives a negative interpolated radius. */
		float t1 = (-B + sq) / (2.0 * A);
		float t2 = (-B - sq) / (2.0 * A);
		t = (r0 + t1 * dr >= 0.0) ? t1 : t2;
	}
	else
	{
		if (abs(B) < 1e-6) return float4(0.0);
		t = -C / B;
	}

	t = gpu_extend_t(t, extend);
	return gpu_eval_stops(stop_count, t);
}

float4 gpu_sample_sweep(float2 renderCoord, float2 box, int stop_count, int extend)
{
	float4 cp[2] = uboGrad.cp;
	float2 rCoord = renderCoord / box;
	float4 t0 = cp[0];
	int4 m = uboGrad.m;// int4(1024, 0, 0, 1024);
	float2 p0 = float2(t0.xy / box);
	float a0 = t0.z;  /* fraction of pi */
	float a1 = t0.w;
	float span = a1 - a0;
	if (abs(span) < 1e-6) return float4(0.0);
	float2 p1 = normalize(renderCoord - t0.xy);
	float2 p = normalize(rCoord - p0);
	p = gpu_apply_minv(m, p);
	/* atan2 returns (-pi, pi]; normalize to [0, 2) fractions of pi. */
	float ang = atan2(p.y, p.x) / 3.14159265358979;
	if (ang < 0.0) ang += 2.0;  // 归一化到 [0, 2]
	float t = (ang - a0) / span;
	t = gpu_extend_t(t, extend);
	return gpu_eval_stops(stop_count, t);
}
float4 gpu_composite(float4 src, float4 dst, int mode)
{
	float4 r = src + dst * (1.0 - src.a);  /* SRC_OVER default */

	/* Approximate unsupported COLRv1 modes with the nearest Porter-Duff
	 * mode we do implement.  Better a recognizable rendering than a
	 * silent SRC_OVER fallback.  DIFFERENCE / EXCLUSION / HSL_* are
	 * not similar enough to anything we have, so they still fall
	 * through to SRC_OVER below. */
	if (mode == 14 || mode == 18 || mode == 19) mode = 23; /* OVERLAY / COLOR_BURN / HARD_LIGHT -> MULTIPLY */
	else if (mode == 17 || mode == 20)               mode = 13; /* COLOR_DODGE / SOFT_LIGHT -> SCREEN */

	if (mode == 0)  r = float4(0.0);                       /* CLEAR */
	else if (mode == 1)  r = src;                              /* SRC */
	else if (mode == 2)  r = dst;                              /* DST */
	else if (mode == 4)  r = dst + src * (1.0 - dst.a);        /* DST_OVER */
	else if (mode == 5)  r = src * dst.a;                      /* SRC_IN */
	else if (mode == 6)  r = dst * src.a;                      /* DST_IN */
	else if (mode == 7)  r = src * (1.0 - dst.a);              /* SRC_OUT */
	else if (mode == 8)  r = dst * (1.0 - src.a);              /* DST_OUT */
	else if (mode == 9)                                        /* SRC_ATOP */
		r = src * dst.a + dst * (1.0 - src.a);
	else if (mode == 10)                                       /* DST_ATOP */
		r = dst * src.a + src * (1.0 - dst.a);
	else if (mode == 11)                                       /* XOR */
		r = src * (1.0 - dst.a) + dst * (1.0 - src.a);
	else if (mode == 12)                                       /* PLUS */
		r = min(src + dst, float4(1.0));
	else if (mode == 13) {                                     /* SCREEN (premul) */
		r.rgb = src.rgb + dst.rgb - src.rgb * dst.rgb;
		r.a = src.a + dst.a - src.a * dst.a;
	}
	else if (mode == 15) {                                     /* DARKEN */
		r.rgb = min(src.rgb * dst.a, dst.rgb * src.a)
			+ src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a);
		r.a = src.a + dst.a - src.a * dst.a;
	}
	else if (mode == 16) {                                     /* LIGHTEN */
		r.rgb = max(src.rgb * dst.a, dst.rgb * src.a)
			+ src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a);
		r.a = src.a + dst.a - src.a * dst.a;
	}
	else if (mode == 23) {                                     /* MULTIPLY (premul) */
		r.rgb = src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a)
			+ src.rgb * dst.rgb;
		r.a = src.a + dst.a - src.a * dst.a;
	}
	/* SRC_OVER (3) and DIFFERENCE / EXCLUSION / HSL_* (21, 22, 24-27)
	 * fall through to the SRC_OVER default. */

	return r;
}
float4 gpu_sample_mesh(float2 renderCoord, float2 box, int stop_count, int extend)
{
	float4 t0 = uboGrad.cp[0];
	int4 m = uboGrad.m;// int4(1024, 0, 0, 1024);
	float2 p0 = float2(t0.xy / box);
	float a0 = t0.z;  /* fraction of pi */
	float a1 = t0.w;
	float span = a1 - a0;
	if (abs(span) < 1e-6) return float4(0.0);
	float2 p = gpu_apply_minv(m, renderCoord - p0);
	float t = renderCoord.x * float(stop_count - 1);
	int index = int(floor(t));
	float fraction = t - float(index);

	index = clamp(index, 0, stop_count - 2);
	t = clamp(fraction, 0.0, 1.0);

	t = gpu_extend_t(t, extend);
	return gpu_eval_stops(stop_count, t);
}
float4 gpu_paint(float2 renderCoord, float4 inSrc, float2x3 inMat, int inPatType)
{
	float2 box = inSrc.xy; 	// 画布大小,一般用最大值
	box *= uboGrad.scale;
	float4 acc = float4(0.0);
	int extend = uboGrad.extend;
	int stop_count = int(uboGrad.count);
	float4 col = inSrc;
	switch (inPatType) {
	case SURFACE:
		float2 p = (renderCoord.xy - inSrc.xy);
		float2 uv = mul(inMat, float3(p, 1.0));
		uv /= inSrc.zw;
		/*if (uv.x < 0 || uv.y < 0 || uv.x > 1 || uv.y > 1)
			discard;*/
		col = source.Sample(uv);
		break;
	case LINEAR:
		col = gpu_sample_linear(renderCoord, box, stop_count, extend);
		break;
	case RADIAL:
		col = gpu_sample_radial(renderCoord, box, stop_count, extend);
		break;
	case SWEEP:
		col = gpu_sample_sweep(renderCoord, box, stop_count, extend);
		break;
	};
	float4 src = float4(col.rgb * col.a, col.a);
	acc = src + acc * (1.0 - src.a);
	return src;
}
[shader("fragment")]
float4 fragMain(VSOutput input)
{
	float4 c = gpu_paint(input.pos.xy, input.Src, input.Mat, input.PatType);
	c *= input.Opacity;
	return c;
}
