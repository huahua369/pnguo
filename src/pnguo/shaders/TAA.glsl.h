#version 420

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_compute_shader : enable

// 宏定义（与原 HLSL 保持一致） 
#define RADIUS      1 
#define GROUP_SIZE  16 
#define TILE_DIM    (2 * RADIUS + GROUP_SIZE) 
layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE, local_size_z = 1) in;  // 工作组大小定义  


// 资源绑定（对应 HLSL 的 [[vk::binding]]，GLSL 中使用 layout(binding = ...)） 
layout(set = 0, binding = 0) uniform sampler2D ColorBuffer;
layout(set = 0, binding = 1) uniform sampler2D DepthBuffer;
layout(set = 0, binding = 2) uniform sampler2D HistoryBuffer;
layout(set = 0, binding = 3) uniform sampler2D VelocityBuffer;
layout(set = 0, binding = 4, rgba16f) writeonly uniform image2D OutputBuffer;  // RWTexture2D 对应 image2D 

// 共享内存（groupshared 对应 shared） 
shared vec3 Tile[TILE_DIM * TILE_DIM];

// 工具函数：获取最近像素的速度（NDC 转 UV 空间） 
vec2 GetClosestVelocity(vec2 uv, vec2 texelSize, out bool isSkyPixel) {
    vec2 velocity = vec2(0.0);
    float closestDepth = 9.9f;

    // 3x3 邻域采样深度，取最近深度对应的速度 
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 st = uv + vec2(x, y) * texelSize;
            float depth = textureLod(DepthBuffer, st, 0.0).x;  // SampleLevel 对应 textureLod 
            if (depth < closestDepth) {
                velocity = textureLod(VelocityBuffer, st, 0.0).xy;
                closestDepth = depth;
            }
        }
    }

    isSkyPixel = (closestDepth == 1.0f);
    return velocity * vec2(0.5f, -0.5f);  // NDC 转 UV 空间（Y 轴翻转） 
}

// 工具函数：Catmull-Rom 历史采样（保留原算法逻辑） 
vec3 SampleHistoryCatmullRom(vec2 uv, vec2 texelSize) {
    vec2 samplePos = uv / texelSize;
    vec2 texPos1 = floor(samplePos - 0.5f) + 0.5f;
    vec2 f = samplePos - texPos1;

    // Catmull-Rom 权重计算 
    vec2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
    vec2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
    vec2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
    vec2 w3 = f * f * (-0.5f + 0.5f * f);

    vec2 w12 = w1 + w2;
    vec2 offset12 = w2 / (w1 + w2);

    // 采样坐标计算 
    vec2 texPos0 = (texPos1 - 1.0f) * texelSize;
    vec2 texPos3 = (texPos1 + 2.0f) * texelSize;
    vec2 texPos12 = (texPos1 + offset12) * texelSize;

    // 9 次采样并加权求和 
    vec3 result = vec3(0.0f);
    result += textureLod(HistoryBuffer, vec2(texPos0.x, texPos0.y), 0.0).xyz * w0.x * w0.y;
    result += textureLod(HistoryBuffer, vec2(texPos12.x, texPos0.y), 0.0).xyz * w12.x * w0.y;
    result += textureLod(HistoryBuffer, vec2(texPos3.x, texPos0.y), 0.0).xyz * w3.x * w0.y;

    result += textureLod(HistoryBuffer, vec2(texPos0.x, texPos12.y), 0.0).xyz * w0.x * w12.y;
    result += textureLod(HistoryBuffer, vec2(texPos12.x, texPos12.y), 0.0).xyz * w12.x * w12.y;
    result += textureLod(HistoryBuffer, vec2(texPos3.x, texPos12.y), 0.0).xyz * w3.x * w12.y;

    result += textureLod(HistoryBuffer, vec2(texPos0.x, texPos3.y), 0.0).xyz * w0.x * w3.y;
    result += textureLod(HistoryBuffer, vec2(texPos12.x, texPos3.y), 0.0).xyz * w12.x * w3.y;
    result += textureLod(HistoryBuffer, vec2(texPos3.x, texPos3.y), 0.0).xyz * w3.x * w3.y;

    return max(result, 0.0f);
}

// 工具函数：Reinhard 色调映射（保留原逻辑） 
vec3 Reinhard(vec3 hdr) {
    return hdr / (hdr + 1.0f);
}

// 工具函数：从共享内存 Tile 读取数据 
vec3 Tap(vec2 pos) {
    return Tile[int(pos.x) + TILE_DIM * int(pos.y)];
}

// 主着色器入口（对应 HLSL 的 main 函数） 
void mainCS() {
    ivec3 dims;
    bool isSkyPixel;

    // 获取纹理尺寸（HLSL 的 GetDimensions 对应 textureSize） 
    dims.xy = textureSize(ColorBuffer, 0);  // 0 为 mip 级别 
    vec2 texelSize = 1.0f / vec2(dims.xy);
    vec2 uv = (gl_GlobalInvocationID.xy + 0.5f) * texelSize;  // gl_GlobalInvocationID 对应 SV_DispatchThreadID 
    vec2 tilePos = gl_LocalInvocationID.xy + RADIUS + 0.5f;  // gl_LocalInvocationID 对应 SV_GroupThreadID 

    // 填充共享内存 Tile（多线程并行写入） 
    uint localIndex = gl_LocalInvocationIndex;  // 对应 SV_GroupIndex 
    if (localIndex < TILE_DIM * TILE_DIM / 4) {
        ivec2 anchor = ivec2(gl_WorkGroupID.xy) * GROUP_SIZE - RADIUS;  // gl_WorkGroupID 对应 SV_GroupID 

        // 计算 4 个采样坐标（原 HLSL 逻辑，分摊到 4 个线程） 
        ivec2 coord1 = anchor + ivec2(localIndex % TILE_DIM, localIndex / TILE_DIM);
        ivec2 coord2 = anchor + ivec2((localIndex + TILE_DIM * TILE_DIM / 4) % TILE_DIM, (localIndex + TILE_DIM * TILE_DIM / 4) / TILE_DIM);
        ivec2 coord3 = anchor + ivec2((localIndex + TILE_DIM * TILE_DIM / 2) % TILE_DIM, (localIndex + TILE_DIM * TILE_DIM / 2) / TILE_DIM);
        ivec2 coord4 = anchor + ivec2((localIndex + TILE_DIM * TILE_DIM * 3 / 4) % TILE_DIM, (localIndex + TILE_DIM * TILE_DIM * 3 / 4) / TILE_DIM);

        // 采样颜色并应用 Reinhard 色调映射 
        vec2 uv1 = (coord1 + 0.5f) * texelSize;
        vec2 uv2 = (coord2 + 0.5f) * texelSize;
        vec2 uv3 = (coord3 + 0.5f) * texelSize;
        vec2 uv4 = (coord4 + 0.5f) * texelSize;

        vec3 color0 = textureLod(ColorBuffer, uv1, 0.0).xyz;
        vec3 color1 = textureLod(ColorBuffer, uv2, 0.0).xyz;
        vec3 color2 = textureLod(ColorBuffer, uv3, 0.0).xyz;
        vec3 color3 = textureLod(ColorBuffer, uv4, 0.0).xyz;

        // 写入共享内存（HLSL 的 Tile[...] 对应 shared 数组） 
        Tile[localIndex] = Reinhard(color0);
        Tile[localIndex + TILE_DIM * TILE_DIM / 4] = Reinhard(color1);
        Tile[localIndex + TILE_DIM * TILE_DIM / 2] = Reinhard(color2);
        Tile[localIndex + TILE_DIM * TILE_DIM * 3 / 4] = Reinhard(color3);
    }

    // 共享内存屏障（对应 GroupMemoryBarrierWithGroupSync） 
    memoryBarrierShared();
    barrier();

    // 边界检查（超出纹理范围则退出） 
    if (any(greaterThanEqual(gl_GlobalInvocationID.xy, dims.xy))) {
        return;
    }

    // 计算邻域颜色的均值和标准差（原 HLSL 逻辑） 
    float wsum = 0.0f;
    vec3 vsum = vec3(0.0f);
    vec3 vsum2 = vec3(0.0f);

    for (float y = -RADIUS; y <= RADIUS; ++y) {
        for (float x = -RADIUS; x <= RADIUS; ++x) {
            vec3 neigh = Tap(tilePos + vec2(x, y));
            float w = exp(-3.0f * (x * x + y * y) / ((RADIUS + 1.0f) * (RADIUS + 1.0f)));
            vsum2 += neigh * neigh * w;
            vsum += neigh * w;
            wsum += w;
        }
    }

    vec3 ex = vsum / wsum;
    vec3 ex2 = vsum2 / wsum;
    vec3 dev = sqrt(max(ex2 - ex * ex, 0.0f));

    // 获取速度并计算历史采样范围 
    vec2 velocity = GetClosestVelocity(uv, texelSize, isSkyPixel);
    float boxSize = mix(0.5f, 2.5f, isSkyPixel ? 0.0f : smoothstep(0.02f, 0.0f, length(velocity)));  // lerp 对应 mix 

    // 历史颜色采样与钳制 
    vec3 nmin = ex - dev * boxSize;
    vec3 nmax = ex + dev * boxSize;
    vec3 history = SampleHistoryCatmullRom(uv - velocity, texelSize);
    vec3 clampedHistory = clamp(history, nmin, nmax);
    vec3 center = Tap(tilePos);
    vec3 result = mix(clampedHistory, center, 1.0f / 16.0f);  // lerp 对应 mix 

    // 输出结果（imageStore 对应 RWTexture2D 写入） 
    imageStore(OutputBuffer, ivec2(gl_GlobalInvocationID.xy), vec4(result, 1.0f));
}
// first 着色器入口（单独的着色器模块或通过宏控制） 
void firstCS()
{
    ivec3 dims;
    bool isSkyPixel;

    dims.xy = textureSize(ColorBuffer, 0);
    vec2 texelSize = 1.0f / vec2(dims.xy);
    vec2 uv = (gl_GlobalInvocationID.xy + 0.5f) * texelSize;
    vec2 tilePos = gl_LocalInvocationID.xy + RADIUS + 0.5f;

    // 填充共享内存（与 main 函数相同逻辑） 
    uint localIndex = gl_LocalInvocationIndex;
    if (localIndex < TILE_DIM * TILE_DIM / 4) {
        ivec2 anchor = ivec2(gl_WorkGroupID.xy) * GROUP_SIZE - RADIUS;  // gl_WorkGroupID 对应 SV_GroupID 

        // 计算 4 个采样坐标（原 HLSL 逻辑，分摊到 4 个线程） 
        ivec2 coord1 = anchor + ivec2(localIndex % TILE_DIM, localIndex / TILE_DIM);
        ivec2 coord2 = anchor + ivec2((localIndex + TILE_DIM * TILE_DIM / 4) % TILE_DIM, (localIndex + TILE_DIM * TILE_DIM / 4) / TILE_DIM);
        ivec2 coord3 = anchor + ivec2((localIndex + TILE_DIM * TILE_DIM / 2) % TILE_DIM, (localIndex + TILE_DIM * TILE_DIM / 2) / TILE_DIM);
        ivec2 coord4 = anchor + ivec2((localIndex + TILE_DIM * TILE_DIM * 3 / 4) % TILE_DIM, (localIndex + TILE_DIM * TILE_DIM * 3 / 4) / TILE_DIM);

        // 采样颜色并应用 Reinhard 色调映射 
        vec2 uv1 = (coord1 + 0.5f) * texelSize;
        vec2 uv2 = (coord2 + 0.5f) * texelSize;
        vec2 uv3 = (coord3 + 0.5f) * texelSize;
        vec2 uv4 = (coord4 + 0.5f) * texelSize;

        vec3 color0 = textureLod(ColorBuffer, uv1, 0.0).xyz;
        vec3 color1 = textureLod(ColorBuffer, uv2, 0.0).xyz;
        vec3 color2 = textureLod(ColorBuffer, uv3, 0.0).xyz;
        vec3 color3 = textureLod(ColorBuffer, uv4, 0.0).xyz;

        // 写入共享内存
        Tile[localIndex] = Reinhard(color0);
        Tile[localIndex + TILE_DIM * TILE_DIM / 4] = Reinhard(color1);
        Tile[localIndex + TILE_DIM * TILE_DIM / 2] = Reinhard(color2);
        Tile[localIndex + TILE_DIM * TILE_DIM * 3 / 4] = Reinhard(color3);
    }

    memoryBarrierShared();
    barrier();

    vec3 center = Tap(tilePos);
    imageStore(OutputBuffer, ivec2(gl_GlobalInvocationID.xy), vec4(center, 1.0f));
}
void main() {
#ifdef FIRST_PASS
	firstCS();
#else
	mainCS();
#endif
}