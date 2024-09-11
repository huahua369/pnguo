/*
 * Copyright (c) 2019-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2019-2024, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NVVKHL_BSDF_FUNCTIONS_H
#define NVVKHL_BSDF_FUNCTIONS_H 1

 //#include "func.h"            // cosineSampleHemisphere
 //#include "ggx.h"             // brdfLambertian, ..
 //#include "bsdf_structs.h"    // Bsdf*,
 //#include "pbr_mat_struct.h"  // PbrMaterial

#ifdef __cplusplus
#define OUT_TYPE(T) T&
#define INOUT_TYPE(T) T&
#define ARRAY_TYPE(T, N, name) std::array<T, N> name
#else
#define OUT_TYPE(T) out T
#define INOUT_TYPE(T) inout T
#define ARRAY_TYPE(T, N, name) T name[N]
#define inline
#endif



#ifndef funch__


inline float square(float x)
{
	return x * x;
}

inline float saturate(float x)
{
	return clamp(x, 0.0F, 1.0F);
}

inline vec3 saturate(vec3 x)
{
	return clamp(x, vec3(0.0F), vec3(1.0F));
}

// Return the luminance of a color
inline float luminance(vec3 color)
{
	return color.x * 0.2126F + color.y * 0.7152F + color.z * 0.0722F;
}

inline vec3 slerp(vec3 a, vec3 b, float angle, float t)
{
	t = saturate(t);
	float sin1 = sin(angle * t);
	float sin2 = sin(angle * (1.0F - t));
	float ta = sin1 / (sin1 + sin2);
	vec3  result = mix(a, b, ta);
	return normalize(result);
}

inline float clampedDot(vec3 x, vec3 y)
{
	return clamp(dot(x, y), 0.0F, 1.0F);
}

//-----------------------------------------------------------------------------
// Builds an orthonormal basis: given only a normal vector, returns a
// tangent and bitangent.
//
// This uses the technique from "Improved accuracy when building an orthonormal
// basis" by Nelson Max, https://jcgt.org/published/0006/01/02.
// Any tangent-generating algorithm must produce at least one discontinuity
// when operating on a sphere (due to the hairy ball theorem); this has a
// small ring-shaped discontinuity at normal.z == -0.99998796.
//-----------------------------------------------------------------------------
inline void orthonormalBasis(vec3 normal, OUT_TYPE(vec3) tangent, OUT_TYPE(vec3) bitangent)
{
	if (normal.z < -0.99998796F)  // Handle the singularity
	{
		tangent = vec3(0.0F, -1.0F, 0.0F);
		bitangent = vec3(-1.0F, 0.0F, 0.0F);
		return;
	}
	float a = 1.0F / (1.0F + normal.z);
	float b = -normal.x * normal.y * a;
	tangent = vec3(1.0F - normal.x * normal.x * a, b, -normal.x);
	bitangent = vec3(b, 1.0f - normal.y * normal.y * a, -normal.y);
}
 
//-----------------------------------------------------------------------------
// Like orthonormalBasis(), but returns a tangent and tangent sign that matches
// the glTF convention.
inline vec4 makeFastTangent(vec3 normal)
{
	vec3 tangent, unused;
	orthonormalBasis(normal, tangent, unused);
	// The glTF bitangent sign here is 1.f since for
	// normal == vec3(0.0F, 0.0F, 1.0F), we get
	// tangent == vec3(1.0F, 0.0F, 0.0F) and bitangent == vec3(0.0F, 1.0F, 0.0F),
	// so bitangent = cross(normal, tangent).
	return vec4(tangent, 1.f);
}

inline vec3 rotate(vec3 v, vec3 k, float theta)
{
	float cos_theta = cos(theta);
	float sin_theta = sin(theta);

	return (v * cos_theta) + (cross(k, v) * sin_theta) + (k * dot(k, v)) * (1.0F - cos_theta);
}


//-----------------------------------------------------------------------
// Return the UV in a lat-long HDR map
//-----------------------------------------------------------------------
inline vec2 getSphericalUv(vec3 v)
{
	float gamma = asin(-v.y);
	float theta = atan(v.z, v.x);

	vec2 uv = vec2(theta * M_1_OVER_PI * 0.5F, gamma * M_1_OVER_PI) + 0.5F;
	return uv;
}

//-----------------------------------------------------------------------
// Return the interpolated value between 3 values and the barycentrics
//-----------------------------------------------------------------------
inline vec2 mixBary(vec2 a, vec2 b, vec2 c, vec3 bary)
{
	return a * bary.x + b * bary.y + c * bary.z;
}

inline vec3 mixBary(vec3 a, vec3 b, vec3 c, vec3 bary)
{
	return a * bary.x + b * bary.y + c * bary.z;
}

inline vec4 mixBary(vec4 a, vec4 b, vec4 c, vec3 bary)
{
	return a * bary.x + b * bary.y + c * bary.z;
}

//-----------------------------------------------------------------------
// https://www.realtimerendering.com/raytracinggems/unofficial_RayTracingGems_v1.4.pdf
// 16.6.1 COSINE-WEIGHTED HEMISPHERE ORIENTED TO THE Z-AXIS
//-----------------------------------------------------------------------
inline vec3 cosineSampleHemisphere(float r1, float r2)
{
	float r = sqrt(r1);
	float phi = M_TWO_PI * r2;
	vec3  dir;
	dir.x = r * cos(phi);
	dir.y = r * sin(phi);
	dir.z = sqrt(max(0.F, 1.F - dir.x * dir.x - dir.y * dir.y));
	return dir;
}

#endif // !funch__



#ifndef ggx


float schlickFresnel(float F0, float F90, float VdotH)
{
	return F0 + (F90 - F0) * pow(1.0F - VdotH, 5.0F);
}

vec3 schlickFresnel(vec3 F0, vec3 F90, float VdotH)
{
	return F0 + (F90 - F0) * pow(1.0F - VdotH, 5.0F);
}

float schlickFresnel(float ior, float VdotH)
{
	// Calculate reflectance at normal incidence (R0)
	float R0 = pow((1.0F - ior) / (1.0F + ior), 2.0F);

	// Fresnel reflectance using Schlick's approximation
	return R0 + (1.0F - R0) * pow(1.0F - VdotH, 5.0F);
}


//-----------------------------------------------------------------------
// MDL-based functions
//-----------------------------------------------------------------------

vec3 mix_rgb(const vec3 base, const vec3 layer, const vec3 factor)
{
	return (1.0f - max(factor.x, max(factor.y, factor.z))) * base + factor * layer;
}

// Square the input
float sqr(float x)
{
	return x * x;
}

// Check for total internal reflection.
bool isTIR(const vec2 ior, const float kh)
{
	const float b = ior.x / ior.y;
	return (1.0f < (b * b * (1.0f - kh * kh)));
}


// Evaluate anisotropic GGX distribution on the non-projected hemisphere.
float hvd_ggx_eval(const vec2 invRoughness,
	const vec3 h)  // == vec3(dot(tangent, h), dot(bitangent, h), dot(normal, h))
{
	const float x = h.x * invRoughness.x;
	const float y = h.y * invRoughness.y;
	const float aniso = x * x + y * y;
	const float f = aniso + h.z * h.z;

	return M_1_PI * invRoughness.x * invRoughness.y * h.z / (f * f);
}

// Samples a visible (Smith-masked) half vector according to the anisotropic GGX distribution
// (see Eric Heitz - A Simpler and Exact Sampling Routine for the GGX Distribution of Visible
// normals)
// The input and output will be in local space:
// vec3(dot(T, k1), dot(B, k1), dot(N, k1)).
vec3 hvd_ggx_sample_vndf(vec3 k, vec2 roughness, vec2 xi)
{
	const vec3 v = normalize(vec3(k.x * roughness.x, k.y * roughness.y, k.z));

	const vec3 t1 = (v.z < 0.99999f) ? normalize(cross(v, vec3(0.0f, 0.0f, 1.0f))) : vec3(1.0f, 0.0f, 0.0f);
	const vec3 t2 = cross(t1, v);

	const float a = 1.0f / (1.0f + v.z);
	const float r = sqrt(xi.x);

	const float phi = (xi.y < a) ? xi.y / a * M_PI : M_PI + (xi.y - a) / (1.0f - a) * M_PI;
	float       sp = sin(phi);
	float       cp = cos(phi);
	const float p1 = r * cp;
	const float p2 = r * sp * ((xi.y < a) ? 1.0f : v.z);

	vec3 h = p1 * t1 + p2 * t2 + sqrt(max(0.0f, 1.0f - p1 * p1 - p2 * p2)) * v;

	h.x *= roughness.x;
	h.y *= roughness.y;
	h.z = max(0.0f, h.z);
	return normalize(h);
}

// Smith-masking for anisotropic GGX.
float smith_shadow_mask(const vec3 k, const vec2 roughness)
{
	float kz2 = k.z * k.z;
	if (kz2 == 0.0f)
	{
		return 0.0f;  // Totally shadowed
	}
	const float ax = k.x * roughness.x;
	const float ay = k.y * roughness.y;
	const float inv_a2 = (ax * ax + ay * ay) / kz2;

	return 2.0f / (1.0f + sqrt(1.0f + inv_a2));
}


float ggx_smith_shadow_mask(OUT_TYPE(float) G1, OUT_TYPE(float) G2, const vec3 k1, const vec3 k2, const vec2 roughness)
{
	G1 = smith_shadow_mask(k1, roughness);
	G2 = smith_shadow_mask(k2, roughness);

	return G1 * G2;
}


// Compute squared norm of s/p polarized Fresnel reflection coefficients and phase shifts in complex unit circle.
// Born/Wolf - "Principles of Optics", section 13.4
vec2 fresnel_conductor(OUT_TYPE(vec2) phase_shift_sin,
	OUT_TYPE(vec2) phase_shift_cos,
	const float n_a,
	const float n_b,
	const float k_b,
	const float cos_a,
	const float sin_a_sqd)
{
	const float k_b2 = k_b * k_b;
	const float n_b2 = n_b * n_b;
	const float n_a2 = n_a * n_a;
	const float tmp0 = n_b2 - k_b2;
	const float half_U = 0.5f * (tmp0 - n_a2 * sin_a_sqd);
	const float half_V = sqrt(max(0.0f, half_U * half_U + k_b2 * n_b2));

	const float u_b2 = half_U + half_V;
	const float v_b2 = half_V - half_U;
	const float u_b = sqrt(max(0.0f, u_b2));
	const float v_b = sqrt(max(0.0f, v_b2));

	const float tmp1 = tmp0 * cos_a;
	const float tmp2 = n_a * u_b;
	const float tmp3 = (2.0f * n_b * k_b) * cos_a;
	const float tmp4 = n_a * v_b;
	const float tmp5 = n_a * cos_a;

	const float tmp6 = (2.0f * tmp5) * v_b;
	const float tmp7 = (u_b2 + v_b2) - tmp5 * tmp5;

	const float tmp8 = (2.0f * tmp5) * ((2.0f * n_b * k_b) * u_b - tmp0 * v_b);
	const float tmp9 = sqr((n_b2 + k_b2) * cos_a) - n_a2 * (u_b2 + v_b2);

	const float tmp67 = tmp6 * tmp6 + tmp7 * tmp7;
	const float inv_sqrt_x = (0.0f < tmp67) ? (1.0f / sqrt(tmp67)) : 0.0f;
	const float tmp89 = tmp8 * tmp8 + tmp9 * tmp9;
	const float inv_sqrt_y = (0.0f < tmp89) ? (1.0f / sqrt(tmp89)) : 0.0f;

	phase_shift_cos = vec2(tmp7 * inv_sqrt_x, tmp9 * inv_sqrt_y);
	phase_shift_sin = vec2(tmp6 * inv_sqrt_x, tmp8 * inv_sqrt_y);

	return vec2((sqr(tmp5 - u_b) + v_b2) / (sqr(tmp5 + u_b) + v_b2),
		(sqr(tmp1 - tmp2) + sqr(tmp3 - tmp4)) / (sqr(tmp1 + tmp2) + sqr(tmp3 + tmp4)));
}


// Simplified for dielectric, no phase shift computation.
vec2 fresnel_dielectric(const float n_a, const float n_b, const float cos_a, const float cos_b)
{
	const float naca = n_a * cos_a;
	const float nbcb = n_b * cos_b;
	const float r_s = (naca - nbcb) / (naca + nbcb);

	const float nacb = n_a * cos_b;
	const float nbca = n_b * cos_a;
	const float r_p = (nbca - nacb) / (nbca + nacb);

	return vec2(r_s * r_s, r_p * r_p);
}


vec3 thin_film_factor(float coating_thickness, const float coating_ior, const float base_ior, const float incoming_ior, const float kh)
{
	coating_thickness = max(0.0f, coating_thickness);

	const float sin0_sqr = max(0.0f, 1.0f - kh * kh);
	const float eta01 = incoming_ior / coating_ior;
	const float eta01_sqr = eta01 * eta01;
	const float sin1_sqr = eta01_sqr * sin0_sqr;

	if (1.0f < sin1_sqr)  // TIR at first interface
	{
		return vec3(1.0f);
	}

	const float cos1 = sqrt(max(0.0f, 1.0f - sin1_sqr));
	const vec2  R01 = fresnel_dielectric(incoming_ior, coating_ior, kh, cos1);

	vec2       phi12_sin, phi12_cos;
	const vec2 R12 = fresnel_conductor(phi12_sin, phi12_cos, coating_ior, base_ior, /* base_k = */ 0.0f, cos1, sin1_sqr);

	const float tmp = (4.0f * M_PI) * coating_ior * coating_thickness * cos1;

	const float R01R12_s = max(0.0f, R01.x * R12.x);
	const float r01r12_s = sqrt(R01R12_s);

	const float R01R12_p = max(0.0f, R01.y * R12.y);
	const float r01r12_p = sqrt(R01R12_p);

	vec3 xyz = vec3(0.0f);

	//!! using low res color matching functions here
	float lambda_min = 400.0f;
	float lambda_step = ((700.0f - 400.0f) / 16.0f);

	const vec3 cie_xyz[16] = { {0.02986f, 0.00310f, 0.13609f}, {0.20715f, 0.02304f, 0.99584f},
							  {0.36717f, 0.06469f, 1.89550f}, {0.28549f, 0.13661f, 1.67236f},
							  {0.08233f, 0.26856f, 0.76653f}, {0.01723f, 0.48621f, 0.21889f},
							  {0.14400f, 0.77341f, 0.05886f}, {0.40957f, 0.95850f, 0.01280f},
							  {0.74201f, 0.97967f, 0.00060f}, {1.03325f, 0.84591f, 0.00000f},
							  {1.08385f, 0.62242f, 0.00000f}, {0.79203f, 0.36749f, 0.00000f},
							  {0.38751f, 0.16135f, 0.00000f}, {0.13401f, 0.05298f, 0.00000f},
							  {0.03531f, 0.01375f, 0.00000f}, {0.00817f, 0.00317f, 0.00000f} };

	float lambda = lambda_min + 0.5f * lambda_step;

	for (int i = 0; i < 16; ++i)
	{
		const float phi = tmp / lambda;

		float phi_s = sin(phi);
		float phi_c = cos(phi);

		const float cos_phi_s = phi_c * phi12_cos.x - phi_s * phi12_sin.x;  // cos(a+b) = cos(a) * cos(b) - sin(a) * sin(b)
		const float tmp_s = 2.0f * r01r12_s * cos_phi_s;
		const float R_s = (R01.x + R12.x + tmp_s) / (1.0f + R01R12_s + tmp_s);

		const float cos_phi_p = phi_c * phi12_cos.y - phi_s * phi12_sin.y;  // cos(a+b) = cos(a) * cos(b) - sin(a) * sin(b)
		const float tmp_p = 2.0f * r01r12_p * cos_phi_p;
		const float R_p = (R01.y + R12.y + tmp_p) / (1.0f + R01R12_p + tmp_p);

		xyz += cie_xyz[i] * (0.5f * (R_s + R_p));

		lambda += lambda_step;
	}

	xyz *= (1.0f / 16.0f);

	// ("normalized" such that the loop for no shifted wave gives reflectivity (1,1,1))
	return clamp(vec3(xyz.x * (3.2406f / 0.433509f) + xyz.y * (-1.5372f / 0.433509f) + xyz.z * (-0.4986f / 0.433509f),
		xyz.x * (-0.9689f / 0.341582f) + xyz.y * (1.8758f / 0.341582f) + xyz.z * (0.0415f / 0.341582f),
		xyz.x * (0.0557f / 0.32695f) + xyz.y * (-0.204f / 0.32695f) + xyz.z * (1.057f / 0.32695f)),
		0.0f, 1.0f);
}


// Compute half vector (convention: pointing to outgoing direction, like shading normal)
vec3 compute_half_vector(const vec3 k1, const vec3 k2, const vec3 normal, const vec2 ior, const float nk2, const bool transmission, const bool thinwalled)
{
	vec3 h;

	if (transmission)
	{
		if (thinwalled)  // No refraction!
		{
			h = k1 + (normal * (nk2 + nk2) + k2);  // Use corresponding reflection direction.
		}
		else
		{
			h = k2 * ior.y + k1 * ior.x;  // Points into thicker medium.

			if (ior.y > ior.x)
			{
				h *= -1.0f;  // Make pointing to outgoing direction's medium.
			}
		}
	}
	else
	{
		h = k1 + k2;  // unnormalized half-vector
	}

	return normalize(h);
}

vec3 refract(const vec3  k,       // direction (pointing from surface)
	const vec3  n,       // normal
	const float b,       // (reflected side IOR) / (transmitted side IOR)
	const float nk,      // dot(n, k)
	OUT_TYPE(bool) tir)  // total internal reflection
{
	const float refraction = b * b * (1.0f - nk * nk);

	tir = (1.0f <= refraction);

	return (tir) ? (n * (nk + nk) - k) : normalize((-k * b + n * (b * nk - sqrt(1.0f - refraction))));
}


// Fresnel equation for an equal mix of polarization.
float ior_fresnel(const float eta,  // refracted / reflected ior
	const float kh)   // cosine of angle between normal/half-vector and direction
{
	float costheta = 1.0f - (1.0f - kh * kh) / (eta * eta);

	if (costheta <= 0.0f)
	{
		return 1.0f;
	}

	costheta = sqrt(costheta);  // refracted angle cosine

	const float n1t1 = kh;
	const float n1t2 = costheta;
	const float n2t1 = kh * eta;
	const float n2t2 = costheta * eta;
	const float r_p = (n1t2 - n2t1) / (n1t2 + n2t1);
	const float r_o = (n1t1 - n2t2) / (n1t1 + n2t2);

	const float fres = 0.5f * (r_p * r_p + r_o * r_o);

	return clamp(fres, 0.0f, 1.0f);
}

// Evaluate anisotropic sheen half-vector distribution on the non-projected hemisphere.
float hvd_sheen_eval(const float invRoughness,
	const float nh)  // dot(shading_normal, h)
{
	const float sinTheta2 = max(0.0f, 1.0f - nh * nh);
	const float sinTheta = sqrt(sinTheta2);

	return (invRoughness + 2.0f) * pow(sinTheta, invRoughness) * 0.5f * M_1_PI * nh;
}


// Cook-Torrance style v-cavities masking term.
float vcavities_mask(const float nh,  // abs(dot(normal, half))
	const float kh,  // abs(dot(dir, half))
	const float nk)  // abs(dot(normal, dir))
{
	return min(2.0f * nh * nk / kh, 1.0f);
}


float vcavities_shadow_mask(OUT_TYPE(float) G1, OUT_TYPE(float) G2, const float nh, const vec3 k1, const float k1h, const vec3 k2, const float k2h)
{
	G1 = vcavities_mask(nh, k1h, k1.z);  // In my renderer the z-coordinate is the normal!
	G2 = vcavities_mask(nh, k2h, k2.z);

	//return (refraction) ? fmaxf(0.0f, G1 + G2 - 1.0f) : fminf(G1, G2);
	return min(G1, G2);  // PERF Need reflection only.
}


// Sample half-vector according to anisotropic sheen distribution.
vec3 hvd_sheen_sample(const vec2 xi, const float invRoughness)
{
	const float phi = 2.0f * M_PI * xi.x;

	float sinPhi = sin(phi);
	float cosPhi = cos(phi);

	const float sinTheta = pow(1.0f - xi.y, 1.0f / (invRoughness + 2.0f));
	const float cosTheta = sqrt(1.0f - sinTheta * sinTheta);

	return normalize(vec3(cosPhi * sinTheta, sinPhi * sinTheta,
		cosTheta));  // In my renderer the z-coordinate is the normal!
}

vec3 flip(const vec3 h, const vec3 k, float xi)
{
	const float a = h.z * k.z;
	const float b = h.x * k.x + h.y * k.y;

	const float kh = max(0.0f, a + b);
	const float kh_f = max(0.0f, a - b);

	const float p_flip = kh_f / (kh + kh_f);

	// PERF xi is not used after this operation by the only caller brdf_sheen_sample(),
	// so there is no need to scale the sample.
	//if (xi < p_flip)
	//{
	//  xi /= p_flip;
	//  return make_float3(-h.x, -h.y, h.z);
	//}
	//else
	//{
	//  xi = (xi - p_flip) / (1.0f - p_flip);
	//  return h;
	//}

	return (xi < p_flip) ? vec3(-h.x, -h.y, h.z) : h;
}

#endif // ggx


#define BSDF_EVENT_ABSORB 0               // 0
#define BSDF_EVENT_DIFFUSE 1              // 1
#define BSDF_EVENT_GLOSSY (1 << 1)        // 2
#define BSDF_EVENT_SPECULAR (1 << 2)      // 4
#define BSDF_EVENT_REFLECTION (1 << 3)    // 8
#define BSDF_EVENT_TRANSMISSION (1 << 4)  // 16

#define BSDF_EVENT_DIFFUSE_REFLECTION (BSDF_EVENT_DIFFUSE | BSDF_EVENT_REFLECTION)        // 9
#define BSDF_EVENT_DIFFUSE_TRANSMISSION (BSDF_EVENT_DIFFUSE | BSDF_EVENT_TRANSMISSION)    // 17
#define BSDF_EVENT_GLOSSY_REFLECTION (BSDF_EVENT_GLOSSY | BSDF_EVENT_REFLECTION)          // 10
#define BSDF_EVENT_GLOSSY_TRANSMISSION (BSDF_EVENT_GLOSSY | BSDF_EVENT_TRANSMISSION)      // 18
#define BSDF_EVENT_SPECULAR_REFLECTION (BSDF_EVENT_SPECULAR | BSDF_EVENT_REFLECTION)      // 12
#define BSDF_EVENT_SPECULAR_TRANSMISSION (BSDF_EVENT_SPECULAR | BSDF_EVENT_TRANSMISSION)  // 20

#define BSDF_USE_MATERIAL_IOR (-1.0)

/** @DOC_START
# struct BsdfEvaluateData
>  Data structure for evaluating a BSDF
@DOC_END */
struct BsdfEvaluateData
{
	vec3  k1;            // [in] Toward the incoming ray
	vec3  k2;            // [in] Toward the sampled light
	vec3  xi;            // [in] 3 random [0..1]
	vec3  bsdf_diffuse;  // [out] Diffuse contribution
	vec3  bsdf_glossy;   // [out] Specular contribution
	float pdf;           // [out] PDF
};

/** @DOC_START
# struct BsdfSampleData
>  Data structure for sampling a BSDF
@DOC_END  */
struct BsdfSampleData
{
	vec3  k1;             // [in] Toward the incoming ray
	vec3  k2;             // [out] Toward the sampled light
	vec3  xi;             // [in] 3 random [0..1]
	float pdf;            // [out] PDF
	vec3  bsdf_over_pdf;  // [out] contribution / PDF
	int   event_type;     // [out] one of the event above
};


// Define a value to represent an infinite impulse or singularity
#define DIRAC -1.0


/** @DOC_START
# Function absorptionCoefficient
>  Compute the absorption coefficient of the material
@DOC_END */
vec3 absorptionCoefficient(gpuMaterial mat)
{
	float tmp1 = mat.attenuationDistance;
	return tmp1 <= 0.0F ? vec3(0.0F, 0.0F, 0.0F) :
		-vec3(log(mat.attenuationColor.x), log(mat.attenuationColor.y), log(mat.attenuationColor.z)) / tmp1;
}

#define LOBE_DIFFUSE_REFLECTION 0
#define LOBE_SPECULAR_TRANSMISSION 1
#define LOBE_SPECULAR_REFLECTION 2
#define LOBE_METAL_REFLECTION 3
#define LOBE_SHEEN_REFLECTION 4
#define LOBE_CLEARCOAT_REFLECTION 5
#define LOBE_COUNT 6

// The Fresnel factor depends on the cosine between the view vector k1 and the
// half vector, H = normalize(k1 + k2). But during sampling, we don't have k2
// until we sample a microfacet. So instead, we approximate it.
// For a mirror surface, we have H == N. For a perfectly diffuse surface, k2
// is sampled in a cosine distribution around N, so H ~ normalize(k1 + N).
// We ad-hoc interpolate between them using the roughness.
float fresnelCosineApproximation(float VdotN, float roughness)
{
	return mix(VdotN, sqrt(0.5F + 0.5F * VdotN), sqrt(roughness));
}

// Calculate the weights of the individual lobes inside the standard PBR material.
ARRAY_TYPE(float, LOBE_COUNT, ) computeLobeWeights(gpuMaterial mat, float VdotN, INOUT_TYPE(vec3) tint)
{
	float frCoat = 0.0F;
	if (mat.clearcoatFactor > 0.0f)
	{
		float frCosineClearcoat = fresnelCosineApproximation(VdotN, mat.clearcoatRoughness);
		frCoat = mat.clearcoatFactor * ior_fresnel(1.5f / mat.ior1, frCosineClearcoat);
	}

	// This Fresnel value defines the weighting between dielectric specular reflection and
	// the base dielectric BXDFs (diffuse reflection and specular transmission).
	float frDielectric = 0;
	if (mat.specular > 0)
	{
		float frCosineDielectric = fresnelCosineApproximation(VdotN, (mat.roughness.x + mat.roughness.y) * 0.5F);
		frDielectric = ior_fresnel(mat.ior2 / mat.ior1, frCosineDielectric);
		frDielectric *= mat.specular;
	}

	// Estimate the iridescence Fresnel factor with the angle to the normal, and
	// blend it in. That's good enough for specular reflections.
	if (mat.iridescence > 0.0f)
	{
		// When there is iridescence enabled, use the maximum of the estimated iridescence factor. (Estimated with VdotN, no half-vector H here.)
		// With the thinfilm decision this handles the mix between non-iridescence and iridescence strength automatically.
		vec3 frIridescence = thin_film_factor(mat.iridescenceThickness, mat.iridescenceIor, mat.ior2, mat.ior1, VdotN);
		frDielectric = mix(frDielectric, max(frIridescence.x, max(frIridescence.y, frIridescence.z)), mat.iridescence);
		// Modulate the dielectric base lobe (diffuse, transmission) colors by the inverse of the iridescence factor,
		// though use the maximum component to not actually generate inverse colors.
		tint = mix_rgb(tint, mat.specularColor, frIridescence * mat.iridescence);
	}

	float sheen = 0.0f;
	if ((mat.sheenColor.r != 0.0F || mat.sheenColor.g != 0.0F || mat.sheenColor.b != 0.0F))
	{
		sheen = pow(1.0f - abs(VdotN), mat.sheenRoughness);  // * luminance(mat.sheenColor);
		sheen = sheen / (sheen + 0.5F);
	}

	/*
	Lobe weights:

	  - Clearcoat       : clearcoat * schlickFresnel(1.5, VdotN)
	  - Sheen           : sheen
	  - Metal           : metallic
	  - Specular        : specular * schlickFresnel(ior, VdotN)
	  - Transmission    : transmission
	  - Diffuse         : 1.0 - clearcoat - sheen - metallic - specular - transmission
	*/

	ARRAY_TYPE(float, LOBE_COUNT, weightLobe);
	weightLobe[LOBE_CLEARCOAT_REFLECTION] = 0;
	weightLobe[LOBE_SHEEN_REFLECTION] = 0;
	weightLobe[LOBE_METAL_REFLECTION] = 0;
	weightLobe[LOBE_SPECULAR_REFLECTION] = 0;
	weightLobe[LOBE_SPECULAR_TRANSMISSION] = 0;
	weightLobe[LOBE_DIFFUSE_REFLECTION] = 0;

	float weightBase = 1.0F;

	weightLobe[LOBE_CLEARCOAT_REFLECTION] = frCoat;  // BRDF clearcoat reflection (GGX-Smith)
	weightBase *= 1.0f - frCoat;

	weightLobe[LOBE_SHEEN_REFLECTION] = weightBase * sheen;  // BRDF sheen reflection (Lambert)
	weightBase *= 1.0f - sheen;

	weightLobe[LOBE_METAL_REFLECTION] = weightBase * mat.metallic;  // BRDF metal (GGX-Smith)
	weightBase *= 1.0f - mat.metallic;

	weightLobe[LOBE_SPECULAR_REFLECTION] = weightBase * frDielectric;  // BRDF dielectric specular reflection (GGX-Smith)
	weightBase *= 1.0f - frDielectric;

	weightLobe[LOBE_SPECULAR_TRANSMISSION] = weightBase * mat.transmission;  // BTDF dielectric specular transmission (GGX-Smith)
	weightLobe[LOBE_DIFFUSE_REFLECTION] = weightBase * (1.0f - mat.transmission);  // BRDF diffuse dielectric reflection (Lambert). // PERF Currently not referenced below.

	return weightLobe;
}

// Calculate the weights of the individual lobes inside the standard PBR material
// and randomly select one.
int findLobe(gpuMaterial mat, float VdotN, float rndVal, INOUT_TYPE(vec3) tint)
{
	ARRAY_TYPE(float, LOBE_COUNT, weightLobe) = computeLobeWeights(mat, VdotN, tint);

	int   lobe = LOBE_COUNT;
	float weight = 0.0f;
	while (--lobe > 0)  // Stops when lobe reaches 0!
	{
		weight += weightLobe[lobe];
		if (rndVal < weight)
		{
			break;  // Sample and evaluate this lobe!
		}
	}

	return lobe;  // Last one is the diffuse reflection
}

void brdf_diffuse_eval(INOUT_TYPE(BsdfEvaluateData) data, gpuMaterial mat, vec3 tint)
{
	// If the incoming light direction is on the backside, there is nothing to evaluate for a BRDF.
	// Note that the state normals have been flipped to the ray side by the caller.
	// Include edge-on (== 0.0f) as "no light" case.
	if (dot(data.k2, mat.Ng) <= 0.0f)  // if (backside)
	{
		return;  // absorb
	}

	data.pdf = max(0.0f, dot(data.k2, mat.N) * M_1_PI);

	// For a white Lambert material, the bxdf components match the evaluation pdf. (See MDL_renderer.)
	data.bsdf_diffuse = tint * data.pdf;
}

void brdf_diffuse_eval(INOUT_TYPE(BsdfEvaluateData) data, gpuMaterial mat)
{
	brdf_diffuse_eval(data, mat, vec3(mat.baseColor));
}

void brdf_diffuse_sample(INOUT_TYPE(BsdfSampleData) data, gpuMaterial mat, vec3 tint)
{
	data.k2 = cosineSampleHemisphere(data.xi.x, data.xi.y);
	data.k2 = mat.T * data.k2.x + mat.B * data.k2.y + mat.N * data.k2.z;
	data.k2 = normalize(data.k2);
	data.pdf = dot(data.k2, mat.N) * M_1_PI;

	data.bsdf_over_pdf = tint;  // bsdf * dot(wi, normal) / pdf;
	data.event_type = (0.0f < dot(data.k2, mat.Ng)) ? BSDF_EVENT_DIFFUSE_REFLECTION : BSDF_EVENT_ABSORB;
}

void brdf_diffuse_sample(INOUT_TYPE(BsdfSampleData) data, gpuMaterial mat)
{
	brdf_diffuse_sample(data, mat, vec3(mat.baseColor));
}

void brdf_ggx_smith_eval(INOUT_TYPE(BsdfEvaluateData) data, gpuMaterial mat, const int lobe, vec3 tint)
{
	// BRDF or BTDF eval?
	// If the incoming light direction is on the backface.
	// Include edge-on (== 0.0f) as "no light" case.
	const bool backside = (dot(data.k2, mat.Ng) <= 0.0f);
	// Nothing to evaluate for given directions?
	if (backside && false)  // && scatter_reflect
	{
		data.pdf = 0.0f;
		data.bsdf_glossy = vec3(0.0f);
		return;
	}

	const float nk1 = abs(dot(data.k1, mat.N));
	const float nk2 = abs(dot(data.k2, mat.N));

	// compute_half_vector() for scatter_reflect.
	const vec3 h = normalize(data.k1 + data.k2);

	// Invalid for reflection / refraction?
	const float nh = dot(mat.N, h);
	const float k1h = dot(data.k1, h);
	const float k2h = dot(data.k2, h);

	// nk1 and nh must not be 0.0f or state.pdf == NaN.
	if (nk1 <= 0.0f || nh <= 0.0f || k1h < 0.0f || k2h < 0.0f)
	{
		data.pdf = 0.0f;
		data.bsdf_glossy = vec3(0.0f);
		return;
	}

	// Compute BSDF and pdf.
	const vec3 h0 = vec3(dot(mat.T, h), dot(mat.B, h), nh);

	data.pdf = hvd_ggx_eval(1.0f / mat.roughness, h0);
	float G1;
	float G2;

	float G12;
	G12 = ggx_smith_shadow_mask(G1, G2, vec3(dot(mat.T, data.k1), dot(mat.B, data.k1), nk1),
		vec3(dot(mat.T, data.k2), dot(mat.B, data.k2), nk2), mat.roughness);

	data.pdf *= 0.25f / (nk1 * nh);

	vec3 bsdf = vec3(G12 * data.pdf);

	data.pdf *= G1;

	if (mat.iridescence > 0.0f)
	{
		const vec3 factor = thin_film_factor(mat.iridescenceThickness, mat.iridescenceIor, mat.ior2, mat.ior1, k1h);

		switch (lobe)
		{
		case LOBE_SPECULAR_REFLECTION:
			tint *= mix(vec3(1.f), factor, mat.iridescence);
			break;

		case LOBE_METAL_REFLECTION:
			tint = mix_rgb(tint, mat.specularColor, factor * mat.iridescence);
			break;
		}
	}

	// eval output: (glossy part of the) bsdf * dot(k2, normal)
	data.bsdf_glossy = bsdf * tint;
}

void brdf_ggx_smith_sample(INOUT_TYPE(BsdfSampleData) data, gpuMaterial mat, const int lobe, vec3 tint)
{
	// When the sampling returns eventType = BSDF_EVENT_ABSORB, the path ends inside the ray generation program.
	// Make sure the returned values are valid numbers when manipulating the PRD.
	data.bsdf_over_pdf = vec3(0.0f);
	data.pdf = 0.0f;

	// Transform to local coordinate system
	const float nk1 = dot(data.k1, mat.N);
	if (nk1 <= 0.0f)
	{
		data.event_type = BSDF_EVENT_ABSORB;
		return;
	}
	const vec3 k10 = vec3(dot(data.k1, mat.T), dot(data.k1, mat.B), nk1);

	// Sample half-vector, microfacet normal.
#if __cplusplus
	const vec3 h0 = hvd_ggx_sample_vndf(k10, mat.roughness, data.xi);
#else
	const vec3 h0 = hvd_ggx_sample_vndf(k10, mat.roughness, data.xi.xy);
#endif // __cplusplus

	if (h0.z == 0.0f)
	{
		data.event_type = BSDF_EVENT_ABSORB;
		return;
	}

	// Transform to world
	const vec3  h = h0.x * mat.T + h0.y * mat.B + h0.z * mat.N;
	const float kh = dot(data.k1, h);

	if (kh <= 0.0f)
	{
		data.event_type = BSDF_EVENT_ABSORB;
		return;
	}

	// BRDF: reflect
	data.k2 = (2.0f * kh) * h - data.k1;

	// Check if the resulting direction is on the correct side of the actual geometry
	const float gnk2 = dot(data.k2, mat.Ng);  // * ((data.typeEvent == BSDF_EVENT_GLOSSY_REFLECTION) ? 1.0f : -1.0f);

	if (gnk2 <= 0.0f)
	{
		data.event_type = BSDF_EVENT_ABSORB;
		return;
	}

	const float nk2 = abs(dot(data.k2, mat.N));
	const float k2h = abs(dot(data.k2, h));

	float G1;
	float G2;

	float G12 = ggx_smith_shadow_mask(G1, G2, k10, vec3(dot(data.k2, mat.T), dot(data.k2, mat.B), nk2), mat.roughness);

	if (G12 <= 0.0f)
	{
		data.event_type = BSDF_EVENT_ABSORB;
		return;
	}

	data.bsdf_over_pdf = vec3(G2);
	data.event_type = BSDF_EVENT_GLOSSY_REFLECTION;

	// Compute pdf
	data.pdf = hvd_ggx_eval(1.0f / mat.roughness, h0) * G1;
	data.pdf *= 0.25f / (nk1 * h0.z);

	if (mat.iridescence > 0.0f)
	{
		const vec3 factor = thin_film_factor(mat.iridescenceThickness, mat.iridescenceIor, mat.ior2, mat.ior1, kh);

		switch (lobe)
		{
		case LOBE_SPECULAR_REFLECTION:
			tint *= mix(vec3(1.f), factor, mat.iridescence);
			break;

		case LOBE_METAL_REFLECTION:
			tint = mix_rgb(tint, mat.specularColor, factor * mat.iridescence);
			break;
		}
	}

	data.bsdf_over_pdf *= tint;
}


void btdf_ggx_smith_eval(INOUT_TYPE(BsdfEvaluateData) data, gpuMaterial mat, const vec3 tint)
{
	bool isThinWalled = (mat.thickness == 0.0f);

	const vec2 ior = vec2(mat.ior1, mat.ior2);

	const float nk1 = abs(dot(data.k1, mat.N));
	const float nk2 = abs(dot(data.k2, mat.N));

	// BRDF or BTDF eval?
	// If the incoming light direction is on the backface.
	// Do NOT include edge-on (== 0.0f) as backside here to take the reflection path.
	const bool backside = (dot(data.k2, mat.Ng) < 0.0f);

	const vec3 h = compute_half_vector(data.k1, data.k2, mat.N, ior, nk2, backside, isThinWalled);

	// Invalid for reflection / refraction?
	const float nh = dot(mat.N, h);
	const float k1h = dot(data.k1, h);
	const float k2h = dot(data.k2, h) * (backside ? -1.0f : 1.0f);

	// nk1 and nh must not be 0.0f or state.pdf == NaN.
	if (nk1 <= 0.0f || nh <= 0.0f || k1h < 0.0f || k2h < 0.0f)
	{
		data.pdf = 0.0f;  // absorb
		data.bsdf_glossy = vec3(0.0f);
		return;
	}

	float fr;

	if (!backside)
	{
		// For scatter_transmit: Only allow TIR with BRDF eval.
		if (!isTIR(ior, k1h))
		{
			data.pdf = 0.0f;  // absorb
			data.bsdf_glossy = vec3(0.0f);
			return;
		}
		else
		{
			fr = 1.0f;
		}
	}
	else
	{
		fr = 0.0f;
	}

	// Compute BSDF and pdf
	const vec3 h0 = vec3(dot(mat.T, h), dot(mat.B, h), nh);
	data.pdf = hvd_ggx_eval(1.0f / mat.roughness, h0);

	float G1;
	float G2;
	float G12 = ggx_smith_shadow_mask(G1, G2, vec3(dot(mat.T, data.k1), dot(mat.B, data.k1), nk1),
		vec3(dot(mat.T, data.k2), dot(mat.B, data.k2), nk2), mat.roughness);

	if (!isThinWalled && backside)  // Refraction?
	{
		// Refraction pdf and BTDF
		const float tmp = k1h * ior.x - k2h * ior.y;

		data.pdf *= k1h * k2h / (nk1 * nh * tmp * tmp);
	}
	else
	{
		// Reflection pdf and BRDF (and pseudo-BTDF for thin-walled)
		data.pdf *= 0.25f / (nk1 * nh);
	}

	const float prob = (backside) ? 1.0f - fr : fr;

	const vec3 bsdf = vec3(prob * G12 * data.pdf);

	data.pdf *= prob * G1;

	// eval output: (glossy part of the) bsdf * dot(k2, normal)
	data.bsdf_glossy = bsdf * tint;
}

void btdf_ggx_smith_sample(INOUT_TYPE(BsdfSampleData) data, gpuMaterial mat, const vec3 tint)
{
	bool isThinWalled = (mat.thickness == 0.0f);

	// When the sampling returns eventType = BSDF_EVENT_ABSORB, the path ends inside the ray generation program.
	// Make sure the returned values are valid numbers when manipulating the PRD.
	data.bsdf_over_pdf = vec3(0.0f);
	data.pdf = 0.0f;

	const vec2 ior = vec2(mat.ior1, mat.ior2);

	const float nk1 = abs(dot(data.k1, mat.N));

	const vec3 k10 = vec3(dot(data.k1, mat.T), dot(data.k1, mat.B), nk1);

	// Sample half-vector, microfacet normal.
#if __cplusplus
	const vec3 h0 = hvd_ggx_sample_vndf(k10, mat.roughness, data.xi);
#else
	const vec3 h0 = hvd_ggx_sample_vndf(k10, mat.roughness, data.xi.xy);
#endif

	if (abs(h0.z) == 0.0f)
	{
		data.event_type = BSDF_EVENT_ABSORB;
		return;
	}

	// Transform to world
	const vec3 h = h0.x * mat.T + h0.y * mat.B + h0.z * mat.N;

	const float kh = dot(data.k1, h);

	if (kh <= 0.0f)
	{
		data.event_type = BSDF_EVENT_ABSORB;
		return;
	}

	// Case scatter_transmit
	bool tir = false;
	if (isThinWalled)  // No refraction!
	{
		// pseudo-BTDF: flip a reflected reflection direction to the back side
		data.k2 = (2.0f * kh) * h - data.k1;
		data.k2 = normalize(data.k2 - 2.0f * mat.N * dot(data.k2, mat.N));
	}
	else
	{
		// BTDF: refract
		data.k2 = refract(data.k1, h, ior.x / ior.y, kh, tir);
	}

	data.bsdf_over_pdf = vec3(1.0f);  // Was: (vec3(1.0f) - fr) / prob; // PERF Always white with the original setup.
	data.event_type = (tir) ? BSDF_EVENT_GLOSSY_REFLECTION : BSDF_EVENT_GLOSSY_TRANSMISSION;

	// Check if the resulting direction is on the correct side of the actual geometry
	const float gnk2 = dot(data.k2, mat.Ng) * ((data.event_type == BSDF_EVENT_GLOSSY_REFLECTION) ? 1.0f : -1.0f);

	if (gnk2 <= 0.0f || isnan(data.k2.x))
	{
		data.event_type = BSDF_EVENT_ABSORB;
		return;
	}


	const float nk2 = abs(dot(data.k2, mat.N));
	const float k2h = abs(dot(data.k2, h));

	float G1;
	float G2;
	float G12 = ggx_smith_shadow_mask(G1, G2, k10, vec3(dot(data.k2, mat.T), dot(data.k2, mat.B), nk2), mat.roughness);

	if (G12 <= 0.0f)
	{
		data.event_type = BSDF_EVENT_ABSORB;
		return;
	}

	data.bsdf_over_pdf *= G2;

	// Compute pdf
	data.pdf = hvd_ggx_eval(1.0f / mat.roughness, h0) * G1;  // * prob;


	if (!isThinWalled && (data.event_type == BSDF_EVENT_GLOSSY_TRANSMISSION))  // if (refraction)
	{
		const float tmp = kh * ior.x - k2h * ior.y;
		if (tmp > 0)
		{
			data.pdf *= kh * k2h / (nk1 * h0.z * tmp * tmp);
		}
	}
	else
	{
		data.pdf *= 0.25f / (nk1 * h0.z);
	}

	data.bsdf_over_pdf *= tint;
}

void brdf_sheen_eval(INOUT_TYPE(BsdfEvaluateData) data, gpuMaterial mat)
{
	// BRDF or BTDF eval?
	// If the incoming light direction is on the backface.
	// Include edge-on (== 0.0f) as "no light" case.
	const bool backside = (dot(data.k2, mat.Ng) <= 0.0f);
	// Nothing to evaluate for given directions?
	if (backside)  // && scatter_reflect
	{
		return;  // absorb
	}

	const float nk1 = abs(dot(data.k1, mat.N));
	const float nk2 = abs(dot(data.k2, mat.N));

	// compute_half_vector() for scatter_reflect.
	const vec3 h = normalize(data.k1 + data.k2);

	// Invalid for reflection / refraction?
	const float nh = dot(mat.N, h);
	const float k1h = dot(data.k1, h);
	const float k2h = dot(data.k2, h);

	// nk1 and nh must not be 0.0f or state.pdf == NaN.
	if (nk1 <= 0.0f || nh <= 0.0f || k1h < 0.0f || k2h < 0.0f)
	{
		return;  // absorb
	}

	const float invRoughness = 1.0f / (mat.sheenRoughness * mat.sheenRoughness);  // Perceptual roughness to alpha G.

	// Compute BSDF and pdf
	const vec3 h0 = vec3(dot(mat.T, h), dot(mat.B, h), nh);

	data.pdf = hvd_sheen_eval(invRoughness, h0.z);

	float G1;
	float G2;

	const float G12 = vcavities_shadow_mask(G1, G2, h0.z, vec3(dot(mat.T, data.k1), dot(mat.B, data.k1), nk1), k1h,
		vec3(dot(mat.T, data.k2), dot(mat.B, data.k2), nk2), k2h);
	data.pdf *= 0.25f / (nk1 * nh);

	const vec3 bsdf = vec3(G12 * data.pdf);

	data.pdf *= G1;

	// eval output: (glossy part of the) bsdf * dot(k2, normal)
	data.bsdf_glossy = bsdf * mat.sheenColor;
}

void brdf_sheen_sample(INOUT_TYPE(BsdfSampleData) data, gpuMaterial mat)
{
	// When the sampling returns eventType = BSDF_EVENT_ABSORB, the path ends inside the ray generation program.
	// Make sure the returned values are valid numbers when manipulating the PRD.
	data.bsdf_over_pdf = vec3(0.0f);
	data.pdf = 0.0f;

	const float invRoughness = 1.0f / (mat.sheenRoughness * mat.sheenRoughness);  // Perceptual roughness to alpha G.

	const float nk1 = abs(dot(data.k1, mat.N));

	const vec3 k10 = vec3(dot(data.k1, mat.T), dot(data.k1, mat.B), nk1);

	float      xiFlip = data.xi.z;
#if __cplusplus
	const vec3 h0 = flip(hvd_sheen_sample(data.xi, invRoughness), k10, xiFlip);
#else
	const vec3 h0 = flip(hvd_sheen_sample(data.xi.xy, invRoughness), k10, xiFlip);
#endif

	if (abs(h0.z) == 0.0f)
	{
		data.event_type = BSDF_EVENT_ABSORB;
		return;
	}

	// Transform to world
	const vec3 h = h0.x * mat.T + h0.y * mat.B + h0.z * mat.N;

	const float k1h = dot(data.k1, h);

	if (k1h <= 0.0f)
	{
		data.event_type = BSDF_EVENT_ABSORB;
		return;
	}

	// BRDF: reflect
	data.k2 = (2.0f * k1h) * h - data.k1;
	data.bsdf_over_pdf = vec3(1.0f);  // PERF Always white with the original setup.
	data.event_type = BSDF_EVENT_GLOSSY_REFLECTION;

	// Check if the resulting reflection direction is on the correct side of the actual geometry.
	const float gnk2 = dot(data.k2, mat.Ng);

	if (gnk2 <= 0.0f)
	{
		data.event_type = BSDF_EVENT_ABSORB;
		return;
	}

	const float nk2 = abs(dot(data.k2, mat.N));
	const float k2h = abs(dot(data.k2, h));

	float G1;
	float G2;

	const float G12 = vcavities_shadow_mask(G1, G2, h0.z, k10, k1h, vec3(dot(data.k2, mat.T), dot(data.k2, mat.B), nk2), k2h);
	if (G12 <= 0.0f)
	{
		data.event_type = BSDF_EVENT_ABSORB;
		return;
	}

	data.bsdf_over_pdf *= G12 / G1;

	// Compute pdf.
	data.pdf = hvd_sheen_eval(invRoughness, h0.z) * G1;

	data.pdf *= 0.25f / (nk1 * h0.z);

	data.bsdf_over_pdf *= mat.sheenColor;
}


/** @DOC_START
# Function bsdfEvaluate
>  Evaluate the BSDF for the given material.
@DOC_END */
int bsdfEvaluate(INOUT_TYPE(BsdfEvaluateData) data, gpuMaterial mat)
{
	vec3  tint = vec3(mat.baseColor);
	float VdotN = dot(data.k1, mat.N);
	int   lobe = findLobe(mat, VdotN, data.xi.z, tint);
	data.bsdf_diffuse = vec3(0, 0, 0);
	data.bsdf_glossy = vec3(0, 0, 0);
	data.pdf = 0.0;

	if (lobe == LOBE_DIFFUSE_REFLECTION)
	{
		brdf_diffuse_eval(data, mat, tint);
	}
	else if (lobe == LOBE_SPECULAR_REFLECTION)
	{
		brdf_ggx_smith_eval(data, mat, LOBE_SPECULAR_REFLECTION, mat.specularColor);
	}
	else if (lobe == LOBE_SPECULAR_TRANSMISSION)
	{
		btdf_ggx_smith_eval(data, mat, tint);
	}
	else if (lobe == LOBE_METAL_REFLECTION)
	{
		brdf_ggx_smith_eval(data, mat, LOBE_METAL_REFLECTION, vec3(mat.baseColor));
	}
	else if (lobe == LOBE_CLEARCOAT_REFLECTION)
	{
		mat.roughness = vec2(mat.clearcoatRoughness * mat.clearcoatRoughness);
		mat.N = mat.clearcoatNormal;
		mat.iridescence = 0.0f;
		brdf_ggx_smith_eval(data, mat, LOBE_CLEARCOAT_REFLECTION, vec3(1, 1, 1));
	}
	else if (lobe == LOBE_SHEEN_REFLECTION)
	{
		brdf_sheen_eval(data, mat);
	}
	return lobe;
}

/** @DOC_START
# Function bsdfSample
>  Sample the BSDF for the given material
@DOC_END */
void bsdfSample(INOUT_TYPE(BsdfSampleData) data, gpuMaterial mat)
{
	vec3  tint = vec3(mat.baseColor);
	float VdotN = dot(data.k1, mat.N);
	int   lobe = findLobe(mat, VdotN, data.xi.z, tint);
	data.pdf = 0;
	data.bsdf_over_pdf = vec3(0.0F);
	data.event_type = BSDF_EVENT_ABSORB;

	if (lobe == LOBE_DIFFUSE_REFLECTION)
	{
		brdf_diffuse_sample(data, mat, tint);
	}
	else if (lobe == LOBE_SPECULAR_REFLECTION)
	{
		brdf_ggx_smith_sample(data, mat, LOBE_SPECULAR_REFLECTION, mat.specularColor);
		data.event_type = BSDF_EVENT_SPECULAR;
	}
	else if (lobe == LOBE_SPECULAR_TRANSMISSION)
	{
		btdf_ggx_smith_sample(data, mat, tint);
	}
	else if (lobe == LOBE_METAL_REFLECTION)
	{
		brdf_ggx_smith_sample(data, mat, LOBE_METAL_REFLECTION, vec3(mat.baseColor));
	}
	else if (lobe == LOBE_CLEARCOAT_REFLECTION)
	{
		mat.roughness = vec2(mat.clearcoatRoughness * mat.clearcoatRoughness);
		mat.N = mat.clearcoatNormal;
		mat.B = normalize(cross(mat.N, mat.T));  // Assumes Nc and Tc are not collinear!
		mat.T = cross(mat.B, mat.N);
		mat.iridescence = 0.0f;
		brdf_ggx_smith_sample(data, mat, LOBE_CLEARCOAT_REFLECTION, vec3(1, 1, 1));
	}
	else if (lobe == LOBE_SHEEN_REFLECTION)
	{
		// Sheen is using the state.sheenColor and state.sheenInvRoughness values directly.
		// Only brdf_sheen_sample needs a third random sample for the v-cavities flip. Put this as argument.
		brdf_sheen_sample(data, mat);
	}

	// Avoid internal reflection
	if (data.pdf <= 0.00001F || any(isnan(data.bsdf_over_pdf)))
	{
		data.event_type = BSDF_EVENT_ABSORB;
	}
	if (isnan(data.pdf) || isinf(data.pdf))
	{
		data.pdf = DIRAC;
	}
}

//--------------------------------------------------------------------------------------------------
// Those functions are used to evaluate and sample the BSDF for a simple PBR material.
// without any additional lobes like clearcoat, sheen, etc. and without the need of random numbers.
// This is based on the metallic/roughness BRDF in Appendix B of the glTF specification.
// For one sample of pure reflection, use xi == vec2(0,0).
//--------------------------------------------------------------------------------------------------

// Returns the probability that bsdfSampleSimple samples a glossy lobe.
float bsdfSimpleGlossyProbability(float NdotV, float metallic)
{
	return mix(schlickFresnel(0.04F, 1.0F, NdotV), 1.0F, metallic);
}

void bsdfEvaluateSimple(INOUT_TYPE(BsdfEvaluateData) data, gpuMaterial mat)
{
	// Specular reflection
	vec3  H = normalize(data.k1 + data.k2);
	float NdotV = clampedDot(mat.N, data.k1);
	float NdotL = clampedDot(mat.N, data.k2);
	float VdotH = clampedDot(data.k1, H);
	float NdotH = clampedDot(mat.N, H);

	if (NdotV == 0.0f || NdotL == 0.0f || VdotH == 0.0f || NdotH == 0.0f)
	{
		data.bsdf_diffuse = data.bsdf_glossy = vec3(0.0f);
		data.pdf = 0.0f;
		return;
	}

	// We combine the metallic and specular lobes into a single glossy lobe.
	// The metallic weight is     metallic *    fresnel(f0 = baseColor)
	// The specular weight is (1-metallic) *    fresnel(f0 = c_min_reflectance)
	// The diffuse weight is  (1-metallic) * (1-fresnel(f0 = c_min_reflectance)) * baseColor

	// Fresnel terms
	float c_min_reflectance = 0.04F;
	vec3  f0 = mix(vec3(c_min_reflectance), vec3(mat.baseColor), mat.metallic);
	vec3  fGlossy = schlickFresnel(f0, vec3(1.0F), VdotH);  // Metallic + specular
	float fDiffuse = schlickFresnel(1.0F - c_min_reflectance, 0.0F, VdotH) * (1.0F - mat.metallic);

	// Specular GGX
	vec3  localH = vec3(dot(mat.T, H), dot(mat.B, H), NdotH);
	float d = hvd_ggx_eval(1.0f / mat.roughness, localH);
	vec3  localK1 = vec3(dot(mat.T, data.k1), dot(mat.B, data.k1), NdotV);
	vec3  localK2 = vec3(dot(mat.T, data.k2), dot(mat.B, data.k2), NdotL);
	float G1 = 0.0f, G2 = 0.0f;
	ggx_smith_shadow_mask(G1, G2, localK1, localK2, mat.roughness);

	float diffusePdf = M_1_PI * NdotL;
	float specularPdf = G1 * d * 0.25f / (NdotV * NdotH);
	data.pdf = mix(diffusePdf, specularPdf, bsdfSimpleGlossyProbability(NdotV, mat.metallic));

	data.bsdf_diffuse = vec3(mat.baseColor) * fDiffuse * diffusePdf;  // Lambertian
	data.bsdf_glossy = fGlossy * G2 * specularPdf;             // GGX-Smith
}

void bsdfSampleSimple(INOUT_TYPE(BsdfSampleData) data, gpuMaterial mat)
{
	vec3 tint = vec3(mat.baseColor);
	data.bsdf_over_pdf = vec3(0.0F);

	float nk1 = clampedDot(mat.N, data.k1);
	if (data.xi.z <= bsdfSimpleGlossyProbability(nk1, mat.metallic))
	{
		// Glossy GGX
		data.event_type = BSDF_EVENT_GLOSSY_REFLECTION;
		// Transform to local space
		vec3 localK1 = vec3(dot(mat.T, data.k1), dot(mat.B, data.k1), nk1);
		vec3 halfVector = hvd_ggx_sample_vndf(localK1, mat.roughness, vec2(data.xi));
		// Transform from local space
		halfVector = mat.T * halfVector.x + mat.B * halfVector.y + mat.N * halfVector.z;
		data.k2 = reflect(-data.k1, halfVector);
	}
	else
	{
		// Diffuse
		data.event_type = BSDF_EVENT_DIFFUSE;
		vec3 localDir = cosineSampleHemisphere(data.xi.x, data.xi.y);
		data.k2 = mat.T * localDir.x + mat.B * localDir.y + mat.N * localDir.z;
	}

	BsdfEvaluateData evalData;
	evalData.k1 = data.k1;
	evalData.k2 = data.k2;
	bsdfEvaluateSimple(evalData, mat);
	data.pdf = evalData.pdf;
	vec3 bsdf_total = evalData.bsdf_diffuse + evalData.bsdf_glossy;
	if (data.pdf <= 0.00001F || any(isnan(bsdf_total)))
	{
		data.bsdf_over_pdf = vec3(0.0f);
		data.event_type = BSDF_EVENT_ABSORB;
	}
	else
	{
		data.bsdf_over_pdf = bsdf_total / data.pdf;
	}
}

// Returns the approximate average reflectance of the Simple BSDF -- that is,
// average_over_k2(f(k1, k2)) -- if GGX didn't lose energy.
// This is useful for things like the variance reduction algorithm in
// Tomasz Stachowiak's *Stochastic Screen-Space Reflections*; see also
// Ray-Tracing Gems 1, chapter 32, *Accurate Real-Time Specular Reflections
// with Radiance Caching*.
vec3 bsdfSimpleAverageReflectance(vec3 k1, gpuMaterial mat)
{
	float NdotV = clampedDot(mat.N, k1);
	float c_min_reflectance = 0.04F;
	vec3  f0 = mix(vec3(c_min_reflectance), vec3(mat.baseColor), mat.metallic);
	// This is approximate because
	// average_over_k2(fresnel(f0, 1.0f, VdotH)) != fresnel(f0, 1.0f, NdotV).
	vec3 bsdf_glossy_average = schlickFresnel(f0, vec3(1.0F), NdotV);
	vec3 bsdf_diffuse_average = vec3(mat.baseColor) * schlickFresnel(1.0F - c_min_reflectance, 0.0F, NdotV) * (1.0F - mat.metallic);
	return bsdf_glossy_average + bsdf_diffuse_average;
}

#undef OUT_TYPE
#undef INOUT_TYPE
#undef ARRAY_TYPE

#endif  // NVVKHL_BSDF_FUNCTIONS_H
