#version 450

// 全屏三角形顶点位置 (齐次坐标) 
const vec4 FullScreenVertsPos[3] = vec4[3]( 
    vec4(-1.0, 1.0, 1.0, 1.0),   // 左上 
    vec4(3.0, 1.0, 1.0, 1.0),    // 右上 (扩展超出屏幕) 
    vec4(-1.0, -3.0, 1.0, 1.0)   // 左下 (扩展超出屏幕) 
); 
 
// 纹理坐标 (匹配扩展的顶点范围) 
const vec2 FullScreenVertsUVs[3] = vec2[3]( 
    vec2(0.0, 0.0),              // 左上UV 
    vec2(2.0, 0.0),              // 右上UV (对应2倍宽度) 
    vec2(0.0, 2.0)               // 左下UV (对应2倍高度) 
);  

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) out struct{ 
    vec2 vTexture;
} vs_out;
// out vec2 inTexCoord;
void main() { 
    // 获取当前顶点ID (GLSL通过gl_VertexID内置变量) 
    int vertexId = gl_VertexIndex % 3;  // 确保索引安全     
    // 设置裁剪空间位置 (对应SV_POSITION) 
    gl_Position = FullScreenVertsPos[vertexId];     
    // 传递纹理坐标 
    vs_out.vTexture = FullScreenVertsUVs[vertexId]; 
} 