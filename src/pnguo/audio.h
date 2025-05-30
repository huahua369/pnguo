﻿// 音频
#ifndef AUDIO_H_V1_

/*
音频

	open_audio
		new_audio_stream创建流
		decoder_data解码数据（支持flac、ogg、mp3）
		put_audio推送数据
		free_audio_stream
	close_audio
*/

/*编码信息*/
struct encoder_info_t
{
	// coder_t->load返回的句柄
	void* handle = 0;
	int sample_rate = 0;
	char channels = 2;
	char bits_per_sample = 0;	// 支持数据位宽16 24 32 64。浮点数32/64，整数24就是int32
	// 压缩层 flac 0-8 ，小于0默认6，大
	char compression_level = 6;
	char src_format = 0;	// 源格式，0整数，1浮点数
	int blocksize = 4096;		//4096
	// 总样本
	uint64_t total_samples = 0;
	const char* data = 0;	// pcm数据
	int data_size = 0;
	int dst_format = 0;		// 目标格式1=s8、2=s16、3=s24。如果0则根据源格式自动匹配

	// 文件名路径 
	const char* file_path = 0;
};
typedef size_t(*encoder_func)(encoder_info_t* p);

struct audio_spec_t
{
	int format;
	int channels;
	int sample_rate;// freq;
	int total_samples;
	int bits_per_sample;
};

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
	void (*get_info)(void* music, audio_spec_t* p);

	// 编码器（可选）,返回0则跳出编码
	encoder_func encoder;

};
/*format int16=0,int=1,float=2,double=3*/
struct audio_data_t
{
	int format = 0, channels = 0, freq = 0;
	int sample_rate;
	int bits_per_sample = 0;
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

struct audio_item
{
	std::string name, path;		// 歌名、路径
	audio_data_t* data = 0;
	void* st = 0;
	size_t cpos = 0;	// 当前播放位置
	double ctime = 0.0;	// 当前播放时间
	double atime = 0.0;	// 总播放时间

};

coders_t* new_coders();
void free_coders(coders_t* p);
audio_data_t* new_audio_data(coders_t* pc, const std::string& fn);
// 输入数据data，iscopy是否复制数据到新内存
audio_data_t* new_audio_data(coders_t* pc, const char* data, int len, bool iscopy);
int decoder_data(audio_data_t* p);
void free_audio_data(audio_data_t* p);



#if 0
void testaudioencoder()
{
	encoder_info_t e = {};
	while (1)
	{
		int rc1 = decoder_data(mad2);
		if (rc1 <= 0)
		{
			break;
		}
	}
	int bits[] = { 16,24,32 };
	e.bits_per_sample = bits[mad2->format];
	e.src_format = mad2->format == 2 ? 1 : 0;
	e.channels = mad2->channels;
	e.sample_rate = mad2->freq;
	e.total_samples = mad2->total_samples;
	e.data = (char*)mad2->data;
	e.data_size = mad2->len;
	e.file_path = R"(E:\song2\abc.flac)";
	auto pe = cp->codes[0];
	e.handle = pe->handle;
	int ret = pe->encoder(&e);
}
#endif // 0

namespace hz {
	class fft_cx
	{
	public:
		double* real = 0;
		void* _complex = 0;
		int count = 0;
		float sample_rate = 0.0f; float freq_start = 0.0f; float freq_end = 0.0f;
		int FRAME_SIZE = 0;
		int bits_per_sample = 0;
		int draw_height = 100;
		float bar_width = 6;
		float bar_step = 4;
		int taps = 64;
		float smoothConstantDown = 0.08;
		float smoothConstantUp = 0.8;
		bool is_smooth = true;			// 是否平滑
		bool is_raw = false;
		glm::vec2 draw_pos = { 60,160 };
		std::vector<float> outdata;
		std::vector<float> heights;
		std::vector<float> _lastY[2];
		std::vector<float> _oy;
		std::vector<double> magnitudesv;
		std::vector<float> vd, vd2;
		std::vector<float> sample_tem;
		std::vector<glm::vec4> _rects;
	public:
		fft_cx();
		~fft_cx();
		void init(float sample_rate, int bits, float freq_start, float freq_end);
		float* calculate_heights(float* audio_frame, int frame_size, int dcount);
		float* calculate_heights(short* audio_frame, int frame_size, int dcount);
	private:
		float* fft(float* data, int n);
		void calculate_heights(std::vector<float>& dh, int dcount, std::vector<float>& oy, std::vector<float>& lastY, glm::vec4* rects, int x, bool is_reverse);

	};
	struct audio_backend_t
	{
		uint32_t dev = 0;
		// 创建流
		void* (*new_audio_stream)(uint32_t dev, int format, int channels, int freq) = 0;
		void (*free_audio_stream)(void* st) = 0;
		// 绑定/解绑到播放设备
		void (*bindaudio)(uint32_t dev, void* st) = 0;
		void (*unbindaudio)(void* st) = 0;
		void (*unbindaudios)(void** st, int count) = 0;
		// 获取流st的剩余播放量
		int (*get_audio_stream_queued)(void* st) = 0;
		int (*get_audio_stream_available)(void* st) = 0;
		int (*get_audio_dst_framesize)(void* st) = 0;
		// 推送音频数据
		void (*put_audio)(void* stream, void* data, int len) = 0;
		// v=0播放、1暂停
		void (*pause_audio)(void* st_, int v) = 0;
		// format：0=S16 , 1=S32 ,2=F32，volume取0-1
		bool (*mix_audio)(uint8_t* dst, uint8_t* src, int format, size_t len, float volume) = 0;
		void (*clear_audio)(void* st) = 0;
		void(*sleep_ms)(int ms) = 0;
		// 返回当前毫秒用
		uint64_t(*get_ticks)() = 0;
	};
	struct audio_list
	{
		std::string name;			// 歌单名
		std::vector<audio_item*> v;
	};
	/*
	音频管理类
		todo 按时间轴推送音频、异步解码音频
	*/
	class audio_cx
	{
	public:
		// 当前播放时间/进度
		double ct = 0.0;
		// 解码器
		coders_t* coders = 0;
		fft_cx* fft = 0;
		// 设置播放后端
		audio_backend_t bk = {};
		// 歌单列表
		std::vector<audio_list*> _lists;
		std::queue<audio_item*> _de_list;
		std::vector<int> _playlist;		// 播放列表
		size_t gd_idx = -1;					// 当前播放的歌单
		size_t gd_sidx = -1;				// 当前播放的歌单索引
		int ge_type = 0;				// 0单曲播放，1单曲循环，2顺序播放，3循环播放，4随机播放
		std::mutex _gd_mutex;
		std::mutex _de_mutex;
		audio_item* _current = 0;	// 当前播放的音频
		// 流
		std::map<glm::ivec3, void*> _streams;
		//
		std::vector<char> tem_buf;
		// 配置信息	{自定义设置、歌单配置等}
		njson config;
		std::string _confn;
		// -1则保存成cbor
		int indent_cbor = 0;
		int waitms = 20;		// 延迟毫秒
		double prev_time = 0.0;
		int64_t cti = 0;
		int frame_size = 0;
		std::jthread put_jt, de_jt;
		bool _run = true;
	public:
		audio_cx();
		~audio_cx();
		void init(audio_backend_t* p, const std::string& confn);
		void save_config();

		void add_song(size_t idx, const std::string& uri);
		// 设置播放歌单
		void set_gd(size_t idx);
		// 设置播放类型
		void set_type(int type = 4);
		audio_list* get_list_it(size_t i);
		// 播放当前歌单指定索引
		void play(size_t idx);
		// 创建线程运行
		void run_thread();
		void push_decoder(audio_item* p);
	private:
		void play(size_t idx, size_t i);
		void play_thr();
	};


	// 常用函数
	void s2flac8_array(const short* src, int32_t* dest, int count);
	void s2flac16_array(const short* src, int32_t* dest, int count);
	void s2flac24_array(const short* src, int32_t* dest, int count);
	void i2flac8_array(const int* src, int32_t* dest, int count);
	void i2flac16_array(const int* src, int32_t* dest, int count);
	void i2flac24_array(const int* src, int32_t* dest, int count);
	void f2flac8_clip_array(const float* src, int32_t* dest, int count, int normalize);
	void f2flac16_clip_array(const float* src, int32_t* dest, int count, int normalize);
	void f2flac24_clip_array(const float* src, int32_t* dest, int count, int normalize);
	void f2flac8_array(const float* src, int32_t* dest, int count, int normalize);
	void f2flac16_array(const float* src, int32_t* dest, int count, int normalize);
	void f2flac24_array(const float* src, int32_t* dest, int count, int normalize);
	void d2flac8_clip_array(const double* src, int32_t* dest, int count, int normalize);
	void d2flac16_clip_array(const double* src, int32_t* dest, int count, int normalize);
	void d2flac24_clip_array(const double* src, int32_t* dest, int count, int normalize);
	void d2flac8_array(const double* src, int32_t* dest, int count, int normalize);
	void d2flac16_array(const double* src, int32_t* dest, int count, int normalize);
	void d2flac24_array(const double* src, int32_t* dest, int count, int normalize);
	//inline int psf_lrintf(float x);
	//inline int psf_lrint(double x);
}
//!hz

#endif // !AUDIO_H_V1_
