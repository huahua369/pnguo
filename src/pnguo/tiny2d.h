#pragma once 
/*
简易2d骨骼动画
*/

// 图集区域
struct texture_region_t {
	void* rendererObject;
	glm::vec2 uv, uv2;
	int degrees;
	glm::vec2 offset;
	glm::ivec2 size;
	glm::ivec2 original_size;
};
enum AttachmentType_e {
	ATTACHMENT_REGION,
	ATTACHMENT_BOUNDING_BOX,
	ATTACHMENT_MESH,
	ATTACHMENT_LINKED_MESH,
	ATTACHMENT_PATH,
	ATTACHMENT_POINT,
	ATTACHMENT_CLIPPING
};

struct attachment_t
{
	char* name;
	int type;
};
// 区域附件
struct region_attachment_t {
	attachment_t super;
	char* path;
	float x, y, scaleX, scaleY, rotation, width, height;
	uint32_t color;
	texture_region_t* region;
	glm::vec2 offset[4];
	glm::vec2 uvs[4];
};
// 顶点附件
struct vertex_attachment_t {
	int bonesCount;
	int* bones;
	int verticesCount;
	float* vertices;
	int worldVerticesLength;
	int id;
};
// 网格附件
struct mesh_attachment_t {
	vertex_attachment_t super;
	texture_region_t* region;
	char* path;
	float* regionUVs;
	float* uvs;
	int trianglesCount;
	unsigned short* triangles;
	uint32_t color;
	int hullLength;
	mesh_attachment_t* parentMesh;
	/* Nonessential. */
	int edgesCount;
	unsigned short* edges;
	float width, height;
};
// 骨骼数据
struct bone_data_t
{
	int index;
	char* name;
	bone_data_t* parent;
	float length;		// 骨骼长度. 运行时通常不使用骨骼长度属性
	float x, y, rotation, scaleX, scaleY, shearX, shearY;
	uint32_t color;
	int/*bool*/ visible;
};
struct bone_t
{
	bone_data_t* data;
	glm::mat3 m;
	int/*bool*/ sorted;
	int/*bool*/ active;
};

struct slot_data_t {
	int index;
	char* name;
	bone_data_t* boneData;	
	char* attachmentName;
	uint32_t color;
	uint32_t darkColor;
	int blendMode;
	int/*bool*/ visible;
};
// 槽位
struct slot_t {
	slot_data_t* data;
	bone_t* bone;
	uint32_t color;				// 有骨骼时，颜色受骨骼影响
	uint32_t* darkColor;
	attachment_t* attachment;
	int attachmentState;
	int deformCapacity;
	int deformCount;
	float* deform;

	int sequenceIndex;
};
// 动画数据，影响
struct animation_t
{
	char* name;
	// float x, y, rotation, scaleX, scaleY, shearX, shearY; vec4 color
	float* times = 0;
	float* value = 0;
	glm::vec4* curve = 0;
	int count;	// 关键帧数量
	int type;   // 值类型长度：1:旋转，2:平移缩放\斜切，4:颜色
};
// 皮肤
struct skeleton_t
{
	char* name;
	slot_t** slots;
	attachment_t** attachments; 
	bone_t** bones;
	int slotCount;
	int attachmentCount;
	int bone_count;
};
