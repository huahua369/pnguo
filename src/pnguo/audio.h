// 音频
#ifndef AUDIO_H_V1_

/*
音频
*/

/*编码信息*/
struct encoder_info_t
{
	// coder_t->load返回的句柄
	void* handle;
	int sample_rate;
	char channels;
	char bits_per_sample;
	// 压缩层 flac 0-8 ，小于0默认6，大
	char compression_level;
	char unknown_;
	int blocksize;		//4096
	// 总样本
	uint64_t total_samples;
	// read_func/write_func用到，可以空
	void* userdata;
	// data和read_func二选一，优先data
	const char* data;
	int data_size;
	// 读数据回调，返回nullptr退出输入，int*len返回长度
	const char* (*read_func)(int* len, void* userdata);

	// 文件名路径file_path和write_func二选一
	const char* file_path;
	const char* file_src;
	// 输出数据回调
	int(*write_func)(const char* data, size_t bytes, uint32_t samples, uint32_t current_frame, void* userdata);
};
typedef size_t(*encoder_func)(encoder_info_t* p);
// 音乐解码API
struct coder_t
{
	const char* tag;
	char tag2[8] = {};
	void* handle;
	// Load the library 可选
	void* (*load)(void);
	void (*unload)(void* handle);

	// 创建返回return void* music
	void* (*open_file)(const char* file, void* handle);				// 可选
	void* (*open_memory)(const void* data, int len, void* handle);	// *

	// 设置音量 
	void (*set_volume)(void* music, int volume);

	// 获取音乐数据，返回剩余的字节数
	int (*get_audio)(void* music, void* data, int bytes); //	*
	// 获取浮点数据
	int (*get_audio_float)(void* music, float* data, int n);

	// 定位播放位置 (单位秒 seconds)
	int (*seek)(void* music, double position); 

	// 释放音频对象
	void (*free_m)(void* music); //	*
	// 获取音频对象信息
	void (*get_info)(void* music, int* total_samples, int* channels, int* sample_rate, int* bits_per_sample);

	// 编码器（可选）,返回0则跳出编码
	encoder_func encoder;

};

struct audio_data_t
{
	int format = 0, channels = 0, freq = 0;
	int len = 0;		// 总长度，解码完成前等于0
	void* data = 0;		// 解码数据
	void* ptr = 0; 
	char* dataold = 0;
	coder_t* code = 0;
	size_t desize = 0;	// 当前解码字节数
	void* mf = 0;
	int cap = 0;
	int total_samples = 0;
};
class coders_t
{
public:
	std::vector<coder_t*> codes;
public:
	coders_t();
	~coders_t();

private:

};


coders_t* new_coders();
void free_coders(coders_t* p);
audio_data_t* new_audio_data(coders_t* pc, const std::string& fn);
// 输入数据data，iscopy是否复制数据到新内存
audio_data_t* new_audio_data(coders_t* pc, const char* data, int len, bool iscopy);
int decoder_data(audio_data_t* p);
void free_audio_data(audio_data_t* p);


#endif // !AUDIO_H_V1_
