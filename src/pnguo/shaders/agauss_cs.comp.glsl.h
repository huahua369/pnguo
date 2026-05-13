#version 450
#extension GL_ARB_compute_shader : enable
layout(local_size_x = 16, local_size_y = 16) in; //工作组大小适配16x16线程块

layout(set = 0, binding = 0, rgba8) readonly uniform image2D srcImage; //保留采样器
layout(set = 0, binding = 1, rgba8) readonly uniform image2D targetImage;// 目标底图
layout(set = 0, binding = 2, rgba8) writeonly uniform image2D dstImage; //输出目标

layout(push_constant, std140) uniform uPushConstant {
	vec2 dir;
	int filterMode;
	int glowMode;
	float glowFactor;
} pc;

#define NUMWT 9
float Gauss[NUMWT] = { 0.93, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1 };
#define WT_NORMALIZE (1.0/(1.0+2.0*(0.93 + 0.8 + 0.7 + 0.6 + 0.5 + 0.4 + 0.3 + 0.2 + 0.1)))

vec4 fpGaussianPass(ivec2 tc) {
	ivec2 size = imageSize(srcImage);
	vec4 c = imageLoad(srcImage, tc) * WT_NORMALIZE;
	vec2 dir = pc.dir;
	for (int i = 0; i < 9; i++) {
		c += imageLoad(srcImage, ivec2(tc + dir)) * Gauss[i] * WT_NORMALIZE;
		c += imageLoad(srcImage, ivec2(tc - dir)) * Gauss[i] * WT_NORMALIZE;
		dir += pc.dir;
	}
	if (pc.filterMode == 1) c *= pc.glowFactor;  //垂直模式增强发光
	return c;
}
vec4 fpFinalCompositing(ivec2 tc)
{
	vec4 c = imageLoad(targetImage, tc);
	vec4 sc = imageLoad(srcImage, tc);
	if (pc.glowMode == 1) {             //外发光
		c = 1 - (1 - c) * (1 - sc);     //滤色混合（屏幕）
		c -= sc;                  //去掉中间
	}
	else {                    //内发光
		c *= sc;                  //正片叠底删除多余像素
	}
	return c;
}
void main() {
	ivec2 tc = ivec2(gl_GlobalInvocationID.xy);
	vec4 result = (pc.filterMode < 2)
		? fpGaussianPass(tc)
		: fpFinalCompositing(tc);
	imageStore(dstImage, tc, result); //写入计算结果
}