
//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------
[[vk::binding(0)]] cbuffer cbPerFrame : register(b0)
{
    matrix u_mClipToWord;
}

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------
struct VERTEX
{
    [[vk::location(0)]] float2 vTexcoord : TEXCOORD; 
};

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
[[vk::binding(1)]] TextureCube      inputTex         :register(t0);
[[vk::binding(1)]] SamplerState     samLinearWrap    :register(s0);

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------
[[vk::location(0)]] float4 main(VERTEX Input) : SV_Target 
{
    float4 clip = float4(2 * Input.vTexcoord.x - 1, 1 - 2 * Input.vTexcoord.y, 1, 1);

    float4 pixelDir = mul(u_mClipToWord, clip);

    return inputTex.Sample(samLinearWrap, pixelDir.xyz)*.02;
}
