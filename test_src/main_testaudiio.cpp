/*
libsoundio示例

set(BUILD_STATIC_LIBS ON)
add_definitions(-DSOUNDIO_STATIC_LIBRARY=1)
set(ENABLE_WASAPI ON)
set(BUILD_DYNAMIC_LIBS OFF)
add_subdirectory(third_lib/libsoundio)

*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <cmath>
#include <vector>
#ifdef  __cplusplus
extern "C" {
#endif
#include <soundio/soundio.h> 
#ifdef  __cplusplus
}
#endif


static const float nPI = 3.1415926535f;
static float seconds_offset = 0.0f;
static void write_callback(struct SoundIoOutStream* outstream,
	int frame_count_min, int frame_count_max)
{
	const struct SoundIoChannelLayout* layout = &outstream->layout;
	float float_sample_rate = outstream->sample_rate;
	float seconds_per_frame = 1.0f / float_sample_rate;
	struct SoundIoChannelArea* areas;
	int frames_left = frame_count_max;
	int err;

	while (frames_left > 0) {
		int frame_count = frames_left;

		if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
			fprintf(stderr, "%s\n", soundio_strerror(err));
			exit(1);
		}

		if (!frame_count)
			break;
		float pitch = 440.0f;
		float radians_per_second = pitch * 2.0f * nPI;
#if 0

		for (int frame = 0; frame < frame_count; frame += 1) {
			float sample = sinf((seconds_offset + frame * seconds_per_frame) * radians_per_second);
			for (int channel = 0; channel < layout->channel_count; channel += 1) {
				float* ptr = (float*)(areas[channel].ptr + areas[channel].step * frame);
				*ptr = sample;
				//auto si32 = (int16_t*)ptr;
				//*si32 = sample * 32767.0;
				auto s = (int32_t*)ptr;
				*s = sample * 2147483647;
			}
		}
#else

		double a = ((int64_t)INT_MAX - (int64_t)INT_MIN) * 0.5;
		double b = ((int64_t)INT_MAX + (int64_t)INT_MIN) * 0.5;
		for (int frame = 0; frame < frame_count; frame += 1) {
			float sample = sinf((seconds_offset + frame * seconds_per_frame) * radians_per_second);
			for (int channel = 0; channel < layout->channel_count; channel += 1) {
				int* ptr = (int*)(areas[channel].ptr + areas[channel].step * frame);
				*ptr = round(a * sample + b);
			}
		}
#endif // 0

		seconds_offset = fmodf(seconds_offset +
			seconds_per_frame * frame_count, 1.0f);

		if ((err = soundio_outstream_end_write(outstream))) {
			fprintf(stderr, "%s\n", soundio_strerror(err));
			exit(1);
		}

		frames_left -= frame_count;
	}
}
int test_create_outstream(void) {
	int err;
	struct SoundIo* soundio = soundio_create();
	if (!soundio) {
		fprintf(stderr, "out of memory\n");
		return 1;
	}
	if ((err = soundio_connect(soundio))) {
		fprintf(stderr, "error connecting: %s", soundio_strerror(err));
		return 1;
	}
	soundio_flush_events(soundio);
	int default_out_device_index = soundio_default_output_device_index(soundio);
	if (default_out_device_index < 0) {
		fprintf(stderr, "no output device found");
		return 1;
	}

	struct SoundIoDevice* device = soundio_get_output_device(soundio, default_out_device_index);
	if (!device) {
		fprintf(stderr, "out of memory");
		return 1;
	}

	fprintf(stderr, "Output device: %s\n", device->name);

	struct SoundIoOutStream* outstream = soundio_outstream_create(device);
	outstream->format = SoundIoFormatFloat32NE;
	outstream->format = SoundIoFormatS32NE;
	outstream->write_callback = write_callback;
	int fb[] = { soundio_device_supports_format(device, SoundIoFormatS16NE)
		, soundio_device_supports_format(device, SoundIoFormatS32NE)
		,soundio_device_supports_format(device, SoundIoFormatFloat32NE)
		,soundio_device_supports_format(device, SoundIoFormatFloat64NE)
	};
	if ((err = soundio_outstream_open(outstream))) {
		fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
		return 1;
	}

	if (outstream->layout_error)
		fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(outstream->layout_error));

	if ((err = soundio_outstream_start(outstream))) {
		fprintf(stderr, "unable to start device: %s", soundio_strerror(err));
		return 1;
	}

	for (;;)
		soundio_wait_events(soundio);

	soundio_outstream_destroy(outstream);
	soundio_device_unref(device);
	soundio_destroy(soundio);
}


#include "stb_src/stb_vorbis.h"
#include <pnguo/mapView.h>

struct AudioSpec_t
{
	int freq;                   /**< DSP frequency -- samples per second */
	int format;     /**< Audio data format */
	uint8_t channels;             /**< Number of channels: 1 mono, 2 stereo */
	uint8_t silence;              /**< Audio buffer silence value (calculated) */
	uint16_t samples;             /**< Audio buffer size in sample FRAMES (total samples divided by channel count) */
	uint16_t padding;             /**< Necessary for some compile environments */
	uint32_t size;                /**< Audio buffer size in bytes (calculated) */
	void* callback;				/**< Callback that feeds the audio device (NULL to use SDL_QueueAudio()). */
	void* userdata;             /**< Userdata passed to callback (ignored for NULL callbacks). */
};

class audio_data_cx :public hz::rw_t
{
public:
	audio_data_cx()
	{
	}

	~audio_data_cx()
	{
	}
	int put(const void* buf, int len)
	{
		char* d = (char*)buf;
		auto it = _data.insert(_data.begin() + _data.size(), d, d + len);
		set(_data.data(), _data.size());
		return it != _data.end() ? 0 : -1;
	}
	int get(void* buf, int len)
	{
		auto s = len - len % blockalign;
		return read(buf, s);
	}
	int flush()
	{
		is_flush = true;
		return 0;
	}
	bool is_done()
	{
		return is_flush;
	}
	void clear()
	{
		_data.clear();
		set(0, 0);
		seek(0);
		is_flush = false;
	}
private:
	std::vector<uint8_t> _data;
	// 对齐字节
	int blockalign = 4;
	bool is_flush = false;
};
class music_de_t
{
public:
	music_de_t()
	{
	}

	~music_de_t()
	{
		//if (stream)
		//{
		//	delete stream;
		//}
		if (src)
		{
			delete src;
		}
	}
	void set_spec(int freq_rate, int channels, int format)
	{
		spec.freq = freq_rate;
		spec.channels = channels;
		spec.format = format;
	}
	int get_rate()
	{
		return spec.freq;
	}
	int get_channels()
	{
		return spec.channels;
	}
	int get_bits_per()
	{
		return bits_per_sample;
	}
	bool is_done() { return isdone; }
	static void get_info(void* music, int* total_samples, int* channels, int* sample_rate, int* bits_per_sample)
	{
		if (music)
		{
			music_de_t* pw = (music_de_t*)music;
			if (total_samples)
			{
				*total_samples = pw->total_samples;
			}
			if (channels)
			{
				*channels = pw->spec.channels;
			}
			if (sample_rate)
			{
				*sample_rate = pw->spec.freq;
			}
			if (bits_per_sample)
			{
				*bits_per_sample = pw->bits_per_sample;
			}
		}
	}
public:
	hz::rw_t* src = nullptr;

	AudioSpec_t spec = {};
	// 总样本数，位
	int64_t total_samples = 0, bits_per_sample = 0;
	int64_t de_samples = 0;
	// 解码器
	void* _decoder = 0;
	//audio_data_cx* stream = 0;
	int samples = 8192;
	double seconds = 0.0;
	double divby = 0.0;// 0.000030518509476;//DIVBY32767
	int current_min = INT_MAX, current_max = INT_MIN;
	int current_min0 = INT_MAX, current_max0 = INT_MIN;
	std::vector<unsigned char> _buffer;
	// 是否全部解码完成
	bool isdone = false;
	// dll 对象
	void* ctx = nullptr;
};


/*
ogg解码器
*/ 
class ogg_decoder
{
public:
	std::vector<float> _data;
	stb_vorbis* v = 0;
	int64_t desize = 0;
	int error_r = 0;
public:
	ogg_decoder();
	~ogg_decoder();
	void open_data(char* data, int len);
	int get_data();
private:

};

ogg_decoder::ogg_decoder()
{
}

ogg_decoder::~ogg_decoder()
{
	if (v)
		stb_vorbis_close(v);
	v = 0;
}

void ogg_decoder::open_data(char* data, int len)
{
	v = stb_vorbis_open_memory((unsigned char*)data, len, &error_r, NULL);
	if (v)
	{
		//stb_vorbis_stream_length_in_samples(v);
		_data.resize(v->total_samples * v->channels);
	}
}

int ogg_decoder::get_data()
{
	auto data = _data.data() + desize;
	int n = v->sample_rate;
	auto rs = stb_vorbis_get_samples_float_interleaved((stb_vorbis*)v, v->channels, data, n) * v->channels;
	return rs;
}

struct i2_t
{
	uint64_t u = 0;
	int8_t size = 0;
};
template<class T>
void w2t(i2_t& r, uint8_t* d) {
	r.u = *(T*)d;
	r.size += sizeof(T);
}
i2_t cbor_read_int(uint8_t* d) {
	i2_t r = {};
	uint8_t& c = *d;
	r.size = 1;
	switch (c)
	{
		// Integer 0x00..0x17 (0..23)
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0A:
	case 0x0B:
	case 0x0C:
	case 0x0D:
	case 0x0E:
	case 0x0F:
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
		r.u = c;
		break;
	case 0x18: // Unsigned integer (one-byte uint8_t follows)
	{
		w2t<uint8_t>(r, d + 1);
	}
	break;

	case 0x19: // Unsigned integer (two-byte uint16_t follows)
	{
		w2t<uint16_t>(r, d + 1);
	}
	break;

	case 0x1A: // Unsigned integer (four-byte uint32_t follows)
	{
		w2t<uint32_t>(r, d + 1);
	}
	break;

	case 0x1B: // Unsigned integer (eight-byte uint64_t follows)
	{
		w2t<uint64_t>(r, d + 1);
	}
	break;

	//// Negative integer -1-0x00..-1-0x17 (-1..-24)
	//case 0x20:
	//case 0x21:
	//case 0x22:
	//case 0x23:
	//case 0x24:
	//case 0x25:
	//case 0x26:
	//case 0x27:
	//case 0x28:
	//case 0x29:
	//case 0x2A:
	//case 0x2B:
	//case 0x2C:
	//case 0x2D:
	//case 0x2E:
	//case 0x2F:
	//case 0x30:
	//case 0x31:
	//case 0x32:
	//case 0x33:
	//case 0x34:
	//case 0x35:
	//case 0x36:
	//case 0x37:
	//	r.i = c;
	//	break;
	//case 0x38: // Negative integer (one-byte uint8_t follows)
	//{
	//	r.i = *(std::uint8_t*)d;
	//}
	//break;

	//case 0x39: // Negative integer -1-n (two-byte uint16_t follows)
	//{
	//	r.i = *(std::uint16_t*)d;
	//}
	//break;

	//case 0x3A: // Negative integer -1-n (four-byte uint32_t follows)
	//{
	//	r.i = *(std::uint32_t*)d;
	//}
	//break;

	//case 0x3B: // Negative integer -1-n (eight-byte uint64_t follows)
	//{
	//	r.i = *(std::uint64_t*)d;
	//}
	//break;
	};
	return r;
}

void build_audio_test(int seconds, std::vector<float>& data)
{
#define SAMPLE_RATE 48000
#define FREQUENCY   440
#define FRAME_SIZE  1024
	int channels = 2;
	int framenum = seconds * SAMPLE_RATE;// ((SAMPLE_RATE + FRAME_SIZE - 1) / FRAME_SIZE);
	auto M_PI = glm::pi<double>();
	data.resize(framenum * channels);
	auto dtt = data.data();
	float zeta = 0; //每一个frame 的初始角度
	int amp = 10000; //幅度
	for (int i = 0; i < channels; ++i) {
		auto dst = dtt + i;
		for (int j = 0; j < framenum; ++j) {
			*dst = amp * sin(2 * M_PI * FREQUENCY / SAMPLE_RATE * j + zeta); //每一个数据递进一个角度2*PI*FREQUENCY/SAMPLE_RATE,此值为角速度
			*dst *= 0.0001;
			dst += channels;
		}
	}
}
template<class T>
void build_audio_test(int seconds, std::vector<T>& data)
{
#define SAMPLE_RATE 48000
#define FREQUENCY   440
#define FRAME_SIZE  1024
	int channels = 2;
	int framenum = seconds * SAMPLE_RATE;// ((SAMPLE_RATE + FRAME_SIZE - 1) / FRAME_SIZE);
	auto M_PI = glm::pi<double>();
	data.resize(framenum * channels);
	auto dtt = data.data();
	float zeta = 0; //每一个frame 的初始角度
	int amp = 10000; //幅度
	for (int i = 0; i < channels; ++i) {
		auto dst = dtt + i;
		for (int j = 0; j < framenum; ++j) {
			*dst = amp * sinf(2 * M_PI * FREQUENCY / SAMPLE_RATE * j + zeta); //每一个数据递进一个角度2*PI*FREQUENCY/SAMPLE_RATE,此值为角速度 
			dst += channels;
		}
	}
}
int main()
{ 
	return test_create_outstream();
}
