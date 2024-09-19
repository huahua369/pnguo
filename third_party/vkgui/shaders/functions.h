// Portions Copyright 2019 Advanced Micro Devices, Inc.All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// textures.glsl needs to be included

const float c_MinReflectance = 0.04;
#ifndef M_PI
const float M_PI = 3.141592653589793F;  // PI
#endif
const float M_TWO_PI = 6.2831853071795F;  // 2*PI
const float M_PI_2 = 1.5707963267948F;  // PI/2
const float M_PI_4 = 0.7853981633974F;  // PI/4
const float M_1_OVER_PI = 0.3183098861837F;  // 1/PI
const float M_2_OVER_PI = 0.6366197723675F;  // 2/PI
const float M_1_PI = 0.3183098861837F;  // 1/PI

#ifndef INFINITE
const float INFINITE = 1e32F;
#endif

struct AngularInfo
{
	float NdotL;                  // cos angle between normal and light direction
	float NdotV;                  // cos angle between normal and view direction
	float NdotH;                  // cos angle between normal and half vector
	float LdotH;                  // cos angle between light direction and half vector

	float VdotH;                  // cos angle between view direction and half vector

	vec3 padding;
};

vec4 getVertexColor()
{
	vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

#ifdef ID_4PS_COLOR_0
	color.rgb = v_Color0.rgb;
#endif

	return color;
}



float getPerceivedBrightness(vec3 vector)
{
	return sqrt(0.299 * vector.r * vector.r + 0.587 * vector.g * vector.g + 0.114 * vector.b * vector.b);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness/examples/convert-between-workflows/js/three.pbrUtilities.js#L34
float solveMetallic(vec3 diffuse, vec3 specular, float oneMinusSpecularStrength) {
	float specularBrightness = getPerceivedBrightness(specular);

	if (specularBrightness < c_MinReflectance) {
		return 0.0;
	}

	float diffuseBrightness = getPerceivedBrightness(diffuse);

	float a = c_MinReflectance;
	float b = diffuseBrightness * oneMinusSpecularStrength / (1.0 - c_MinReflectance) + specularBrightness - 2.0 * c_MinReflectance;
	float c = c_MinReflectance - specularBrightness;
	float D = b * b - 4.0 * a * c;

	return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

AngularInfo getAngularInfo(vec3 pointToLight, vec3 normal, vec3 view)
{
	// Standard one-letter names
	vec3 n = normalize(normal);           // Outward direction of surface point
	vec3 v = normalize(view);             // Direction from surface point to view
	vec3 l = normalize(pointToLight);     // Direction from surface point to light
	vec3 h = normalize(l + v);            // Direction of the vector between l and v
	float c0 = 0.0f, c1 = 1.0f;
	float NdotL = clamp(dot(n, l), c0, c1);
	float NdotV = clamp(dot(n, v), c0, c1);
	float NdotH = clamp(dot(n, h), c0, c1);
	float LdotH = clamp(dot(l, h), c0, c1);
	float VdotH = clamp(dot(v, h), c0, c1);

	return AngularInfo(
		NdotL,
		NdotV,
		NdotH,
		LdotH,
		VdotH,
		vec3(0, 0, 0)
	);
}

