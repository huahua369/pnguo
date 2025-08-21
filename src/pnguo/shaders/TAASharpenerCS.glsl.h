#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_compute_shader : enable

layout(local_size_x = 8, local_size_y = 8) in;
layout(set = 0, binding = 0, rgba32f) uniform image2D TAABuffer;
layout(set = 0, binding = 1, rgba32f) uniform image2D HDR;
layout(set = 0, binding = 2, rgba32f) uniform image2D History;
//--------------------------------------------------------------------------------------
// Helper functions
//--------------------------------------------------------------------------------------

vec3 RGBToYCoCg(vec3 rgb)
{
	return vec3(
		0.25f * rgb.r + 0.5f * rgb.g + 0.25f * rgb.b,
		0.5f * rgb.r - 0.5f * rgb.b,
		-0.25f * rgb.r + 0.5f * rgb.g - 0.25f * rgb.b);
}

vec3 YCoCgToRGB(vec3 yCoCg)
{
	return vec3(
		yCoCg.x + yCoCg.y - yCoCg.z,
		yCoCg.x + yCoCg.z,
		yCoCg.x - yCoCg.y - yCoCg.z);
}

vec3 ApplySharpening(vec3 center, vec3 top, vec3 left, vec3 right, vec3 bottom)
{
	vec3 result = RGBToYCoCg(center);
	float unsharpenMask = 4.0f * result.x;
	unsharpenMask -= RGBToYCoCg(top).x;
	unsharpenMask -= RGBToYCoCg(bottom).x;
	unsharpenMask -= RGBToYCoCg(left).x;
	unsharpenMask -= RGBToYCoCg(right).x;
	result.x = min(result.x + 0.25f * unsharpenMask, 1.1f * result.x);
	return YCoCgToRGB(result);
}

vec3 ReinhardInverse(vec3 sdr)
{
	return sdr / max(1.0f - sdr, 1e-5f);
}

//--------------------------------------------------------------------------------------
// Main function
//-------------------------------------------------------------------------------------- 
void mainCS()
{
	ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
	const vec3 center = imageLoad(TAABuffer, coords).xyz;
	const vec3 top = imageLoad(TAABuffer, coords + ivec2(0, 1)).xyz;
	const vec3 left = imageLoad(TAABuffer, coords + ivec2(1, 0)).xyz;
	const vec3 right = imageLoad(TAABuffer, coords + ivec2(-1, 0)).xyz;
	const vec3 bottom = imageLoad(TAABuffer, coords + ivec2(0, -1)).xyz;
	const vec3 color = ApplySharpening(center, top, left, right, bottom);
	imageStore(HDR, coords, vec4(ReinhardInverse(center), 1.0f));
	imageStore(History, coords, vec4(center, 1.0f));
}

void postCS()
{
	ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
	const vec3 center = imageLoad(TAABuffer, coords).xyz;
	imageStore(HDR, coords, vec4(ReinhardInverse(center), 1.0f));
	imageStore(History, coords, vec4(center, 1.0f));
}
