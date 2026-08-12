struct VSInput
{
	float3 pos;
	float2 uv;
	float4 color;
#ifdef DOUBLESIDEDCOLOR
	float4 color1;
#endif
};

struct VSOutput
{
	float4 pos : SV_Position;
	float2 uv;
	float4 color;
#ifdef DOUBLESIDEDCOLOR
	float4 color1;
#endif
};

struct PushConsts
{
	float4x4 mvp;
	float masktime;
};
[[vk::push_constant]] PushConsts pc;
#ifdef ID_INSTANCING
StructuredBuffer<float4x4> instance_model_matrix;
[shader("vertex")]
VSOutput main(VSInput input, uint InstanceIndex : SV_InstanceID)
{
	VSOutput output;
	float4x4 m = mul(pc.mvp, instance_model_matrix[InstanceIndex]);
	output.pos = mul(m, float4(input.pos, 1.0));
	output.uv = input.uv;
	output.color = input.color;
#ifdef DOUBLESIDEDCOLOR
	output.color1 = input.color1;
#endif
	return output;
}
#else
[shader("vertex")]
VSOutput main(VSInput input)
{
	VSOutput output;
	output.pos = mul(pc.mvp, float4(input.pos, 1.0));
	output.uv = input.uv;
	output.color = input.color;
#ifdef DOUBLESIDEDCOLOR
	output.color1 = input.color1;
#endif
	return output;
}
#endif

Sampler2D samplerColor;
#ifdef COLOR_MASK
Sampler2D samplerMask;
#endif
[shader("fragment")]
float4 fragMain(VSOutput input, bool FrontFacing : SV_IsFrontFace) : SV_TARGET
{
	float4 color = input.color;
#ifdef DOUBLESIDEDCOLOR
	color = FrontFacing ? input.color : input.color1;
#endif
	color *= samplerColor.Sample(input.uv);
#ifdef COLOR_MASK
	color *= 1.0 - step(pc.masktime, samplerMask.Sample(input.uv).r);
#endif
#ifndef NOT_PREMULT_ALPHA
	color.rgb *= color.a;
#endif
	return color;
}