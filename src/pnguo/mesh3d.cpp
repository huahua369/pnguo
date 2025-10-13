
/*
pbr材质：金属度metalness、粗糙度roughness、清漆.clearcoat、透光率(透射度).transmission
KHR_materials_ior ：折射率描述了光在穿过对象时是如何散射的。 通过使艺术家能够控制 IOR 值，
						可以使各种透明材料看起来更逼真，包括空气、水、眼睛、玻璃、蓝宝石和钻石。
KHR_materials_volume ：体积扩展使网格表面能够充当体积之间的界面，并实现更逼真的折射和吸收特性，如透明材料中所见。
							这种延伸使半透明材料具有深度和重量的外观。 对于无法进行光线追踪的实时引擎，此扩展还提供了一个厚度纹理贴图，以实现光与大量材料相互作用的快速近似。
KHR_materials_specular ：镜面属性是一个对象的类似镜子的属性：它有规律地反射光线的能力，创建其他对象的相干反射。
						与其前身 KHR_materials_pbrSpecularGlossiness 不同，这个新的镜面反射扩展在 glTF 的 PBR 材料模型核心的现代金属/粗糙度工作流程中运行，使彩色镜面高光与高级 PBR 材料扩展阵列兼容。
*/

#include "pch1.h" 
#include "mesh3d.h"

#include <noise/noise.h>

#include "print_time.h"

#include <stb_image.h>
#include <stb_image_write.h>


// 地形类型枚举 
typedef enum {
	PLAIN,      // 平原 (高度0-50)
	HILL,       // 丘陵 (高度51-150)
	MOUNTAIN,   // 山脉 (高度151-200)
	RIVER       // 河流 (特殊标记)
} TerrainType;
// 分类地形类型 
TerrainType classifyTerrain(float height) {
	if (height < 0)
		return RIVER;	// 河流
	if (height < 50) return PLAIN;	// 平原
	if (height < 150) return HILL;	// 丘陵
	return MOUNTAIN;				// 山脉
}
// 平滑函数
double fade(double t) {
	return t * t * t * (t * (t * 6 - 15) + 10);
}

// 梯度函数
double grad(int hash, double x, double y, double z) {
	int h = hash & 15;
	double u = h < 8 ? x : y;
	double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// 线性插值
double lerp(double t, double a, double b) {
	return a + t * (b - a);
}
// 计算柏林噪声
double perlinNoise(double x, double y, double z, int* p) {
	int X = (int)x & 255;
	int Y = (int)y & 255;
	int Z = (int)z & 255;

	x -= (int)x;
	y -= (int)y;
	z -= (int)z;

	double u = fade(x);
	double v = fade(y);
	double w = fade(z);

	int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
	int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

	return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)),
		lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))),
		lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1)),
			lerp(u, grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1))));
}

float fractal_noise1(float x, float y, float z, int* p) {
	float total = 0.0;
	float frequency = 1.0;
	float amplitude = 1.0;
	float max_val = 0.0;

	//for (int i = 0; i < OCTAVES; i++) {
	//	total += perlinNoise(x * frequency, y * frequency, z, p) * amplitude;
	//	max_val += amplitude;
	//	amplitude *= PERSISTENCE;
	//	frequency *= 2;
	//}
	return total / max_val; // 归一化
}
// 哈希函数生成固定随机值（基于坐标）
float noise_(int x, int y) {
	unsigned int n = x * 157 + y * 113;
	n = (n << 13) ^ n;
	n = (n * (n * n * 15731 + 789221) + 1376312589);
	return (float)(n & 0x7FFFFFFF) / 0x7FFFFFFF; // 归一化到[0,1]
}

// 双线性插值平滑 
float interpolate(float a, float b, float t) {
	return a + t * (b - a);
}

// 应用高斯平滑（使地形更自然）
void applySmoothing(float* heightMap, int width, int height) {
	float kernel[3][3] = {
		{1 / 16.0, 2 / 16.0, 1 / 16.0},
		{2 / 16.0, 4 / 16.0, 2 / 16.0},
		{1 / 16.0, 2 / 16.0, 1 / 16.0}
	};
	std::vector<float> tt;
	tt.resize(width * height);
	float* temp = tt.data();

	for (int x = 1; x < width - 1; x++) {
		for (int y = 1; y < height - 1; y++) {
			float sum = 0;
			for (int i = -1; i <= 1; i++) {
				for (int j = -1; j <= 1; j++) {
					sum += heightMap[x + i + (y + j) * width] * kernel[i + 1][j + 1];
				}
			}
			temp[x + y * width] = sum;
		}
	}

	// 复制回原数组 
	for (int x = 1; x < width - 1; x++) {
		for (int y = 1; y < height - 1; y++) {
			heightMap[x + y * width] = temp[x + y * width];
		}
	}
}
int generateRivers(int width, int height, std::vector<float>* pheight_map, std::vector<int>* priver_path)
{
	auto height_map = pheight_map->data();
	int* river_path = priver_path->data(); // 河流路径标记

	// 生成河流（从随机高山点开始）
	int start_x = -1, start_y = -1;
	for (int attempt = 0; attempt < 100; attempt++) { // 最多尝试100次找源头 
		int x = rand() % width;
		int y = rand() % height;
		if (height_map[x + y * width] > 100) {
			start_x = x;
			start_y = y;
			break;
		}
	}
	int count = 0;
	if (start_x != -1) { // 找到有效源头
		int x = start_x, y = start_y;
		for (int step = 0; step < 1000; step++) { // 最多1000步 
			river_path[x + y * width] = 1; // 标记河流路径
			float min_height = height_map[x + y * width];
			int next_x = x, next_y = y;
			count++;
			// 检查8邻域最低点 
			for (int dy = -1; dy <= 1; dy++) {
				for (int dx = -1; dx <= 1; dx++) {
					if (dx == 0 && dy == 0) continue;
					int nx = x + dx, ny = y + dy;
					if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
						if (height_map[nx + ny * width] < min_height && !river_path[nx + ny * width]) {
							min_height = height_map[nx + ny * width];
							next_x = nx;
							next_y = ny;
						}
					}
				}
			}
			if (next_x == x && next_y == y)
				break; // 无更低点，停止流动 
			x = next_x;
			y = next_y;
		}
	}
	return 0;
}


// 导出高程图为PGM文件（可视化）
void exportHeightMap(float* heightMap, int width, int height, const char* filename) {
	std::vector<unsigned char> bit;// [WIDTH * HEIGHT] = {};
	bit.resize(width * height);
	auto t = bit.data();
	int mx = 0, mi = 100000;
	int f = 0;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			auto v = heightMap[x + y * width];
			mi = fmin(v, mi);
			mx = fmax(v, mx);
			if (v < 0)
			{
				f++;
			}
			unsigned char value = v;
			*t = value; t++;
		}
	}
	stbi_write_png(filename, width, height, 1, bit.data(), width);
	return;
}

// 打印地形统计信息 
void printTerrainStats(float* heightMap, int width, int height) {
	int counts[4] = { 0 }; // PLAIN, HILL, MOUNTAIN, RIVER 

	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			TerrainType type = classifyTerrain(heightMap[x + y * width]);
			counts[type]++;
		}
	}

	printf((char*)u8"\n地形统计:\n");
	printf((char*)u8"平原占比: %.2f%%\n", counts[PLAIN] * 100.0 / (width * height));
	printf((char*)u8"丘陵占比: %.2f%%\n", counts[HILL] * 100.0 / (width * height));
	printf((char*)u8"山脉占比: %.2f%%\n", counts[MOUNTAIN] * 100.0 / (width * height));
	printf((char*)u8"河流占比: %.2f%%\n", counts[RIVER] * 100.0 / (width * height));
}

int main_heightmap(uint32_t seed)
{
	print_time aa((char*)u8"地图种子:" + std::to_string(seed));

	{
		/*
		参数项					山地		平原					对地形影响说明
		噪声幅度					1.0		0.3 ~ 0.5			决定整体起伏强度，越小越平坦
		八度数（octaves）		6 ~ 8	2 ~ 3				控制细节层次，越少越平滑
		持久度（persistence）	0.5		0.3 ~ 0.4			高频成分衰减更快，减少小起伏
		高度映射范围				0 ~ 1	0 ~ 0.4				人为压缩最大高程，突出低地
		后处理函数				线性		幂函数（如 x².⁵）	使中间值更集中，两端拉伸少
		*/
		// 1. 配置噪声模块（生成平原占比高的地形）
   // ---------------------- 
		noise::module::Perlin baseNoise;  // 基础噪声（Perlin噪声，平缓起伏）
		baseNoise.SetFrequency(0.0128);      // 频率（越小，起伏越平缓）
		baseNoise.SetOctaveCount(2);       // 八度（越少，细节越少）
		baseNoise.SetPersistence(0.3);     // 持续性（越小，高八度影响越小）
		baseNoise.SetLacunarity(2.0);      //  Lacunarity（频率倍增系数，默认2.0） 
		baseNoise.SetSeed(seed);
		// 可选：添加细节噪声（小幅度起伏，增强真实感） 
		noise::module::Perlin detailNoise;
		detailNoise.SetFrequency(0.8);     // 高频率（细节）
		detailNoise.SetOctaveCount(1);     // 1层细节 
		noise::module::Multiply detailMod;        // 细节噪声权重（0.1，不影响整体起伏）
		detailMod.SetSourceModule(0, detailNoise);
		noise::module::Const ct;
		ct.SetConstValue(0.1);
		detailMod.SetSourceModule(1, ct);

		// 组合基础噪声和细节噪声（基础占90%，细节占10%）
		noise::module::Add finalNoise;
		finalNoise.SetSourceModule(0, baseNoise);
		finalNoise.SetSourceModule(1, detailMod);

		// 调整噪声范围到[0, 255]（高程图像素值） 
		noise::module::ScaleBias scaleBias;
		scaleBias.SetSourceModule(0, finalNoise);
		scaleBias.SetScale(127.5);  // (-1到1) * 127.5 → -127.5到127.5
		scaleBias.SetBias(127.5 - 50);   // +127.5 → 0到255 

		// ---------------------- 
		// 2. 生成高程图数据（2D） 
		// ---------------------- 
		const int width = 50;  // 高程图宽度（像素）
		const int height = 50; // 高程图高度（像素）
		std::vector<float> heightMap(width * height);  // 存储每个像素的高度（0-255）
		std::vector<unsigned char> heightMap1(width * height);  // 存储每个像素的高度（0-255）

		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				// 将像素坐标映射到噪声空间（扩大范围，让起伏更平缓）
				double nx = (double)x / width * 200.0;  // x范围：0→200 
				double ny = (double)y / height * 200.0; // y范围：0→200 
				double nz = 0.0;                        // 2D高程图，z=0

				// 获取噪声值（已映射到0-255）
				double noiseValue = scaleBias.GetValue(nx, ny, nz);
				// 确保值在0-255之间（防止溢出）
				unsigned char pixelValue = static_cast<unsigned char>(std::clamp(noiseValue, 0.0, 255.0));
				heightMap[y * width + x] = pixelValue;
			}
		}
		applySmoothing(heightMap.data(), width, height);
		std::vector<int> river_path;
		river_path.resize(width * height);
		generateRivers(width, height, &heightMap, &river_path);
		for (auto i = 0; i < width * height; i++)
		{
			heightMap1[i] = (unsigned char)heightMap[i];
		}
		int river_paths[50][50];
		for (size_t i = 0; i < height; i++)
		{
			auto& it = river_paths[i];
			for (size_t j = 0; j < width; j++)
			{
				it[j] = river_path[j + i * width];
			}
		}
		printTerrainStats(heightMap.data(), width, height);
		// ---------------------- 
		// 3. 保存高程图（PNG可视化 + RAW导入3D引擎）
		// ---------------------- 
		// 保存为PNG（用图片查看器打开，灰度越浅表示海拔越高）
		stbi_write_png("heightmap_plains.png", width, height, 1, heightMap1.data(), width);
		//std::cout << "高程图生成成功！\n- PNG路径：heightmap_plains.png\n-  RAW路径：heightmap_plains.raw" << std::endl;
	}
	return 0;
}

