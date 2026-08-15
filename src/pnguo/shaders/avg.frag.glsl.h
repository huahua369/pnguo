#version 450
#extension GL_EXT_scalar_block_layout : enable
layout(row_major) uniform;
layout(row_major) buffer;
// Constant ID
layout(constant_id = 0) const int NUM_SAMPLES = 8;

// Binding layouts (Vulkan)
layout(scalar, binding = 0, set = 3) uniform uboGrad_ {
	vec4  colors[32];
	float stops[32];
	vec4  cp[2];
	ivec4 m;
	vec2  scale;
	uint  count;
	int   extend;
} uboGrad;

layout(binding = 0, set = 2) uniform sampler2D source;

// VS I/O
layout(location = 0) in vec4		inSrc;		//source bounds or color depending on pattern type
layout(location = 1) in vec2		inUV;	//if it is a text drawing, inFontUV.z hold fontMap layer
layout(location = 2) in flat int	inPatType;	//pattern type
layout(location = 3) in flat float	inOpacity;
layout(location = 4) in mat3x2		inMat;

layout(location = 0) out vec4 fragColor;

// Constants
const int FULLSCREEN_BIT = 0x10000000;
const int SRCTYPE_MASK = 0x000000FF;
const int SOLID = 0;
const int SURFACE = 1;
const int LINEAR = 2;
const int RADIAL = 3;
const int MESH = 4;
const int RASTER_SOURCE = 5;
const int SWEEP = 6;

/* ---------- Helpers ---------- */

vec4 gpu_stop_color(int i, out float offset) {
	offset = uboGrad.stops[i];
	return uboGrad.colors[i];
}

float gpu_extend_t(float t, int extend) {
	if (extend == 1) { // REPEAT
		return t - floor(t);
	}
	else if (extend == 2) { // REFLECT
		float u = t - 2.0 * floor(t * 0.5);
		return u > 1.0 ? 2.0 - u : u;
	}
	return clamp(t, 0.0, 1.0); // PAD
}

vec4 gpu_eval_stops(int stop_count, float t) {
	float off_prev;
	vec4 col_prev = gpu_stop_color(0, off_prev);
	if (t <= off_prev)
		return col_prev;

	for (int i = 1; i < stop_count; ++i) {
		float off;
		vec4 col = gpu_stop_color(i, off);
		if (t <= off) {
			float span = off - off_prev;
			float f = span > 1e-6 ? (t - off_prev) / span : 0.0;
			vec4 pm = mix(col_prev, col, f);
			return pm.a > 1e-6 ? pm : vec4(0.0);
		}
		col_prev = col;
		off_prev = off;
	}
	return col_prev;
}

vec2 gpu_apply_minv(ivec4 m, vec2 v) {
	vec4 mf = vec4(m) * (1.0 / 1024.0);
	return vec2(
		mf.x * v.x + mf.y * v.y,
		mf.z * v.x + mf.w * v.y
	);
}

/* ---------- Gradients ---------- */

vec4 gpu_sample_linear(vec2 renderCoord, vec2 box, int stop_count, int extend) {
	vec4 cp = uboGrad.cp[0];
	vec2 p0 = cp.xy / box;
	vec2 d = (cp.zw / box) - p0;

	float denom = dot(d, d);
	if (denom < 1e-6) return vec4(0.0);

	vec2 p = (renderCoord - cp.xy) / box;
	p = gpu_apply_minv(uboGrad.m, p);

	float t = dot(p, d) / denom;
	t = gpu_extend_t(t, extend);
	return gpu_eval_stops(stop_count, t);
}

vec4 gpu_sample_radial(vec2 renderCoord, vec2 box, int stop_count, int extend) {
	vec4 cp0 = uboGrad.cp[0];
	vec4 cp1 = uboGrad.cp[1];

	vec2 c0_r = cp0.xy / box;
	vec2 cd = (cp1.xy / box) - c0_r;
	float r0 = cp0.z / box.x;
	float r1 = cp1.z / box.x;
	float dr = r1 - r0;

	vec2 p = gpu_apply_minv(uboGrad.m, (renderCoord - cp0.xy) / box);

	float A = dot(cd, cd) - dr * dr;
	float B = -2.0 * (dot(p, cd) + r0 * dr);
	float C = dot(p, p) - r0 * r0;

	float t;
	if (abs(A) > 1e-6) {
		float disc = B * B - 4.0 * A * C;
		if (disc < 0.0) return vec4(0.0);
		float sq = sqrt(disc);
		float t1 = (-B + sq) / (2.0 * A);
		float t2 = (-B - sq) / (2.0 * A);
		t = (r0 + t1 * dr >= 0.0) ? t1 : t2;
	}
	else {
		if (abs(B) < 1e-6) return vec4(0.0);
		t = -C / B;
	}

	t = gpu_extend_t(t, extend);
	return gpu_eval_stops(stop_count, t);
}

vec4 gpu_sample_sweep(vec2 renderCoord, vec2 box, int stop_count, int extend) {
	vec4 cp = uboGrad.cp[0];
	vec2 p0 = cp.xy / box;
	float a0 = cp.z;
	float a1 = cp.w;
	float span = a1 - a0;
	if (abs(span) < 1e-6) return vec4(0.0);

	vec2 p = normalize(renderCoord - cp.xy);
	p = gpu_apply_minv(uboGrad.m, normalize((renderCoord / box) - p0));

	float ang = atan(p.y, p.x) / 3.14159265358979;
	if (ang < 0.0) ang += 2.0;

	float t = (ang - a0) / span;
	t = gpu_extend_t(t, extend);
	return gpu_eval_stops(stop_count, t);
}

/* ---------- Composite ---------- */

vec4 gpu_composite(vec4 src, vec4 dst, int mode) {
	vec4 r = src + dst * (1.0 - src.a); // SRC_OVER default

	if (mode == 14 || mode == 18 || mode == 19) mode = 23;
	else if (mode == 17 || mode == 20) mode = 13;

	if (mode == 0)       r = vec4(0.0);
	else if (mode == 1)  r = src;
	else if (mode == 2)  r = dst;
	else if (mode == 4)  r = dst + src * (1.0 - dst.a);
	else if (mode == 5)  r = src * dst.a;
	else if (mode == 6)  r = dst * src.a;
	else if (mode == 7)  r = src * (1.0 - dst.a);
	else if (mode == 8)  r = dst * (1.0 - src.a);
	else if (mode == 9)  r = src * dst.a + dst * (1.0 - src.a);
	else if (mode == 10) r = dst * src.a + src * (1.0 - dst.a);
	else if (mode == 11) r = src * (1.0 - dst.a) + dst * (1.0 - src.a);
	else if (mode == 12) r = min(src + dst, vec4(1.0));
	else if (mode == 13) {
		r.rgb = src.rgb + dst.rgb - src.rgb * dst.rgb;
		r.a = src.a + dst.a - src.a * dst.a;
	}
	else if (mode == 15) {
		r.rgb = min(src.rgb * dst.a, dst.rgb * src.a)
			+ src.rgb * (1.0 - dst.a)
			+ dst.rgb * (1.0 - src.a);
		r.a = src.a + dst.a - src.a * dst.a;
	}
	else if (mode == 16) {
		r.rgb = max(src.rgb * dst.a, dst.rgb * src.a)
			+ src.rgb * (1.0 - dst.a)
			+ dst.rgb * (1.0 - src.a);
		r.a = src.a + dst.a - src.a * dst.a;
	}
	else if (mode == 23) {
		r.rgb = src.rgb * (1.0 - dst.a)
			+ dst.rgb * (1.0 - src.a)
			+ src.rgb * dst.rgb;
		r.a = src.a + dst.a - src.a * dst.a;
	}

	return r;
}

/* ---------- Paint ---------- */

vec4 gpu_paint(vec2 renderCoord, vec4 inSrc, mat3x2 inMat, int inPatType) {
	vec2 box = inSrc.xy * uboGrad.scale;
	int extend = uboGrad.extend;
	int stop_count = int(uboGrad.count);
	vec4 col = inSrc;

	switch (inPatType) {
	case SURFACE: {
		vec2 p = renderCoord.xy - inSrc.xy;
		vec2 uv = (inMat * vec3(p, 1.0)).xy;
		uv /= inSrc.zw;
		col = texture(source, uv);
		break;
	}
	case LINEAR:
		col = gpu_sample_linear(renderCoord, box, stop_count, extend);
		break;
	case RADIAL:
		col = gpu_sample_radial(renderCoord, box, stop_count, extend);
		break;
	case SWEEP:
		col = gpu_sample_sweep(renderCoord, box, stop_count, extend);
		break;
	}

	vec4 src = vec4(col.rgb * col.a, col.a);
	return src + vec4(0.0) * (1.0 - src.a); // acc placeholder
}

/* ---------- Fragment Main ---------- */

void main() {
	vec4 c = gpu_paint(gl_FragCoord.xy, inSrc, inMat, inPatType);
	c *= inOpacity;
	fragColor = c;
}
