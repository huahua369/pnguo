/*
 * Copyright (c) 2018-2022 Jean-Philippe Bruyère <jp_bruyere@hotmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_scalar_block_layout : enable

layout(set = 0, binding = 0) uniform sampler2DArray fontMap;
layout(set = 1, binding = 0) uniform sampler2D		source;
layout(scalar, set = 2, binding = 0) uniform _uboGrad {
	vec4	colors[32];
	float	stops[32];
	vec4    cp[2];
	ivec4   m;
	vec2    scale;
	uint	count;
	int		extend;
}uboGrad;
#define COLORSTOP(i) uboGrad.stops[i] 

layout(location = 0) in vec3		inFontUV;	//if it is a text drawing, inFontUV.z hold fontMap layer
layout(location = 1) in vec4		inSrc;		//source bounds or color depending on pattern type
layout(location = 2) in flat int	inPatType;	//pattern type
layout(location = 3) in flat float	inOpacity;
layout(location = 4) in mat3x2		inMat;

layout(location = 0) out vec4 outFragColor;

layout(constant_id = 0) const int NUM_SAMPLES = 8;


#define SOLID			0
#define SURFACE			1
#define LINEAR			2
#define RADIAL			3
#define MESH			4
#define RASTER_SOURCE	5
#define SWEEP			6

vec4 gpu_stop_color(int i, out float offset)
{
	offset = COLORSTOP(i);
	return uboGrad.colors[i];
}

/* Apply the color-line extend mode to a projected `t` value. */
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

/* Walk stops starting at @stops_base and return the sampled color
 * at @t.  Same logic reused by all gradient subtypes. */
vec4 gpu_eval_stops(int stop_count, float t)
{
	float off_prev;
	vec4 col_prev = gpu_stop_color(0, off_prev);
	if (t <= off_prev)
		return col_prev;
	for (int i = 1; i < stop_count; i++)
	{
		float off;
		vec4 col = gpu_stop_color(i, off);
		if (t <= off)
		{
			float span = off - off_prev;
			float f = span > 1e-6 ? (t - off_prev) / span : 0.0;
			/* Interpolate in premultiplied space per OpenType COLR spec. */
			//vec4 p0 = vec4(col_prev.rgb * col_prev.a, col_prev.a);
			//vec4 p1 = vec4(col.rgb * col.a, col.a);
			vec4 pm = mix(col_prev, col, f);
			//return pm.a > 1e-6 ? vec4(pm.rgb / pm.a, pm.a) : vec4(0.0);
			return pm.a > 1e-6 ? pm : vec4(0.0);
		}
		col_prev = col;
		off_prev = off;
	}
	return col_prev;
}

/* Apply the stored 2x2 M^-1 (row-major i16 Q10) to @v.  Scaling
 * renderCoord deltas back into canonical gradient space. */
vec2 gpu_apply_minv(ivec4 m, vec2 v)
{
	vec4 mf = vec4(m) * (1.0 / 1024.0);
	return vec2(mf.x * v.x + mf.y * v.y,
		mf.z * v.x + mf.w * v.y);
}

/* Sample a linear gradient whose param blob starts at @grad_base:
 *   texel 0: (p0_rendered.x, p0_rendered.y, d_canonical.x, d_canonical.y)
 *   texel 1: L^-1 as i16 Q10 (row-major)
 *   texels 2..: stops (2 texels each)
 * Evaluate t in untransformed space. */
vec4 gpu_sample_linear(vec2 renderCoord, vec2 box, int stop_count, int extend)
{
	vec4 t0 = uboGrad.cp[0];
	//ivec4 m = ivec4(1024, 0, 0, 1024);
	vec2 p0_r = vec2(t0.xy / box);
	vec2 d = vec2(t0.zw / box);
	float denom = dot(d, d);
	if (denom < 1e-6) return vec4(0.0);
	ivec4 m = uboGrad.m;
	vec2 p = gpu_apply_minv(m, renderCoord - p0_r);
	//vec2 p = renderCoord - p0_r;
	float t = dot(p, d) / denom;
	t = gpu_extend_t(t, extend);
	return gpu_eval_stops(stop_count, t);
}
/* Sample a two-circle radial gradient whose param blob starts at
 * @grad_base:
 *   texel 0: (c0_rendered.x, c0_rendered.y, d_canonical.x, d_canonical.y)
 *     d = c1 - c0 in untransformed space
 *   texel 1: (r0, r1, _, _) in untransformed font units
 *   texel 2: L^-1 as i16 Q10 (row-major)
 *   texels 3..: stops (2 texels each)
 * Solves |p - t*cd|^2 = (r0 + t*(r1-r0))^2 with p in untransformed
 * space, so non-uniform scale / shear on the transform becomes a
 * proper ellipse-in-rendered-space instead of a scalar-fudge. */
vec4 gpu_sample_radial_hb(vec2 renderCoord, vec2 box, int stop_count, int extend)
{
	ivec4 m = uboGrad.m;
	vec2 c0_r = uboGrad.cp[0].xy / box;
	vec2 cd = uboGrad.cp[1].xy / box;
	float r0 = uboGrad.cp[0].z / box.x;
	float r1 = uboGrad.cp[1].z / box.y;
	//renderCoord *= box;
	float dr = r1 - r0;
	vec2 p = gpu_apply_minv(m, normalize(renderCoord - c0_r));
	//vec2 p = normalize(renderCoord - c0_r);

	float A = dot(cd, cd) - dr * dr;
	float B = -2.0 * (dot(p, cd) + r0 * dr);
	float C = dot(p, p) - r0 * r0;

	float t;
	if (abs(A) > 1e-6)
	{
		float disc = B * B - 4.0 * A * C;
		if (disc < 0.0) return vec4(0.0);
		float sq = sqrt(disc);
		/* Prefer the larger root; fall back to the smaller if the
		 * larger gives a negative interpolated radius. */
		float t1 = (-B + sq) / (2.0 * A);
		float t2 = (-B - sq) / (2.0 * A);
		t = (r0 + t1 * dr >= 0.0) ? t1 : t2;
	}
	else
	{
		if (abs(B) < 1e-6) return vec4(0.0);
		t = -C / B;
	}

	t = gpu_extend_t(t, extend);
	return gpu_eval_stops(stop_count, t);
}
vec4 gpu_sample_radial(vec2 renderCoord, vec2 box, int stop_count, int extend)
{
	vec2 c0 = uboGrad.cp[0].xy / box;
	vec2 c1 = uboGrad.cp[1].xy / box;
	float r0 = uboGrad.cp[0].z / box.x;
	float r1 = uboGrad.cp[1].z / box.x;
	float gradLength = 1.0;
	vec2 diff = c0 - c1;
	vec2 rayDir = normalize(renderCoord - c0);
	ivec4 m = uboGrad.m;
	rayDir = gpu_apply_minv(m, rayDir);
	float a = dot(rayDir, rayDir);
	float b = 2.0 * dot(rayDir, diff);
	float cc = dot(diff, diff) - r1 * r1;
	float disc = b * b - 4.0 * a * cc;
	if (disc >= 0.0)
	{
		float t = (-b + sqrt(abs(disc))) / (2.0 * a);
		vec2 projection = c0 + rayDir * t;
		gradLength = distance(projection, c0) - r0;
	}
	else
	{
		//return vec4(0.0);
		//gradient is undefined for this coordinate
	}
	float grad = (distance(renderCoord, c0) - r0) / gradLength;
	float t = grad;
	t = gpu_extend_t(t, extend);
	return gpu_eval_stops(stop_count, t);
}

/* Sample a sweep gradient whose param blob starts at @grad_base:
 *   texel 0: (center_rendered.x, center_rendered.y, start_q14, end_q14)
 *            start/end are Q14 fractions of pi in untransformed space
 *   texel 1: L^-1 as i16 Q10 (row-major)
 *   texels 2..: stops (2 texels each) */
vec4 gpu_sample_sweep(vec2 renderCoord, vec2 box, int stop_count, int extend)
{
	vec4 t0 = uboGrad.cp[0];
	ivec4 m = uboGrad.m;// ivec4(1024, 0, 0, 1024);
	vec2 p0 = vec2(t0.xy / box);
	float a0 = t0.z;  /* fraction of pi */
	float a1 = t0.w;
	float span = a1 - a0;
	if (abs(span) < 1e-6) return vec4(0.0);
	vec2 p = gpu_apply_minv(m, renderCoord - p0);
	//vec2 p = (renderCoord - p0);
	/* atan2 returns (-pi, pi]; normalize to [0, 2) fractions of pi. */
	float ang = atan(p.y, p.x) / 3.14159265358979;
	if (ang < 0.0) ang += 2.0;  // 归一化到 [0, 2]
	float t = (ang - a0) / span;
	t = gpu_extend_t(t, extend);
	return gpu_eval_stops(stop_count, t);
}

/* Composite two premultiplied RGBA layers using one of the COLRv1
 * compositing modes.  Unsupported modes fall back to SRC_OVER.
 * Values match hb_paint_composite_mode_t. */
vec4 gpu_composite(vec4 src, vec4 dst, int mode)
{
	vec4 r = src + dst * (1.0 - src.a);  /* SRC_OVER default */

	/* Approximate unsupported COLRv1 modes with the nearest Porter-Duff
	 * mode we do implement.  Better a recognizable rendering than a
	 * silent SRC_OVER fallback.  DIFFERENCE / EXCLUSION / HSL_* are
	 * not similar enough to anything we have, so they still fall
	 * through to SRC_OVER below. */
	if (mode == 14 || mode == 18 || mode == 19) mode = 23; /* OVERLAY / COLOR_BURN / HARD_LIGHT -> MULTIPLY */
	else if (mode == 17 || mode == 20)               mode = 13; /* COLOR_DODGE / SOFT_LIGHT -> SCREEN */

	if (mode == 0)  r = vec4(0.0);                       /* CLEAR */
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
		r = min(src + dst, vec4(1.0));
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
// todo
vec4 gpu_sample_mesh(vec2 renderCoord, vec2 box, int stop_count, int extend)
{
	vec4 t0 = uboGrad.cp[0];
	ivec4 m = uboGrad.m;// ivec4(1024, 0, 0, 1024);
	vec2 p0 = vec2(t0.xy / box);
	float a0 = t0.z;  /* fraction of pi */
	float a1 = t0.w;
	float span = a1 - a0;
	if (abs(span) < 1e-6) return vec4(0.0);
	vec2 p = gpu_apply_minv(m, renderCoord - p0);
	float t = renderCoord.x * float(stop_count - 1);
	int index = int(floor(t));
	float fraction = t - float(index);

	index = clamp(index, 0, stop_count - 2);
	t = clamp(fraction, 0.0, 1.0);

	t = gpu_extend_t(t, extend);
	return gpu_eval_stops(stop_count, t);
}
/* Walks the paint blob's flat op stream and returns a
 * premultiplied RGBA coverage value for the current fragment.
 *
 * glyphLoc: atlas texel offset of the paint-blob header.
 * foreground: caller-supplied foreground color, used when an op
 *             sets the is_foreground flag.
 */
vec4 gpu_paint(vec2 renderCoord)
{
	float bmax = max(inSrc.x, inSrc.y);	// 画布大小
	vec2 box = vec2(bmax, bmax);		// 最大边界
	box *= uboGrad.scale;
	renderCoord = renderCoord / box;
	vec4 acc = vec4(0.0);
	int extend = uboGrad.extend;
	int stop_count = int(uboGrad.count);
	vec4 col = inSrc;
	switch (inPatType) {
	case SURFACE:
		vec2 p = (gl_FragCoord.xy - inSrc.xy);
		vec2 uv = vec2(
			inMat[0][0] * p.x + inMat[1][0] * p.y + inMat[2][0],
			inMat[0][1] * p.x + inMat[1][1] * p.y + inMat[2][1]
		);
		uv /= inSrc.zw;
		/*if (uv.x < 0 || uv.y < 0 || uv.x > 1 || uv.y > 1)
			discard;*/
		col = texture(source, uv);
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
	vec4 src = vec4(col.rgb * col.a, col.a);
	acc = src + acc * (1.0 - src.a);
	return src;
}
vec4 old_gpu_paint()
{
	vec4 c = inSrc;
	switch (inPatType) {
	case SURFACE:
		vec2 p = (gl_FragCoord.xy - inSrc.xy);
		vec2 uv = vec2(
			inMat[0][0] * p.x + inMat[1][0] * p.y + inMat[2][0],
			inMat[0][1] * p.x + inMat[1][1] * p.y + inMat[2][1]
		);
		uv /= inSrc.zw;
		/*if (uv.x < 0 || uv.y < 0 || uv.x > 1 || uv.y > 1)
			discard;*/
		c = texture(source, uv);
		break;
	case LINEAR:
		float dist = 1;
		vec2 p0 = uboGrad.cp[0].xy / inSrc.xy;
		vec2 p1 = uboGrad.cp[0].zw / inSrc.xy;
		p = gl_FragCoord.xy / inSrc.xy;

		float l = length(p1 - p0);
		vec2 u = normalize(p1 - p0);

		if (u.y == 0)
			if (u.x < 0)
				dist = -(p.x - p0.x) / l;
			else
				dist = (p.x - p0.x) / l;
		else {
			float m = -u.x / u.y;
			float bb = p0.y - m * p0.x;
			dist = ((p.y - m * p.x - bb) / sqrt(1 + m * m)) / l;
			if (u.y < 0)
				dist = -dist;
		}

		c = mix(uboGrad.colors[0], uboGrad.colors[1], mix(COLORSTOP(0), COLORSTOP(1), dist));//smoothstep
		for (int i = 1; i < uboGrad.count - 1; ++i)
			c = mix(c, uboGrad.colors[i + 1], mix(COLORSTOP(i), COLORSTOP(i + 1), dist));
		break;
	case RADIAL:
		p = gl_FragCoord.xy / inSrc.xy;
		vec2 c0 = uboGrad.cp[0].xy / inSrc.xy;
		vec2 c1 = uboGrad.cp[1].xy / inSrc.xy;
		float r0 = uboGrad.cp[0].z / inSrc.x;
		float r1 = uboGrad.cp[1].z / inSrc.x;
		/// APPLY FOCUS MODIFIER
		//project a point on the circle such that it passes through the focus and through the coord,
		//and then get the distance of the focus from that point.
		//that is the effective gradient length
		float gradLength = 1.0;
		vec2 diff = c0 - c1;
		vec2 rayDir = normalize(p - c0);
		float a = dot(rayDir, rayDir);
		float b = 2.0 * dot(rayDir, diff);
		float cc = dot(diff, diff) - r1 * r1;
		float disc = b * b - 4.0 * a * cc;
		if (disc >= 0.0)
		{
			float t = (-b + sqrt(abs(disc))) / (2.0 * a);
			vec2 projection = c0 + rayDir * t;
			gradLength = distance(projection, c0) - r0;
		}
		else
		{
			//gradient is undefined for this coordinate
		}
		/// OUTPUT
		float grad = (distance(p, c0) - r0) / gradLength;
		c = mix(uboGrad.colors[0], uboGrad.colors[1], mix(COLORSTOP(0), COLORSTOP(1), grad));
		for (int i = 2; i < uboGrad.count; i++)
			c = mix(c, uboGrad.colors[i], mix(COLORSTOP(i - 1), COLORSTOP(i), grad));
		break;
	case SWEEP:
		uv = (gl_FragCoord.xy / inSrc.xy);
		p0 = uboGrad.cp[0].xy / inSrc.xy;
		vec2 dir = uv - p0;	// 中心
		vec2 range = uv - uboGrad.cp[0].zw; // 开始到结束到弧度，暂用
		float angle = atan(dir.y, dir.x); // 计算角度
		angle = (angle + 3.14159) / (2.0 * 3.14159); // 归一化到 [0, 1] 
		// 插值颜色
		vec4 color = uboGrad.colors[0];
		for (uint i = 1; i < uboGrad.count; i++) {
			if (angle < uboGrad.stops[i]) {
				float t = (angle - uboGrad.stops[i - 1]) / (uboGrad.stops[i] - uboGrad.stops[i - 1]);
				color = mix(uboGrad.colors[i - 1], uboGrad.colors[i], t);
				break;
			}
		}
		c = color;
		break;
	}
	return c;
}
void main()
{
	vec4 c = gpu_paint(gl_FragCoord.xy);
	if (inFontUV.z >= 0.0)
		c *= texture(fontMap, inFontUV).r;

#ifdef VKVG_PREMULT_ALPHA
	c *= inOpacity;
#else
	c.a *= inOpacity;
#endif

	outFragColor = c;
}

void op_CLEAR() {
	outFragColor = vec4(0);
}
