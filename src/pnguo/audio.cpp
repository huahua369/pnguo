﻿/*
音频实现
本文件创建时间：2025/3/19
*/
#include "pch1.h"
#include "audio.h"
#include "mapView.h"

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <stb_src/stb_vorbis.h>
#include <FLAC/all.h>
// 解码mp3
#include <mpg123.h>
// 编码mp3
#include <out123.h>
//syn123：一些音频信号合成和格式转换
#include <syn123.h>

#include <fftw3.h>
#include <cmath> 
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif // !M_PI

namespace hz {

	inline int psf_lrintf(float x)
	{
#ifndef NO_SSE2
		return _mm_cvtss_si32(_mm_load_ss(&x));
#else
		return lrintf(x);
#endif 
	} /* psf_lrintf */

	inline int psf_lrint(double x)
	{
#ifndef NO_SSE2
		return _mm_cvtsd_si32(_mm_load_sd(&x));
#else
		return lrint(x);
#endif
	} /* psf_lrintf */

	void s2flac8_array(const short* src, int32_t* dest, int count)
	{
		for (int i = 0; i < count; i++)
			dest[i] = src[i] >> 8;
	} /* s2flac8_array */

	void s2flac16_array(const short* src, int32_t* dest, int count)
	{
		for (int i = 0; i < count; i++)
			dest[i] = src[i];
	} /* s2flac16_array */

	void s2flac24_array(const short* src, int32_t* dest, int count)
	{
		for (int i = 0; i < count; i++)
			dest[i] = src[i] << 8;
	} /* s2flac24_array */

	void i2flac8_array(const int* src, int32_t* dest, int count)
	{
		for (int i = 0; i < count; i++)
			dest[i] = src[i] >> 24;
	} /* i2flac8_array */

	void i2flac16_array(const int* src, int32_t* dest, int count)
	{
		for (int i = 0; i < count; i++)
			dest[i] = src[i] >> 16;
	} /* i2flac16_array */

	void i2flac24_array(const int* src, int32_t* dest, int count)
	{
		for (int i = 0; i < count; i++)
			dest[i] = src[i] >> 8;
	} /* i2flac24_array */

	void f2flac8_clip_array(const float* src, int32_t* dest, int count, int normalize)
	{
		float normfact, scaled_value;

		normfact = normalize ? (8.0 * 0x10) : 1.0;

		for (int i = 0; i < count; i++)
		{
			scaled_value = src[i] * normfact;
			if (scaled_value >= (1.0 * 0x7F))
			{
				dest[i] = 0x7F;
				continue;
			};
			if (scaled_value <= (-8.0 * 0x10))
			{
				dest[i] = -0x80;
				continue;
			};
			dest[i] = psf_lrintf(scaled_value);
		};

		return;
	} /* f2flac8_clip_array */

	void f2flac16_clip_array(const float* src, int32_t* dest, int count, int normalize)
	{
		float normfact, scaled_value;

		normfact = normalize ? (8.0 * 0x1000) : 1.0;

		for (int i = 0; i < count; i++)
		{
			scaled_value = src[i] * normfact;
			if (scaled_value >= (1.0 * 0x7FFF))
			{
				dest[i] = 0x7FFF;
				continue;
			};
			if (scaled_value <= (-8.0 * 0x1000))
			{
				dest[i] = -0x8000;
				continue;
			};
			dest[i] = psf_lrintf(scaled_value);
		};
	} /* f2flac16_clip_array */

	void f2flac24_clip_array(const float* src, int32_t* dest, int count, int normalize)
	{
		float normfact, scaled_value;

		normfact = normalize ? (8.0 * 0x100000) : 1.0;

		for (int i = 0; i < count; i++)
		{
			scaled_value = src[i] * normfact;
			if (scaled_value >= (1.0 * 0x7FFFFF))
			{
				dest[i] = 0x7FFFFF;
				continue;
			};

			if (scaled_value <= (-8.0 * 0x100000))
			{
				dest[i] = -0x800000;
				continue;
			}
			dest[i] = psf_lrintf(scaled_value);
		};

		return;
	} /* f2flac24_clip_array */

	void f2flac8_array(const float* src, int32_t* dest, int count, int normalize)
	{
		float normfact = normalize ? (1.0 * 0x7F) : 1.0;

		for (int i = 0; i < count; i++)
			dest[i] = psf_lrintf(src[i] * normfact);
	} /* f2flac8_array */

	void f2flac16_array(const float* src, int32_t* dest, int count, int normalize)
	{
		float normfact = normalize ? (1.0 * 0x7FFF) : 1.0;

		for (int i = 0; i < count; i++)
			dest[i] = psf_lrintf(src[i] * normfact);
	} /* f2flac16_array */

	void f2flac24_array(const float* src, int32_t* dest, int count, int normalize)
	{
		float normfact = normalize ? (1.0 * 0x7FFFFF) : 1.0;

		for (int i = 0; i < count; i++)
			dest[i] = psf_lrintf(src[i] * normfact);
	} /* f2flac24_array */

	void d2flac8_clip_array(const double* src, int32_t* dest, int count, int normalize)
	{
		double normfact, scaled_value;

		normfact = normalize ? (8.0 * 0x10) : 1.0;

		for (int i = 0; i < count; i++)
		{
			scaled_value = src[i] * normfact;
			if (scaled_value >= (1.0 * 0x7F))
			{
				dest[i] = 0x7F;
				continue;
			};
			if (scaled_value <= (-8.0 * 0x10))
			{
				dest[i] = -0x80;
				continue;
			};
			dest[i] = psf_lrint(scaled_value);
		};

		return;
	} /* d2flac8_clip_array */

	void d2flac16_clip_array(const double* src, int32_t* dest, int count, int normalize)
	{
		double normfact, scaled_value;

		normfact = normalize ? (8.0 * 0x1000) : 1.0;

		for (int i = 0; i < count; i++)
		{
			scaled_value = src[i] * normfact;
			if (scaled_value >= (1.0 * 0x7FFF))
			{
				dest[i] = 0x7FFF;
				continue;
			};
			if (scaled_value <= (-8.0 * 0x1000))
			{
				dest[i] = -0x8000;
				continue;
			};
			dest[i] = psf_lrint(scaled_value);
		};

		return;
	} /* d2flac16_clip_array */

	void d2flac24_clip_array(const double* src, int32_t* dest, int count, int normalize)
	{
		double normfact, scaled_value;

		normfact = normalize ? (8.0 * 0x100000) : 1.0;

		for (int i = 0; i < count; i++)
		{
			scaled_value = src[i] * normfact;
			if (scaled_value >= (1.0 * 0x7FFFFF))
			{
				dest[i] = 0x7FFFFF;
				continue;
			};
			if (scaled_value <= (-8.0 * 0x100000))
			{
				dest[i] = -0x800000;
				continue;
			};
			dest[i] = psf_lrint(scaled_value);
		};

		return;
	} /* d2flac24_clip_array */

	void d2flac8_array(const double* src, int32_t* dest, int count, int normalize)
	{
		double normfact = normalize ? (1.0 * 0x7F) : 1.0;

		for (int i = 0; i < count; i++)
			dest[i] = psf_lrint(src[i] * normfact);
	} /* d2flac8_array */

	void d2flac16_array(const double* src, int32_t* dest, int count, int normalize)
	{
		double normfact = normalize ? (1.0 * 0x7FFF) : 1.0;

		for (int i = 0; i < count; i++)
			dest[i] = psf_lrint(src[i] * normfact);
	} /* d2flac16_array */

	void d2flac24_array(const double* src, int32_t* dest, int count, int normalize)
	{
		double normfact = normalize ? (1.0 * 0x7FFFFF) : 1.0;

		for (int i = 0; i < count; i++)
			dest[i] = psf_lrint(src[i] * normfact);
	} /* d2flac24_array */




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

	class audio_data_cx :public rw_t
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
			if (src)
			{
				delete src;
				src = 0;
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
		static void get_info(void* music, audio_spec_t* p)
		{
			if (music && p)
			{
				music_de_t* pw = (music_de_t*)music;
				p->total_samples = pw->total_samples;
				p->channels = pw->spec.channels;
				p->sample_rate = pw->spec.freq;
				p->format = pw->spec.format;
				p->bits_per_sample = pw->bits_per_sample;
			}
		}
	public:
		rw_t* src = nullptr;

		AudioSpec_t spec = {};
		// 总样本数，位
		int64_t total_samples = 0, bits_per_sample = 0;
		int64_t de_samples = 0;
		// 解码器
		void* _decoder = 0;
		//audio_data_t* stream = 0; 

		int samples = 8192;
		int pcmtype = 0;
		uint32_t pos = 0, len = 0, remain = 0;
		size_t bufferpos;
		char* ptr = 0;
		size_t wlen = 0;
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


#ifndef _NO_FLAC_DE_

	typedef struct {
		void* loaded;
		void* handle;
		FLAC__StreamDecoder* (*FLAC__stream_decoder_new)(void);
		void (*FLAC__stream_decoder_delete)(FLAC__StreamDecoder* decoder);
		FLAC__StreamDecoderInitStatus(*FLAC__stream_decoder_init_stream)(
			FLAC__StreamDecoder* decoder,
			FLAC__StreamDecoderReadCallback read_callback,
			FLAC__StreamDecoderSeekCallback seek_callback,
			FLAC__StreamDecoderTellCallback tell_callback,
			FLAC__StreamDecoderLengthCallback length_callback,
			FLAC__StreamDecoderEofCallback eof_callback,
			FLAC__StreamDecoderWriteCallback write_callback,
			FLAC__StreamDecoderMetadataCallback metadata_callback,
			FLAC__StreamDecoderErrorCallback error_callback,
			void* client_data);
		FLAC__bool(*FLAC__stream_decoder_finish)(FLAC__StreamDecoder* decoder);
		FLAC__bool(*FLAC__stream_decoder_flush)(FLAC__StreamDecoder* decoder);
		FLAC__bool(*FLAC__stream_decoder_process_single)(FLAC__StreamDecoder* decoder);
		FLAC__bool(*FLAC__stream_decoder_process_until_end_of_metadata)(FLAC__StreamDecoder* decoder);
		FLAC__bool(*FLAC__stream_decoder_process_until_end_of_stream)(FLAC__StreamDecoder* decoder);
		FLAC__bool(*FLAC__stream_decoder_seek_absolute)(FLAC__StreamDecoder* decoder, FLAC__uint64 sample);
		FLAC__StreamDecoderState(*FLAC__stream_decoder_get_state)(const FLAC__StreamDecoder* decoder);
		// encoder
		FLAC__StreamEncoder* (*FLAC__stream_encoder_new)(void);
		void (*FLAC__stream_encoder_delete)(FLAC__StreamEncoder* encoder);

		FLAC__bool(*FLAC__stream_encoder_set_verify)(FLAC__StreamEncoder* encoder, FLAC__bool value);
		FLAC__bool(*FLAC__stream_encoder_set_streamable_subset)(FLAC__StreamEncoder* encoder, FLAC__bool value);
		FLAC__bool(*FLAC__stream_encoder_set_channels)(FLAC__StreamEncoder* encoder, uint32_t value);
		FLAC__bool(*FLAC__stream_encoder_set_bits_per_sample)(FLAC__StreamEncoder* encoder, uint32_t value);
		FLAC__bool(*FLAC__stream_encoder_set_sample_rate)(FLAC__StreamEncoder* encoder, uint32_t value);
		FLAC__bool(*FLAC__stream_encoder_set_compression_level)(FLAC__StreamEncoder* encoder, uint32_t value);
		FLAC__bool(*FLAC__stream_encoder_set_blocksize)(FLAC__StreamEncoder* encoder, uint32_t value);
		FLAC__bool(*FLAC__stream_encoder_set_total_samples_estimate)(FLAC__StreamEncoder* encoder, FLAC__uint64 value);
		FLAC__bool(*FLAC__stream_encoder_set_metadata)(FLAC__StreamEncoder* encoder, FLAC__StreamMetadata** metadata, uint32_t num_blocks);

		FLAC__uint64(*FLAC__stream_encoder_get_total_samples_estimate)(const FLAC__StreamEncoder* encoder);
		FLAC__StreamDecoderState(*FLAC__stream_encoder_get_verify_decoder_state)(const FLAC__StreamEncoder* encoder);
		FLAC__StreamEncoderState(*FLAC__stream_encoder_get_state)(const FLAC__StreamEncoder* encoder);
		const char* (*FLAC__stream_encoder_get_resolved_state_string)(const FLAC__StreamEncoder* encoder);
		void (*FLAC__stream_encoder_get_verify_decoder_error_stats)(const FLAC__StreamEncoder* encoder, FLAC__uint64* absolute_sample, uint32_t* frame_number, uint32_t* channel, uint32_t* sample, FLAC__int32* expected, FLAC__int32* got);
		uint32_t(*FLAC__stream_encoder_get_blocksize)(const FLAC__StreamEncoder* encoder);

		FLAC__StreamEncoderInitStatus(*FLAC__stream_encoder_init_stream)(FLAC__StreamEncoder* encoder, FLAC__StreamEncoderWriteCallback write_callback, FLAC__StreamEncoderSeekCallback seek_callback, FLAC__StreamEncoderTellCallback tell_callback, FLAC__StreamEncoderMetadataCallback metadata_callback, void* client_data);
		FLAC__StreamEncoderInitStatus(*FLAC__stream_encoder_init_ogg_stream)(FLAC__StreamEncoder* encoder, FLAC__StreamEncoderReadCallback read_callback, FLAC__StreamEncoderWriteCallback write_callback, FLAC__StreamEncoderSeekCallback seek_callback, FLAC__StreamEncoderTellCallback tell_callback, FLAC__StreamEncoderMetadataCallback metadata_callback, void* client_data);
		FLAC__StreamEncoderInitStatus(*FLAC__stream_encoder_init_file)(FLAC__StreamEncoder* encoder, const char* filename, FLAC__StreamEncoderProgressCallback progress_callback, void* client_data);
		FLAC__StreamEncoderInitStatus(*FLAC__stream_encoder_init_ogg_file)(FLAC__StreamEncoder* encoder, const char* filename, FLAC__StreamEncoderProgressCallback progress_callback, void* client_data);
		FLAC__bool(*FLAC__stream_encoder_finish)(FLAC__StreamEncoder* encoder);
		FLAC__bool(*FLAC__stream_encoder_process_interleaved)(FLAC__StreamEncoder* encoder, const FLAC__int32 buffer[], uint32_t samples);


		const char* const* FLAC__StreamEncoderStateString;
		const char* const* FLAC__StreamEncoderInitStatusString;
		FLAC__StreamMetadata* (*FLAC__metadata_object_new)(FLAC__MetadataType type);
		void (*FLAC__metadata_object_delete)(FLAC__StreamMetadata* object);
		FLAC__bool(*FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair)(FLAC__StreamMetadata_VorbisComment_Entry* entry, const char* field_name, const char* field_value);
		FLAC__bool(*FLAC__metadata_object_vorbiscomment_append_comment)(FLAC__StreamMetadata* object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy);

	} flac_loader;

	int flac_load(flac_loader* flac) {
		static const char* ccfn[] = {
			"FLAC__stream_decoder_new",
			"FLAC__stream_decoder_delete",
			"FLAC__stream_decoder_init_stream",
			"FLAC__stream_decoder_finish",
			"FLAC__stream_decoder_flush",
			"FLAC__stream_decoder_process_single",
			"FLAC__stream_decoder_process_until_end_of_metadata",
			"FLAC__stream_decoder_process_until_end_of_stream",
			"FLAC__stream_decoder_seek_absolute",
			"FLAC__stream_decoder_get_state",

			"FLAC__stream_encoder_new",
			"FLAC__stream_encoder_delete",

			"FLAC__stream_encoder_set_verify",
			"FLAC__stream_encoder_set_streamable_subset",
			"FLAC__stream_encoder_set_channels",
			"FLAC__stream_encoder_set_bits_per_sample",
			"FLAC__stream_encoder_set_sample_rate",
			"FLAC__stream_encoder_set_compression_level",
			"FLAC__stream_encoder_set_blocksize",
			"FLAC__stream_encoder_set_total_samples_estimate",
			"FLAC__stream_encoder_set_metadata",

			"FLAC__stream_encoder_get_total_samples_estimate",
			"FLAC__stream_encoder_get_verify_decoder_state",
			"FLAC__stream_encoder_get_state",
			"FLAC__stream_encoder_get_resolved_state_string",
			"FLAC__stream_encoder_get_verify_decoder_error_stats",
			"FLAC__stream_encoder_get_blocksize",

			"FLAC__stream_encoder_init_stream",
			"FLAC__stream_encoder_init_ogg_stream",
			"FLAC__stream_encoder_init_file",
			"FLAC__stream_encoder_init_ogg_file",
			"FLAC__stream_encoder_finish",
			"FLAC__stream_encoder_process_interleaved",

			"FLAC__StreamEncoderStateString",
			"FLAC__StreamEncoderInitStatusString",
			"FLAC__metadata_object_new",
			"FLAC__metadata_object_delete",
			"FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair",
			"FLAC__metadata_object_vorbiscomment_append_comment",
		};
		if (flac->handle)
		{
			return 0;
		}
#ifdef FLAC_DYNAMIC
		auto libflac = Shared::loadShared(FLAC_DYNAMIC);
		if (!libflac)
		{
			return -1;
		}
		libflac->dllsyms(ccfn, (void**)&flac->FLAC__stream_decoder_new, sizeof(ccfn) / sizeof(char*));
		flac->handle = libflac;
#elif defined(__MACOSX__)
		extern FLAC__StreamDecoder* FLAC__stream_decoder_new(void) __attribute__((weak_import));
		if (FLAC__stream_decoder_new == NULL)
		{
			/* Missing weakly linked framework */
			Mix_SetError("Missing FLAC.framework");
			return -1;
		}
#else
#ifndef FLAC_GETCB
#define FLAC_GETCB(n) flac-> n=n
#endif // !FLAC_GETCB
		FLAC_GETCB(FLAC__stream_decoder_new);
		FLAC_GETCB(FLAC__stream_decoder_delete);
		FLAC_GETCB(FLAC__stream_decoder_init_stream);
		FLAC_GETCB(FLAC__stream_decoder_finish);
		FLAC_GETCB(FLAC__stream_decoder_flush);
		FLAC_GETCB(FLAC__stream_decoder_process_single);
		FLAC_GETCB(FLAC__stream_decoder_process_until_end_of_metadata);
		FLAC_GETCB(FLAC__stream_decoder_process_until_end_of_stream);
		FLAC_GETCB(FLAC__stream_decoder_seek_absolute);
		FLAC_GETCB(FLAC__stream_decoder_get_state);

		FLAC_GETCB(FLAC__stream_encoder_new);
		FLAC_GETCB(FLAC__stream_encoder_delete);

		FLAC_GETCB(FLAC__stream_encoder_set_verify);
		FLAC_GETCB(FLAC__stream_encoder_set_streamable_subset);
		FLAC_GETCB(FLAC__stream_encoder_set_channels);
		FLAC_GETCB(FLAC__stream_encoder_set_bits_per_sample);
		FLAC_GETCB(FLAC__stream_encoder_set_sample_rate);
		FLAC_GETCB(FLAC__stream_encoder_set_compression_level);
		FLAC_GETCB(FLAC__stream_encoder_set_blocksize);
		FLAC_GETCB(FLAC__stream_encoder_set_total_samples_estimate);
		FLAC_GETCB(FLAC__stream_encoder_set_metadata);

		FLAC_GETCB(FLAC__stream_encoder_get_total_samples_estimate);
		FLAC_GETCB(FLAC__stream_encoder_get_verify_decoder_state);
		FLAC_GETCB(FLAC__stream_encoder_get_state);
		FLAC_GETCB(FLAC__stream_encoder_get_resolved_state_string);
		FLAC_GETCB(FLAC__stream_encoder_get_verify_decoder_error_stats);
		FLAC_GETCB(FLAC__stream_encoder_get_blocksize);

		FLAC_GETCB(FLAC__stream_encoder_init_stream);
		FLAC_GETCB(FLAC__stream_encoder_init_ogg_stream);
		FLAC_GETCB(FLAC__stream_encoder_init_file);
		FLAC_GETCB(FLAC__stream_encoder_init_ogg_file);
		FLAC_GETCB(FLAC__stream_encoder_finish);
		FLAC_GETCB(FLAC__stream_encoder_process_interleaved);

		FLAC_GETCB(FLAC__StreamEncoderStateString);
		FLAC_GETCB(FLAC__StreamEncoderInitStatusString);
		FLAC_GETCB(FLAC__metadata_object_new);
		FLAC_GETCB(FLAC__metadata_object_delete);
		FLAC_GETCB(FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair);
		FLAC_GETCB(FLAC__metadata_object_vorbiscomment_append_comment);
#endif
		return 0;
	}
	void flac_unload(flac_loader* flac)
	{
		if (flac)
		{
			if (flac->handle)
			{
				Shared::destroy((Shared*)flac->handle);
				flac->handle = 0;
			}
		}
	}

	class flac_de_t
	{
	private:
		flac_loader flac[1] = { };
	public:
		flac_de_t()
		{
			flac_load(flac);
		}

		~flac_de_t()
		{
			flac_unload(flac);
		}
		flac_loader* lib()
		{
			return flac;
		}
		static void* load()
		{
			flac_de_t* p = new flac_de_t();
			return p;
		}
		static void unload(void* ph)
		{
			flac_de_t* p = (flac_de_t*)ph;
			if (p)
			{
				delete p;
			}
		}
		static void* open_memory(const void* data, int len, void* handle)
		{
			int data_len, offset, total, limit;
			if (!len || !data || !handle)
			{
				return nullptr;
			}
			rw_t* src = new rw_t();
			music_de_t* music = new music_de_t();
			int init_stage = 0;
			int was_error = 1;
			auto flac = ((flac_de_t*)handle)->lib();
			music->ctx = flac;
			music->_decoder = flac->FLAC__stream_decoder_new();
			src->set_end(len);
			src->set_data((void*)data);
			if (music->_decoder) {
				{
					music->src = src;
					init_stage++; /* stage 1! */
					if (flac->FLAC__stream_decoder_init_stream(
						(FLAC__StreamDecoder*)music->_decoder,
						flac_read_music_cb, flac_seek_music_cb,
						flac_tell_music_cb, flac_length_music_cb,
						flac_eof_music_cb, flac_write_music_cb,
						flac_metadata_music_cb, flac_error_music_cb,
						music) == FLAC__STREAM_DECODER_INIT_STATUS_OK) {
						init_stage++; /* stage 2! */

						if (flac->FLAC__stream_decoder_process_until_end_of_metadata((FLAC__StreamDecoder*)music->_decoder)) {
							was_error = 0;
						}
						else {
							printf("FLAC__stream_decoder_process_until_end_of_metadata() failed");
						}
					}
					else {
						printf("FLAC__stream_decoder_init_stream() failed");
					}
				}
			}
			else {
				printf("FLAC__stream_decoder_new() failed");
			}

			if (was_error) {
				switch (init_stage) {
				case 2:
					flac->FLAC__stream_decoder_finish((FLAC__StreamDecoder*)music->_decoder);
				case 1:
					flac->FLAC__stream_decoder_delete((FLAC__StreamDecoder*)music->_decoder);
				case 0:
					delete music;
					break;
				}
				return NULL;
			}
			return music;
		}
		static int get_audio(void* music, void* data, int bytes)
		{
			music_de_t* p = (music_de_t*)music;
			if (!p)return 0;
			bool* done = &p->isdone;
			return FLAC_GetSome(music, data, bytes, done);
		}

		/* Seek to a play position (in seconds) */
		static int seek(void* context, double position)
		{
			music_de_t* music = (music_de_t*)context;
			double seek_sample = music->get_rate() * position;

			//music->stream->clear();
			auto flac = (flac_loader*)music->ctx;
			if (!flac->FLAC__stream_decoder_seek_absolute((FLAC__StreamDecoder*)music->_decoder, (FLAC__uint64)seek_sample)) {
				if (flac->FLAC__stream_decoder_get_state((FLAC__StreamDecoder*)music->_decoder) == FLAC__STREAM_DECODER_SEEK_ERROR) {
					flac->FLAC__stream_decoder_flush((FLAC__StreamDecoder*)music->_decoder);
				}
				printf("Seeking of FLAC stream failed: libFLAC seek failed.");
				return -1;
			}
			return 0;
		}

		/* Delete a music object */
		static void free_m(void* p)
		{
			music_de_t* music = (music_de_t*)p;
			if (music) {
				auto flac = (flac_loader*)music->ctx;
				if (music->_decoder) {
					flac->FLAC__stream_decoder_finish((FLAC__StreamDecoder*)music->_decoder);
					flac->FLAC__stream_decoder_delete((FLAC__StreamDecoder*)music->_decoder);
				}
				//if (music->stream) {
				//	delete music->stream;
				//}
				if (music->src)
				{
					delete music->src;
				}
				delete music;
			}
		}
	private:

#if 1
		static FLAC__StreamDecoderReadStatus flac_read_music_cb(
			const FLAC__StreamDecoder* decoder,
			FLAC__byte buffer[],
			size_t* bytes,
			void* client_data)
		{
			music_de_t* data = (music_de_t*)client_data;
			rw_t* mv = (rw_t*)data->src;
			/* make sure there is something to be reading */
			if (*bytes > 0) {
				//*bytes = SDL_RWread(data->src, buffer, sizeof(FLAC__byte), 
				*bytes = mv->read(buffer, *bytes);
				if (*bytes == 0) { /* error or no data was read (EOF) */
					return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
				}
				else { /* data was read, continue */
					return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
				}
			}
			else {
				return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
			}
		}

		static FLAC__StreamDecoderSeekStatus flac_seek_music_cb(
			const FLAC__StreamDecoder* decoder,
			FLAC__uint64 absolute_byte_offset,
			void* client_data)
		{
			music_de_t* data = (music_de_t*)client_data;
			rw_t* mv = (rw_t*)data->src;
			if (mv->seek(absolute_byte_offset) < 0) {
				return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
			}
			else {
				return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
			}
		}

		static FLAC__StreamDecoderTellStatus flac_tell_music_cb(
			const FLAC__StreamDecoder* decoder,
			FLAC__uint64* absolute_byte_offset,
			void* client_data)
		{
			music_de_t* data = (music_de_t*)client_data;
			rw_t* mv = (rw_t*)data->src;
			int64_t pos = mv->tell();// = SDL_RWtell(data->src);

			if (pos < 0) {
				return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
			}
			else {
				*absolute_byte_offset = (FLAC__uint64)pos;
				return FLAC__STREAM_DECODER_TELL_STATUS_OK;
			}
		}

		static FLAC__StreamDecoderLengthStatus flac_length_music_cb(
			const FLAC__StreamDecoder* decoder,
			FLAC__uint64* stream_length,
			void* client_data)
		{
			music_de_t* data = (music_de_t*)client_data;
			rw_t* mv = (rw_t*)data->src;
			if (data && data->src)
			{
				int64_t length = mv->size();
				*stream_length = (FLAC__uint64)length;
				return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
			}
			return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
		}

		static FLAC__bool flac_eof_music_cb(
			const FLAC__StreamDecoder* decoder,
			void* client_data)
		{
			music_de_t* music = (music_de_t*)client_data;
			rw_t* mv = (rw_t*)music->src;

			return mv->iseof();
		}

		static void Convert_S32_to_F32_Scalar(float* dst, const int32_t* src, int num_samples, double divby)
		{
#ifndef DIVBY2147483648
#define DIVBY2147483648 0.0000000004656612873077392578125
#define DIVBY32768 0.000030517578125 
#endif
			//int x = INT_MIN;
			//int n = INT_MAX;
			//for (size_t i = 0; i < num_samples; i++)
			//{
			//	if (src[i] > x)
			//	{
			//		x = src[i];
			//	}
			//	if (src[i] < n)
			//	{
			//		n = src[i];
			//	} 
			//}
			for (int i = 0; i < num_samples; i++) {
				dst[i] = (float)src[i] * divby;
			}
		}

		typedef enum
		{
			PFLAC_PCM_SHORT,
			PFLAC_PCM_INT,
			PFLAC_PCM_FLOAT,
			PFLAC_PCM_DOUBLE
		} PFLAC_PCM;
		struct flac_buf_t
		{
			const FLAC__StreamDecoder* decoder;
			const FLAC__Frame* frame;
			const FLAC__int32** wbuffer;
			music_de_t* music = 0;
			int norm_double = 1;
			int norm_float = 1;
		};
		static size_t flac_buffer_copy(flac_buf_t* psf)
		{
			const FLAC__Frame* frame = psf->frame;
			const int32_t* const* buffer = psf->wbuffer;
			size_t i = 0, j = 0, offset = 0, len = 0;
			auto pflac = psf->music;
			if (psf->music->spec.channels != (int)frame->header.channels)
			{
				// 通道数错误
				return 0;
			}
			if (frame->header.blocksize > FLAC__MAX_BLOCK_SIZE)
			{
				printf("%s\t%d\t: frame->header.blocksize (%u) > FLAC__MAX_BLOCK_SIZE (%u)\n", __func__, __LINE__, frame->header.blocksize, FLAC__MAX_BLOCK_SIZE);
				return 0;
			}
			if (frame->header.channels > FLAC__MAX_CHANNELS)
				printf("%s\t%d\t: frame->header.channels (%u) > FLAC__MAX_BLOCK_SIZE (%u)\n", __func__, __LINE__, frame->header.channels, FLAC__MAX_CHANNELS);

			uint32_t channels = glm::min(frame->header.channels, FLAC__MAX_CHANNELS);
			len = glm::min(pflac->len, frame->header.blocksize);
			if (pflac->remain % channels != 0)
			{
				printf("Error: pflac->remain %u    channels %u\n", pflac->remain, channels);
				return 0;
			}
			if (!pflac->ptr || pflac->_buffer.size() < len * sizeof(double))
			{
				pflac->_buffer.resize(len * sizeof(double));
				pflac->ptr = (char*)pflac->_buffer.data();
			}
			switch (pflac->pcmtype)
			{
			case PFLAC_PCM_SHORT:
			{
				short* retpcm = (short*)pflac->ptr;
				int shift = 16 - frame->header.bits_per_sample;
				pflac->wlen = sizeof(short);
				if (shift < 0)
				{
					shift = abs(shift);
					for (i = 0; i < len && pflac->remain > 0; i++)
					{
						offset = pflac->pos + i * channels;
						if (pflac->bufferpos >= frame->header.blocksize)
							break;
						if (offset + channels > pflac->len)
							break;
						for (j = 0; j < channels; j++)
							retpcm[offset + j] = buffer[j][pflac->bufferpos] >> shift;
						pflac->remain -= channels;
						pflac->bufferpos++;
					}
				}
				else
				{
					for (i = 0; i < len && pflac->remain > 0; i++)
					{
						offset = pflac->pos + i * channels;
						if (pflac->bufferpos >= frame->header.blocksize)
							break;
						if (offset + channels > pflac->len)
							break;
						for (j = 0; j < channels; j++)
							retpcm[offset + j] = ((uint16_t)buffer[j][pflac->bufferpos]) << shift;
						pflac->remain -= channels;
						pflac->bufferpos++;
					}
				}
			}
			break;
			case PFLAC_PCM_INT:
			{
				int* retpcm = (int*)pflac->ptr;
				int shift = 32 - frame->header.bits_per_sample;
				pflac->wlen = sizeof(int);
				for (i = 0; i < len && pflac->remain > 0; i++)
				{
					offset = pflac->pos + i * channels;
					if (pflac->bufferpos >= frame->header.blocksize)
						break;
					if (offset + channels > pflac->len)
						break;
					for (j = 0; j < channels; j++)
						retpcm[offset + j] = ((uint32_t)buffer[j][pflac->bufferpos]) << shift;
					pflac->remain -= channels;
					pflac->bufferpos++;
				}
			}
			break;
			case PFLAC_PCM_FLOAT:
			{
				float* retpcm = (float*)pflac->ptr;
				uint32_t bps = (1 << (frame->header.bits_per_sample - 1));
				float norm = (psf->norm_float) ? 1.0 / bps : 1.0;
				pflac->wlen = sizeof(float);
				for (i = 0; i < len && pflac->remain > 0; i++)
				{
					offset = pflac->pos + i * channels;
					if (pflac->bufferpos >= frame->header.blocksize)
						break;
					if (offset + channels > pflac->len)
						break;
					for (j = 0; j < channels; j++)
						retpcm[offset + j] = buffer[j][pflac->bufferpos] * norm;
					pflac->remain -= channels;
					pflac->bufferpos++;
				}
			}
			break;
			case PFLAC_PCM_DOUBLE:
			{
				double* retpcm = (double*)pflac->ptr;
				uint32_t bps = (1 << (frame->header.bits_per_sample - 1));
				double norm = (psf->norm_double) ? 1.0 / bps : 1.0;
				pflac->wlen = sizeof(double);
				for (i = 0; i < len && pflac->remain > 0; i++)
				{
					offset = pflac->pos + i * channels;
					if (pflac->bufferpos >= frame->header.blocksize)
						break;
					if (offset + channels > pflac->len)
						break;
					for (j = 0; j < channels; j++)
						retpcm[offset + j] = buffer[j][pflac->bufferpos] * norm;
					pflac->remain -= channels;
					pflac->bufferpos++;
				}
			}
			break;
			default:
				return 0;
			};
			pflac->wlen *= offset;
			offset = i * channels;
			pflac->pos += i * channels;

			return offset;
		}

		static FLAC__StreamDecoderWriteStatus flac_write_music_cb(
			const FLAC__StreamDecoder* decoder,
			const FLAC__Frame* frame,
			const FLAC__int32* const buffer[],
			void* client_data)
		{
			music_de_t* music = (music_de_t*)client_data;
			flac_buf_t pfb = { decoder, frame, (const FLAC__int32**)buffer, music };
			music->bufferpos = 0;
			flac_buffer_copy(&pfb);
			return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
		}

		static void flac_metadata_music_cb(
			const FLAC__StreamDecoder* decoder,
			const FLAC__StreamMetadata* metadata,
			void* client_data)
		{
			music_de_t* music = (music_de_t*)client_data;
			rw_t* mv = (rw_t*)music->src;
			int channels;

			if (metadata->type != FLAC__METADATA_TYPE_STREAMINFO) {
				return;
			}
			int ns = sizeof(short);
			music->bits_per_sample = metadata->data.stream_info.bits_per_sample;
			int format = 0;
			//  PFLAC_PCM_SHORT,
			//	PFLAC_PCM_INT,
			//	PFLAC_PCM_FLOAT,
			//	PFLAC_PCM_DOUBLE
			// todo 输出格式pcmtype
			switch (music->bits_per_sample)
			{
			case 16:
			{
				music->pcmtype = 0; format = 0;
			}
			break;
			case 24:
			{
				music->pcmtype = 1; format = 1;
			}
			break;
			case 32:
			{
				music->pcmtype = 2; format = 2;
			}
			break;
			}
			music->set_spec(metadata->data.stream_info.sample_rate, metadata->data.stream_info.channels, format);// SDL_AUDIO_S16);
			music->total_samples = metadata->data.stream_info.total_samples;
			double t = music->total_samples; t /= music->spec.freq;
			music->seconds = t;

			//music->stream.resize(music->total_samples * metadata->data.stream_info.channels * sizeof(float));
		}

		static void flac_error_music_cb(
			const FLAC__StreamDecoder* decoder,
			FLAC__StreamDecoderErrorStatus status,
			void* client_data)
		{
			music_de_t* music = (music_de_t*)client_data;
			rw_t* mv = (rw_t*)music->src;
			/* print an SDL error based on the error status */
			switch (status) {
			case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC:
				printf("Error processing the FLAC file [LOST_SYNC].");
				break;
			case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER:
				printf("Error processing the FLAC file [BAD_HEADER].");
				break;
			case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH:
				printf("Error processing the FLAC file [CRC_MISMATCH].");
				break;
			case FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM:
				printf("Error processing the FLAC file [UNPARSEABLE].");
				break;
			default:
				printf("Error processing the FLAC file [UNKNOWN].");
				break;
			}
		}


		/* Read some FLAC stream data and convert it for output */
		static int FLAC_GetSome(void* context, void* data, int bytes, bool* done)
		{
			music_de_t* music = (music_de_t*)context;
			int filled = 0;
			do
			{
				music->pos = 0;
				music->len = bytes;
				music->remain = bytes;
				music->_buffer.clear();
				auto flac = (flac_loader*)music->ctx;
				if (!flac->FLAC__stream_decoder_process_single((FLAC__StreamDecoder*)music->_decoder)) {
					printf("FLAC__stream_decoder_process_single() failed");
					return -1;
				}
				if (flac->FLAC__stream_decoder_get_state((FLAC__StreamDecoder*)music->_decoder) == FLAC__STREAM_DECODER_END_OF_STREAM) {
					music->_buffer.clear();
					music->wlen = 0;
					if (done)
						*done = true;
				}
				filled = music->wlen;
				if (filled)
				{
					memcpy(data, music->_buffer.data(), filled);
				}

			} while (0);
			return filled;
		}

#endif
	};

#endif // !_NO_FLAC_DE_


	// flac编码器
#ifndef _NO_FLAC_EN_

	class flac_en_t
	{
	public:
		flac_loader* flac = 0;
		class en_data_t :public rw_t
		{
		public:
			void* ud = 0;
			int(*write_func)(const char* data, size_t bytes, uint32_t samples, uint32_t current_frame, void* ud) = nullptr;
			int (*seek_func)(uint64_t pos, void* ud) = nullptr;
			uint64_t(*tell_func)(void* ud) = nullptr;
		public:
			en_data_t()
			{
			}

			~en_data_t()
			{
			}

		private:

		};

	public:
		flac_en_t(flac_loader* ctx) :flac(ctx)
		{
		}

		~flac_en_t()
		{
		}
		static FLAC__StreamEncoderWriteStatus enWriteCallback(const FLAC__StreamEncoder* encoder, const FLAC__byte buffer[], size_t bytes
			, uint32_t samples, uint32_t current_frame, void* client_data)
		{
			en_data_t* t = (en_data_t*)client_data;
			if (NULL == bytes) {
				return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
			}
			int ws = 0;
			if (t->write_func)
			{
				ws = t->write_func((char*)buffer, bytes, samples, current_frame, t->ud);
			}
			else
			{
				t->seek(bytes, SEEK_CUR);
			}
			//auto ret = t->write(buffer, bytes);
			return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;

		}
		static FLAC__StreamEncoderSeekStatus enSeekCallback(const FLAC__StreamEncoder* encoder, FLAC__uint64 absolute_byte_offset, void* client_data)
		{
			en_data_t* t = (en_data_t*)client_data;
			auto ret = (t->seek_func) ? t->seek_func(absolute_byte_offset, t->ud) : t->seek(absolute_byte_offset);
			return ret != absolute_byte_offset ? FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR : FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
		}
		static FLAC__StreamEncoderTellStatus enTellCallback(const FLAC__StreamEncoder* encoder, FLAC__uint64* absolute_byte_offset, void* client_data)
		{
			en_data_t* t = (en_data_t*)client_data;
			*absolute_byte_offset = (t->tell_func) ? t->tell_func(t->ud) : t->tell();
			return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
		}
		static void enMetadataCallback(const FLAC__StreamEncoder* encoder, const FLAC__StreamMetadata* metadata, void* client_data)
		{
			en_data_t* t = (en_data_t*)client_data;

			return;
		}
		static void enProgressCallback(const FLAC__StreamEncoder* encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, uint32_t frames_written, uint32_t total_frames_estimate, void* client_data)
		{
			en_data_t* t = (en_data_t*)client_data;
			return;
		}
		struct enflac_t
		{
			flac_loader* flac;
			FLAC__StreamEncoder* _encoder;
			FLAC__int32* pcm;
			int pcmsize;
			int channels;
			int format;
			int norm_double = 1;
			int norm_float = 1;
			bool add_clipping = true;
		};
		// func返回nullptr或长度小于0则跳出编码
		static size_t encoder(encoder_info_t* ep)
		{
			if (!ep->handle || !ep || (!ep->file_path) || !ep->data || !ep->data_size)
			{
				return -1;
			}
			flac_loader* flac = ((flac_de_t*)ep->handle)->lib();
			if (!flac)return -2;
			//return wav2flac(ep, ep->file_src, ep->file_path, flac);
			en_data_t out_stream;
			//create the flac encoder
			FLAC__StreamEncoder* _encoder = flac->FLAC__stream_encoder_new();
			flac->FLAC__stream_encoder_set_verify(_encoder, true);
			flac->FLAC__stream_encoder_set_compression_level(_encoder, ep->compression_level);
			flac->FLAC__stream_encoder_set_channels(_encoder, ep->channels);
			flac->FLAC__stream_encoder_set_bits_per_sample(_encoder, ep->bits_per_sample);
			flac->FLAC__stream_encoder_set_sample_rate(_encoder, ep->sample_rate);
			//unknown total samples
			flac->FLAC__stream_encoder_set_total_samples_estimate(_encoder, ep->total_samples);

			int64_t total_bytes_read = 0;
			{
				mfile_t fmv;
				if (ep->file_path && fmv.open_m(ep->file_path, false))// && fmv.createfile(ep->file_path))
				{
					auto md = fmv.map(ep->data_size, 0);
					auto ks = flac->FLAC__stream_encoder_get_total_samples_estimate(_encoder);
					out_stream.write_func = fwrite_func;
					out_stream.seek_func = fseek_func;
					out_stream.tell_func = ftell_func;
					out_stream.ud = &fmv;
					flac->FLAC__stream_encoder_init_stream(_encoder, enWriteCallback, enSeekCallback, enTellCallback, enMetadataCallback, &out_stream);
				}
				else if (ep->file_path)
				{
					flac->FLAC__stream_encoder_init_file(_encoder, ep->file_path, progress_callback, NULL);
				}
				int bytes_read = 0;
				const int pcmsize = flac->FLAC__stream_encoder_get_blocksize(_encoder);
				std::vector<FLAC__int32> pcmv(pcmsize);
				auto pcm = pcmv.data();
				// { flac_write_i2flac,flac_write_s2flac,flac_write_f2flac,flac_write_d2flac };
				if (ep->data && ep->data_size > 0)
				{
					// dst_format支持8 16 24三种
					enflac_t et = { flac, _encoder, pcm, (int)(pcmsize * sizeof(int)), ep->channels, ep->dst_format };
					if (ep->src_format)
					{
						if (ep->bits_per_sample == 32)
						{
							bytes_read = flac_write_f2flac(&et, (float*)ep->data, ep->data_size / sizeof(float));
						}
						else {
							bytes_read = flac_write_d2flac(&et, (double*)ep->data, ep->data_size / sizeof(double));
						}
					}
					else
					{
						if (ep->bits_per_sample == 16)
						{
							bytes_read = flac_write_s2flac(&et, (short*)ep->data, ep->data_size / sizeof(short));
						}
						else {
							bytes_read = flac_write_i2flac(&et, (int*)ep->data, ep->data_size / sizeof(int));
						}
					}
					total_bytes_read += bytes_read;
				}
				//finish encoding the current flac file
				flac->FLAC__stream_encoder_finish(_encoder);
				flac->FLAC__stream_encoder_delete(_encoder);
				auto e = fmv.last_size();
				fmv.flush(0, e);
				fmv.ftruncate_m(e);
			}
			return total_bytes_read > 0 ? total_bytes_read : 0;
		}


		static size_t flac_write_s2flac(enflac_t* e, const short* ptr, size_t len)
		{
			void (*convert) (const short*, int32_t*, int);
			int bufferlen, writecount, thiswrite;
			size_t	total = 0;
			int32_t* buffer = e->pcm;
			switch (e->format)
			{
			case 1:
				convert = s2flac8_array;
				break;
			case 3:
				convert = s2flac24_array;
				break;
			default:
				convert = s2flac16_array;
				break;
			};
			bufferlen = e->pcmsize / (sizeof(int32_t) * e->channels);
			bufferlen *= e->channels;
			while (len > 0)
			{
				writecount = (len >= bufferlen) ? bufferlen : (int)len;
				convert(ptr + total, buffer, writecount);
				if (FLAC__stream_encoder_process_interleaved(e->_encoder, buffer, writecount / e->channels))
					thiswrite = writecount;
				else
					break;
				total += thiswrite;
				if (thiswrite < writecount)
					break;
				len -= thiswrite;
			}
			return total;
		}
		static size_t flac_write_i2flac(enflac_t* e, const int* ptr, size_t len)
		{
			void (*convert) (const int*, int32_t*, int);
			int bufferlen, writecount, thiswrite;
			size_t	total = 0;
			int32_t* buffer = e->pcm;
			switch (e->format)
			{
			case 1:
				convert = i2flac8_array;
				break;
			case 2:
				convert = i2flac16_array;
				break;
			default:
				convert = i2flac24_array;
				break;
			};
			bufferlen = e->pcmsize / (sizeof(int32_t) * e->channels);
			bufferlen *= e->channels;
			while (len > 0)
			{
				writecount = (len >= bufferlen) ? bufferlen : (int)len;
				convert(ptr + total, buffer, writecount);
				if (FLAC__stream_encoder_process_interleaved(e->_encoder, buffer, writecount / e->channels))
					thiswrite = writecount;
				else
					break;
				total += thiswrite;
				if (thiswrite < writecount)
					break;
				len -= thiswrite;
			}
			return total;
		}

		static size_t flac_write_f2flac(enflac_t* e, const float* ptr, size_t len)
		{
			void (*convert) (const float*, int32_t*, int, int);
			int bufferlen, writecount, thiswrite;
			size_t	total = 0;
			int32_t* buffer = e->pcm;

			switch (e->format)
			{
			case 1:
				convert = (e->add_clipping) ? f2flac8_clip_array : f2flac8_array;
				break;
			case 2:
				convert = (e->add_clipping) ? f2flac16_clip_array : f2flac16_array;
				break;
			default:
				convert = (e->add_clipping) ? f2flac24_clip_array : f2flac24_array;
				break;
			};

			bufferlen = e->pcmsize / (sizeof(int32_t) * e->channels);
			bufferlen *= e->channels;

			while (len > 0)
			{
				writecount = (len >= bufferlen) ? bufferlen : (int)len;
				convert(ptr + total, buffer, writecount, e->norm_float);
				if (FLAC__stream_encoder_process_interleaved(e->_encoder, buffer, writecount / e->channels))
					thiswrite = writecount;
				else
					break;
				total += thiswrite;
				if (thiswrite < writecount)
					break;

				len -= thiswrite;
			};

			return total;
		} /* flac_write_f2flac */


		static size_t flac_write_d2flac(enflac_t* e, const double* ptr, size_t len)
		{
			void (*convert) (const double*, int32_t*, int, int);
			int bufferlen, writecount, thiswrite;
			size_t	total = 0;
			int32_t* buffer = e->pcm;

			switch (e->format)
			{
			case 1:
				convert = (e->add_clipping) ? d2flac8_clip_array : d2flac8_array;
				break;
			case 2:
				convert = (e->add_clipping) ? d2flac16_clip_array : d2flac16_array;
				break;
			default:
				convert = (e->add_clipping) ? d2flac24_clip_array : d2flac24_array;
				break;
			};

			bufferlen = e->pcmsize / (sizeof(int32_t) * e->channels);
			bufferlen *= e->channels;

			while (len > 0)
			{
				writecount = (len >= bufferlen) ? bufferlen : (int)len;
				convert(ptr + total, buffer, writecount, e->norm_double);
				if (FLAC__stream_encoder_process_interleaved(e->_encoder, buffer, writecount / e->channels))
					thiswrite = writecount;
				else
					break;
				total += thiswrite;
				if (thiswrite < writecount)
					break;

				len -= thiswrite;
			};

			return total;
		} /* flac_write_d2flac */

	private:
		static int fwrite_func(const char* data, size_t bytes, uint32_t samples, uint32_t current_frame, void* ud)
		{
			mfile_t* fp = (mfile_t*)ud;
			int ret = fp->write(data, bytes);
			return ret;
		}
		static int fseek_func(uint64_t pos, void* ud)
		{
			auto fp = (mfile_t*)ud;
			return fp->seek(pos);
		}
		static uint64_t ftell_func(void* ud)
		{
			auto fp = (mfile_t*)ud;
			return fp->tell();
		}
	public:

#define READSIZE 4096
		class wav2flac_t
		{
		public:
			unsigned total_samples = 0; /* can use a 32-bit number due to WAVE size limitations */
			FLAC__byte* _buffer = 0;// [READSIZE/*samples*/ * 2/*bytes_per_sample*/ * 2/*channels*/] ; /* we read the WAVE data into here */
			FLAC__int32* _pcm = 0;// [READSIZE/*samples*/ * 2/*channels*/] ;
			std::vector<int> data;
		public:
			wav2flac_t()
			{
				data.resize(READSIZE * 2 * 2 + READSIZE * 2);
				_buffer = (FLAC__byte*)data.data();
				_pcm = (FLAC__int32*)(_buffer + READSIZE * 2 * 2);
			}

			~wav2flac_t()
			{
			}
		private:

		};


		static int wav2flac(encoder_info_t* ep, const char* wavfn, const char* outputfn, flac_loader* flac)
		{
			FLAC__bool ok = true;
			FLAC__StreamEncoder* encoder = 0;
			FLAC__StreamEncoderInitStatus init_status = {};
			FLAC__StreamMetadata* metadata[2] = {};
			FLAC__StreamMetadata_VorbisComment_Entry entry = {};
			FILE* fin = 0;
			unsigned sample_rate = 0;
			unsigned channels = 0;
			unsigned bps = 0;
			wav2flac_t wft[1] = {};

			if ((fin = fopen(wavfn, "rb")) == NULL) {
				fprintf(stderr, "ERROR: opening %s for output\n", wavfn);
				return 1;
			}
			char* buffer = (char*)wft->_buffer;
			/* read wav header and validate it */
			if (
				fread(buffer, 1, 44, fin) != 44 ||
				memcmp(buffer, "RIFF", 4) ||
				memcmp(buffer + 8, "WAVEfmt \020\000\000\000\001\000\002\000", 16) ||
				memcmp(buffer + 32, "\004\000\020\000data", 8)
				) {
				fprintf(stderr, "ERROR: invalid/unsupported WAVE file, only 16bps stereo WAVE in canonical form allowed\n");
				fclose(fin);
				return 1;
			}
			uint32_t* sr = (uint32_t*)&buffer[24];
			sample_rate = *sr;// ((((((unsigned)buffer[27] << 8) | buffer[26]) << 8) | buffer[25]) << 8) | buffer[24];
			channels = *((short*)&buffer[0x16]);
			bps = *((short*)&buffer[0x22]);
			uint32_t* ts = (uint32_t*)&buffer[40];//0x28
			wft->total_samples = *ts / (channels * 2);// (((((((unsigned)buffer[43] << 8) | buffer[42]) << 8) | buffer[41]) << 8) | buffer[40]) / 4;

			/* allocate the encoder */
			if ((encoder = flac->FLAC__stream_encoder_new()) == NULL) {
				fprintf(stderr, "ERROR: allocating encoder\n");
				fclose(fin);
				return 1;
			}

			ok &= flac->FLAC__stream_encoder_set_verify(encoder, true);
			ok &= flac->FLAC__stream_encoder_set_compression_level(encoder, ep->compression_level);
			ok &= flac->FLAC__stream_encoder_set_channels(encoder, channels);
			ok &= flac->FLAC__stream_encoder_set_bits_per_sample(encoder, bps);
			ok &= flac->FLAC__stream_encoder_set_sample_rate(encoder, sample_rate);
			ok &= flac->FLAC__stream_encoder_set_blocksize(encoder, ep->blocksize > 0 ? ep->blocksize : 4096);
			ok &= flac->FLAC__stream_encoder_set_total_samples_estimate(encoder, wft->total_samples);

			/* now add some metadata; we'll add some tags and a padding block */
			if (ok) {
				if (
					(metadata[0] = flac->FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) == NULL ||
					(metadata[1] = flac->FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL ||
					/* there are many tag (vorbiscomment) functions but these are convenient for this particular use: */
					!flac->FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ARTIST", "Some Artist") ||
					!flac->FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, /*copy=*/false) || /* copy=false: let metadata object take control of entry's allocated string */
					!flac->FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "YEAR", "1984") ||
					!flac->FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, /*copy=*/false)
					) {
					fprintf(stderr, "ERROR: out of memory or tag error\n");
					ok = false;
				}

				metadata[1]->length = 1234; /* set the padding length */

				ok = flac->FLAC__stream_encoder_set_metadata(encoder, metadata, 2);
			}

			/* initialize encoder */
			if (ok) {
				init_status = flac->FLAC__stream_encoder_init_file(encoder, outputfn, progress_callback, wft);
				if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
					fprintf(stderr, "ERROR: initializing encoder: %s\n", flac->FLAC__StreamEncoderInitStatusString[init_status]);
					ok = false;
				}
			}

			/* read blocks of samples from WAVE file and feed to encoder */
			if (ok) {
				size_t left = (size_t)wft->total_samples;
				while (ok && left) {
					size_t need = (left > READSIZE ? (size_t)READSIZE : (size_t)left);
					if (fread(buffer, channels * (bps / 8), need, fin) != need) {
						fprintf(stderr, "ERROR: reading from WAVE file\n");
						ok = false;
					}
					else {
						/* convert the packed little-endian 16-bit PCM samples from WAVE into an interleaved FLAC__int32 buffer for libFLAC */
						//size_t i;
						//for (i = 0; i < need * channels; i++) {
						//	/* inefficient but simple and works on big- or little-endian machines */
						//	wft->pcm[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)buffer[2 * i + 1] << 8) | (FLAC__int16)buffer[2 * i]);
						//}
						short* bs = (short*)buffer;
						for (int i = 0; i < need * channels; i++) {
							/* inefficient but simple and works on big- or little-endian machines */
							wft->_pcm[i] = bs[i];// (FLAC__int32)(((FLAC__int16)(FLAC__int8)buffer[2 * i + 1] << 8) | (FLAC__int16)buffer[2 * i]);
						}
						/* feed samples to encoder */
						ok = flac->FLAC__stream_encoder_process_interleaved(encoder, wft->_pcm, need);
					}
					left -= need;
				}
			}

			ok &= flac->FLAC__stream_encoder_finish(encoder);

			fprintf(stderr, "encoding: %s\n", ok ? "succeeded" : "FAILED");
			fprintf(stderr, "   state: %s\n", flac->FLAC__StreamEncoderStateString[flac->FLAC__stream_encoder_get_state(encoder)]);
			fprintf(stderr, "   state: %s\n", flac->FLAC__StreamEncoderStateString[flac->FLAC__stream_encoder_get_verify_decoder_state(encoder)]);
			/* now that encoding is finished, the metadata can be freed */
			flac->FLAC__metadata_object_delete(metadata[0]);
			flac->FLAC__metadata_object_delete(metadata[1]);

			flac->FLAC__stream_encoder_delete(encoder);
			fclose(fin);

			return 0;
		}

		static void progress_callback(const FLAC__StreamEncoder* encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void* client_data)
		{
			(void)encoder;
			wav2flac_t* wft = (wav2flac_t*)client_data;
			//fprintf(stderr, "wrote %" PRIu64 " bytes, %" PRIu64 "/%u samples, %u/%u frames\n", bytes_written, samples_written, wft->total_samples, frames_written, total_frames_estimate);
		}


		int convertWavToFlac(const char* wave_file, const char* flac_file, int split_interval_seconds, char** out_flac_files) {
			FILE* fin;

			static FLAC__byte buffer[READSIZE/*samples*/ * 2/*bytes_per_sample*/ * 2/*channels*/];
			static FLAC__int32 pcm[READSIZE/*samples*/ * 2/*channels*/];
			int k = sizeof(short);
			if ((fin = fopen(wave_file, "rb")) == NULL) {
				fprintf(stderr, "ERROR: opening %s for output\n", wave_file);
				return 1;
			}

			// read wav header and validate it, note this will most likely fail for WAVE files not created by Apple
			if (fread(buffer, 1, 44, fin) != 44 ||
				memcmp(buffer, "RIFF", 4) /*||
				memcmp(buffer + 36, "FLLR", 4)*/) {
				fprintf(stderr, "ERROR: invalid/unsupported WAVE file\n");
				fclose(fin);
				return 1;
			}
			unsigned num_channels = ((unsigned)buffer[23] << 8) | buffer[22];;
			unsigned sample_rate = ((((((unsigned)buffer[27] << 8) | buffer[26]) << 8) | buffer[25]) << 8) | buffer[24];
			//unsigned byte_rate = ((((((unsigned)buffer[31] << 8) | buffer[30]) << 8) | buffer[29]) << 8) | buffer[28];
			//unsigned block_align = ((unsigned)buffer[33] << 8) | buffer[32];
			unsigned bps = ((unsigned)buffer[35] << 8) | buffer[34];

			//Apple puts the number of filler bytes in the 2 bytes following FLLR in the filler chunk
			//get the int value of the hex
			unsigned filler_byte_count = ((unsigned)buffer[41] << 8) | buffer[40];
			//swallow the filler bytes, exiting if there were not enough
			if (fread(buffer, 1, filler_byte_count, fin) != filler_byte_count) {
				fprintf(stderr, "ERROR: invalid number of filler bytes\n");
				return 1;
			}
			//swallow the beginning of the data chunk, i.e. the word 'data'
			unsigned data_subchunk_size = 0;
			if (fread(buffer, 1, 8, fin) != 8 || memcmp(buffer, "data", 4)) {
				fprintf(stderr, "ERROR: bad data start section\n");
				return 1;
			}
			else {
				//Subchunk2Size == NumSamples * NumChannels * BitsPerSample/8
				data_subchunk_size = ((((((unsigned)buffer[7] << 8) | buffer[6]) << 8) | buffer[5]) << 8) | buffer[4];
			}

			//create the flac encoder
			FLAC__StreamEncoder* encoder = flac->FLAC__stream_encoder_new();
			flac->FLAC__stream_encoder_set_verify(encoder, true);
			flac->FLAC__stream_encoder_set_compression_level(encoder, 5);
			flac->FLAC__stream_encoder_set_channels(encoder, num_channels);
			flac->FLAC__stream_encoder_set_bits_per_sample(encoder, bps);
			flac->FLAC__stream_encoder_set_sample_rate(encoder, sample_rate);
			//unknown total samples
			flac->FLAC__stream_encoder_set_total_samples_estimate(encoder, 0);
			char* next_flac_file = (char*)malloc(sizeof(char) * 1024);
			sprintf(next_flac_file, "%s.flac", flac_file);
			fprintf(stderr, "writing to new flac file %s\n", next_flac_file);
			flac->FLAC__stream_encoder_init_file(encoder, next_flac_file, progress_callback, NULL);

			long total_bytes_read = 0;
			int did_split_at_interval[1024];
			for (int i = 0; i < 1024; i++) {
				did_split_at_interval[i] = 0;
			}

			//read the wav file data chunk until we reach the end of the file.
			size_t bytes_read = 0;
			size_t need = (size_t)READSIZE;
			int flac_file_index = 0;
			while ((bytes_read = fread(buffer, num_channels * (bps / 8), need, fin)) != 0) {
				/* convert the packed little-endian 16-bit PCM samples from WAVE into an interleaved FLAC__int32 buffer for libFLAC */
				size_t i;
				for (i = 0; i < bytes_read * num_channels; i++) {
					/* inefficient but simple and works on big- or little-endian machines */
					pcm[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)buffer[2 * i + 1] << 8) | (FLAC__int16)buffer[2 * i]);
				}
				/* feed samples to encoder */
				flac->FLAC__stream_encoder_process_interleaved(encoder, pcm, bytes_read);
				total_bytes_read += bytes_read;

				if (split_interval_seconds > 0) {
					double elapsed_time_seconds = (total_bytes_read * 16) / (bps * sample_rate);
					int interval = elapsed_time_seconds / split_interval_seconds;
					if (interval > 0) {
						if (!did_split_at_interval[interval - 1]) {
							//finish encoding the current flac file
							flac->FLAC__stream_encoder_finish(encoder);
							flac->FLAC__stream_encoder_delete(encoder);

							//add the flac file to the out_flac_files output parameter
							*(out_flac_files + flac_file_index) = next_flac_file;
							flac_file_index += 1;

							//get a new flac file name
							//free(next_flac_file);
							next_flac_file = (char*)malloc(sizeof(char) * 1024);
							sprintf(next_flac_file, "%s_%d.flac", flac_file, interval);
							fprintf(stderr, "writing to new flac file %s\n", next_flac_file);

							//create a new encoder
							encoder = flac->FLAC__stream_encoder_new();
							flac->FLAC__stream_encoder_set_verify(encoder, true);
							flac->FLAC__stream_encoder_set_compression_level(encoder, 5);
							flac->FLAC__stream_encoder_set_channels(encoder, num_channels);
							flac->FLAC__stream_encoder_set_bits_per_sample(encoder, bps);
							flac->FLAC__stream_encoder_set_sample_rate(encoder, sample_rate);
							flac->FLAC__stream_encoder_set_total_samples_estimate(encoder, 0);
							flac->FLAC__stream_encoder_init_file(encoder, next_flac_file, progress_callback, NULL);

							//mark the interval as split
							did_split_at_interval[interval - 1] = 1;
						}
					}
				}
			}
			fprintf(stderr, "total bytes read: %ld\nbits per sample: %d\nsample rate: %d\n", total_bytes_read, bps, sample_rate);

			//*(out_flac_files + flac_file_index) = next_flac_file;

			//cleanup
			flac->FLAC__stream_encoder_finish(encoder);
			flac->FLAC__stream_encoder_delete(encoder);
			fclose(fin);

			return 0;
		}

		static void progress_callback0(const FLAC__StreamEncoder* encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written
			, unsigned frames_written, unsigned total_frames_estimate, void* client_data)
		{
			//fprintf(stderr, "wrote %llu bytes\n", bytes_written);
		}
	};

#endif // !_NO_FLAC_EN_

	void get_info_ty(void* music, audio_spec_t* p)
	{
		if (music)
		{
			music_de_t* pw = (music_de_t*)music;
			p->total_samples = pw->total_samples;
			p->channels = pw->spec.channels;
			p->sample_rate = pw->spec.freq;
			p->format = pw->spec.format;
			p->bits_per_sample = pw->bits_per_sample;
		}
	}

#ifndef NO_OGGS


	/*
	ogg解码器
	*/
	class ogg_decoder
	{
	public:
		std::vector<float> _data;
		std::vector<int16_t> _data16;
		stb_vorbis* v = 0;
		int64_t size = 0;
		int error_r = 0;
		bool gtype = 0;// 0获取float
	public:
		ogg_decoder();
		~ogg_decoder();
		void open_data(char* data, int len);
		int get_data(void* data, int bytes);
		int get_data16(void* data, int bytes);
		int seek_start();
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
			stb_vorbis_stream_length_in_samples(v);
			size = v->total_samples * v->channels;
		}
	}

	int ogg_decoder::get_data(void* data, int bytes)
	{
		if (!data || !v)return 0;
		bytes /= sizeof(float);
		int dn = 0;
		if (_data.size() < bytes)
		{
			_data.resize(bytes);
		}
		auto buf = _data.data();
		do {
			if (dn < bytes)
			{
				auto kr = stb_vorbis_get_samples_float_interleaved((stb_vorbis*)v, v->channels, buf, bytes - dn) * v->channels;
				dn += kr;
				buf += kr;
				if (kr == 0)break;
			}
			else { break; }
		} while (1);
		if (dn > 0)
		{
			memcpy(data, _data.data(), dn * sizeof(float));
		}
		return dn * sizeof(float);
	}
	int ogg_decoder::get_data16(void* data, int bytes)
	{
		if (!data || !v)return 0;
		bytes /= sizeof(int16_t);
		int dn = 0;
		if (_data16.size() < bytes)
		{
			_data16.resize(bytes);
		}
		auto buf = _data16.data();
		do {
			if (dn < bytes)
			{
				auto kr = stb_vorbis_get_frame_short_interleaved((stb_vorbis*)v, v->channels, buf, bytes - dn) * v->channels;
				dn += kr;
				buf += kr;
				if (kr == 0)break;
			}
			else { break; }
		} while (1);
		if (dn > 0)
		{
			memcpy(data, _data16.data(), dn * sizeof(int16_t));
		}
		return dn * sizeof(int16_t);
	}
	int ogg_decoder::seek_start()
	{
		return stb_vorbis_seek_start(v);
	}
	void* ogg_open_memory(const void* data, int len, void* handle) {
		ogg_decoder* p = 0;
		if (data && len > 0) {
			p = new ogg_decoder();
			p->gtype = 0;
			p->open_data((char*)data, len);
			if (!p->v)
			{
				delete p; p = 0;
			}
		}
		return p;
	}
	void get_info_ogg(void* music, audio_spec_t* p)
	{
		if (music && p)
		{
			ogg_decoder* pw = (ogg_decoder*)music;
			p->total_samples = pw->v->total_samples;
			p->channels = pw->v->channels;
			p->sample_rate = pw->v->sample_rate;
			p->format = pw->gtype ? 0 : 2;
			p->bits_per_sample = pw->gtype ? 16 : 32;// ogg解码只有int16或float
		}
	}
	char* ogg_get_buffer(void* ptr) {
		auto p = (ogg_decoder*)ptr;
		char* r = 0;
		if (p)
		{
			r = p->gtype ? (char*)p->_data16.data() : (char*)p->_data.data();
		}
		return r;
	}
	int ogg_get_audio(void* ptr, void* data, int bytes) {
		auto p = (ogg_decoder*)ptr;
		int r = 0;
		if (p)
		{
			r = p->gtype ? p->get_data16(data, bytes) : p->get_data(data, bytes);
		}
		return r;
	}
	void free_ogg(void* p) {
		if (p) {
			delete (ogg_decoder*)p;
		}
	}


#endif // !NO_OGGS

#ifndef NO_MP3_MPG123

	//#include <stdio.h>      // For SEEK_SET
	//#include <assert.h>

#ifndef ssize_t
#define ssize_t size_t
#endif
	typedef struct {

		void* handle;

		int (*mpg123_close)(mpg123_handle* mh);
		void (*mpg123_delete)(mpg123_handle* mh);
		void (*mpg123_exit)(void);
		int (*mpg123_format)(mpg123_handle* mh, long rate, int channels, int encodings);
		int (*mpg123_format_none)(mpg123_handle* mh);
		int (*mpg123_getformat)(mpg123_handle* mh, long* rate, int* channels, int* encoding);
		int (*mpg123_init)(void);
		mpg123_handle* (*mpg123_new)(const char* decoder, int* error);
		int (*mpg123_open_handle)(mpg123_handle* mh, void* iohandle);
		const char* (*mpg123_plain_strerror)(int errcode);
		void (*mpg123_rates)(const long** list, size_t* number);
		int (*mpg123_read)(mpg123_handle* mh, void* outmemory, size_t outmemsize, size_t* done);
		int (*mpg123_replace_reader_handle)(mpg123_handle* mh, mpg123_ssize_t(*r_read) (void*, void*, size_t), off_t(*r_lseek)(void*, off_t, int), void (*cleanup)(void*));
		off_t(*mpg123_seek)(mpg123_handle* mh, off_t sampleoff, int whence);
		const char* (*mpg123_strerror)(mpg123_handle* mh);
		int (*mpg123_info2)(mpg123_handle* mh, struct mpg123_frameinfo2* mi);
		off_t(*mpg123_length)(mpg123_handle* mh);
		double (*mpg123_tpf)(mpg123_handle* mh);
	} mpg123_loader;

	class mpg123_t
	{
	private:

		mpg123_loader mpg123 = {
		   0, nullptr
		};

	public:
		mpg123_t()
		{
			loadlib();
		}

		~mpg123_t()
		{
			MPG123_Close();
		}

		void MPG123_Close(void)
		{
			if (mpg123.mpg123_exit)
				mpg123.mpg123_exit();
		}

	private:
		int loadlib()
		{
			static const char* ccfn[] = {
				"mpg123_close",
				"mpg123_delete",
				"mpg123_exit",
				"mpg123_format",
				"mpg123_format_none",
				"mpg123_getformat",
				"mpg123_init",
				"mpg123_new",
				"mpg123_open_handle",
				"mpg123_plain_strerror",
				"mpg123_rates",
				"mpg123_read",
				"mpg123_replace_reader_handle",
				"mpg123_seek",
				"mpg123_strerror",
				"mpg123_info2",
				"mpg123_length",
				"mpg123_tpf",
			};
			if (mpg123.handle)
			{
				return 0;
			}

#ifndef MPG_GETCB
#define MPG_GETCB(n) mpg123. n=n
#endif // !MPG_GETCB
			MPG_GETCB(mpg123_close);
			MPG_GETCB(mpg123_delete);
			MPG_GETCB(mpg123_exit);
			MPG_GETCB(mpg123_format);
			MPG_GETCB(mpg123_format_none);
			MPG_GETCB(mpg123_getformat);
			MPG_GETCB(mpg123_init);
			MPG_GETCB(mpg123_new);
			MPG_GETCB(mpg123_open_handle);
			MPG_GETCB(mpg123_plain_strerror);
			MPG_GETCB(mpg123_rates);
			MPG_GETCB(mpg123_read);
			MPG_GETCB(mpg123_replace_reader_handle);
			MPG_GETCB(mpg123_seek);
			MPG_GETCB(mpg123_strerror);
			MPG_GETCB(mpg123_info2);
			MPG_GETCB(mpg123_length);
			MPG_GETCB(mpg123_tpf);
			mpg123.handle = (void*)1;
			return MPG123_Open();
		}

	private:

		static int mpg123_format_to_sdl(int fmt)
		{
			switch (fmt)
			{
			case MPG123_ENC_SIGNED_8:       return 0;// SDL_AUDIO_S8;
			case MPG123_ENC_UNSIGNED_8:     return 0;// SDL_AUDIO_U8;
			case MPG123_ENC_SIGNED_16:      return 0;// SDL_AUDIO_S16;
			case MPG123_ENC_UNSIGNED_16:    return 0;// SDL_AUDIO_S16;
			case MPG123_ENC_SIGNED_32:      return 1;// SDL_AUDIO_S32;
			case MPG123_ENC_FLOAT_32:       return 2;// SDL_AUDIO_F32;
			default:                        return 0;
			}
		}
		static int mpg123_format_to_bit(int fmt)
		{
			switch (fmt)
			{
			case MPG123_ENC_SIGNED_8:       return 8;
			case MPG123_ENC_UNSIGNED_8:     return 8;
			case MPG123_ENC_SIGNED_16:      return 16;
			case MPG123_ENC_UNSIGNED_16:    return 16;
			case MPG123_ENC_SIGNED_32:      return 32;
			case MPG123_ENC_FLOAT_32:       return 32;
			default:                        return 0;
			}
		}

		/*
		static const char *mpg123_format_str(int fmt)
		{
			switch (fmt)
			{
		#define f(x) case x: return #x;
				f(MPG123_ENC_UNSIGNED_8)
				f(MPG123_ENC_UNSIGNED_16)
				f(MPG123_ENC_SIGNED_8)
				f(MPG123_ENC_SIGNED_16)
				f(MPG123_ENC_SIGNED_32)
				f(MPG123_ENC_FLOAT_32)
		#undef f
			}
			return "unknown";
		}
		*/

		char const* mpg_err(mpg123_handle* mpg, int result)
		{
			char const* err = "unknown error";

			if (mpg && result == MPG123_ERR) {
				err = mpg123.mpg123_strerror(mpg);
			}
			else {
				err = mpg123.mpg123_plain_strerror(result);
			}
			return err;
		}

		/* we're gonna override mpg123's I/O with these wrappers for RWops */
		static mpg123_ssize_t rwops_read(void* p, void* dst, size_t n)
		{
			return (mpg123_ssize_t)RWread((rw_t*)p, dst, 1, n);
		}

		static off_t rwops_seek(void* p, off_t offset, int whence)
		{
			return (off_t)RWseek((rw_t*)p, (int64_t)offset, whence);
		}

		static void rwops_cleanup(void* p)
		{
			(void)p;
			/* do nothing, we will free the file later */
		}


		int MPG123_Open()
		{
			if (mpg123.mpg123_init() != MPG123_OK) {
				printf("mpg123_init() failed");
				return -1;
			}
			return 0;
		}

		void* MPG123_CreateFromRW(rw_t* src)
		{
			music_de_t* music;
			int result;
			const long* rates;
			size_t i, num_rates;

			music = new music_de_t();
			if (!music) {
				return NULL;
			}
			music->src = src;
			music->ctx = this;
			music->_decoder = mpg123.mpg123_new(0, &result);
			if (result != MPG123_OK) {
				free_m(music);
				printf("mpg123_new failed");
				return NULL;
			}

			result = mpg123.mpg123_replace_reader_handle(
				(mpg123_handle*)music->_decoder,
				rwops_read, rwops_seek, rwops_cleanup
			);
			if (result != MPG123_OK) {
				free_m(music);
				printf("mpg123_replace_reader_handle: %s", mpg_err((mpg123_handle*)music->_decoder, result));
				return NULL;
			}

			result = mpg123.mpg123_format_none((mpg123_handle*)music->_decoder);
			if (result != MPG123_OK) {
				free_m(music);
				printf("mpg123_format_none: %s", mpg_err((mpg123_handle*)music->_decoder, result));
				return NULL;
			}

			mpg123.mpg123_rates(&rates, &num_rates);
			for (i = 0; i < num_rates; ++i) {
				const int channels = (MPG123_MONO | MPG123_STEREO);
				const int formats = (MPG123_ENC_SIGNED_8 |
					MPG123_ENC_UNSIGNED_8 |
					MPG123_ENC_SIGNED_16 |
					MPG123_ENC_UNSIGNED_16 |
					MPG123_ENC_SIGNED_32 |
					MPG123_ENC_FLOAT_32);

				mpg123.mpg123_format((mpg123_handle*)music->_decoder, rates[i], channels, formats);
			}

			result = mpg123.mpg123_open_handle((mpg123_handle*)music->_decoder, music->src);
			if (result != MPG123_OK) {
				free_m(music);
				printf("mpg123_open_handle: %s", mpg_err((mpg123_handle*)music->_decoder, result));
				return NULL;
			}
			MPG123_getinfo(music);
			return music;
		}
		int MPG123_getinfo(music_de_t* music)
		{
			long rate;
			int channels, encoding, format;
			off_t offset = 0;
			size_t amount = 0;
			unsigned char buf[8192] = {};
			int result;// = mpg123.mpg123_read((mpg123_handle*)music->_decoder, buf, 8192, &amount);
			result = mpg123.mpg123_getformat((mpg123_handle*)music->_decoder, &rate, &channels, &encoding);
			if (result != MPG123_OK) {
				printf("mpg123_getformat: %s", mpg_err((mpg123_handle*)music->_decoder, result));
				return -1;
			}
			/*printf("MPG123 format: %s, channels = %d, rate = %ld\n", mpg123_format_str(encoding), channels, rate);*/

			format = mpg123_format_to_sdl(encoding);
			assert(format != -1);
			music->set_spec(rate, channels, format);
			struct mpg123_frameinfo2 mi[1] = {};
			if (mpg123.mpg123_info2)
				result = mpg123.mpg123_info2((mpg123_handle*)music->_decoder, mi);
			offset = mpg123.mpg123_length((mpg123_handle*)music->_decoder);
			if (offset < 0)
			{
				offset = mpg123.mpg123_seek((mpg123_handle*)music->_decoder, 0, SEEK_END);
				mpg123.mpg123_seek((mpg123_handle*)music->_decoder, 0, SEEK_SET);
			}
			if (!music->total_samples)
			{
				music->total_samples = offset;// music->samples;
			}
			double t = music->total_samples; t /= music->spec.freq;
			music->seconds = t;
			music->bits_per_sample = mpg123_format_to_bit(encoding);

			/* Just assume 16-bit 2 channel audio for now */
			auto buffer_size = music->samples * music->bits_per_sample * 0.5 * channels;
			music->_buffer.resize(buffer_size);
			//music->stream.resize(buffer_size);
			return 0;
		}

		/* read some mp3 stream data and convert it for output */
		int MPG123_GetSome(void* context, void* data, int bytes, bool* done)
		{
			music_de_t* music = (music_de_t*)context;
			int filled, result;
			size_t amount = 0;
			auto len = glm::clamp(bytes, 0, (int)music->_buffer.size());
			do
			{
				if (len <= 0)
					break;
				if (music->isdone) {
					/* All done */
					if (done)
						*done = true;
					return amount;
				}
				result = mpg123.mpg123_read((mpg123_handle*)music->_decoder, music->_buffer.data(), len, &amount);
				switch (result) {
				case MPG123_OK:
					memcpy(data, music->_buffer.data(), amount);
					break;
				case MPG123_NEW_FORMAT:
					//MPG123_getinfo(music);
					printf("MPG123_NEW_FORMAT\n");
					break;
				case MPG123_DONE:
					music->isdone = true;
					break;
				default:
					printf("mpg123_read: %s", mpg_err((mpg123_handle*)music->_decoder, result));
					return amount;
				}
			} while (0);
			return amount;
		}
		int MPG123_GetAudio(void* context, void* data, int bytes)
		{
			music_de_t* music = (music_de_t*)context;
			return MPG123_GetSome(context, data, bytes, nullptr);
		}

		int MPG123_Seek(void* context, double secs)
		{
			music_de_t* music = (music_de_t*)context;

			off_t offset = (off_t)(music->spec.freq * secs);
			if ((offset = mpg123.mpg123_seek((mpg123_handle*)music->_decoder, offset, SEEK_SET)) < 0) {
				return printf("mpg123_seek: %s", mpg_err((mpg123_handle*)music->_decoder, (int)-offset));
			}
			music->isdone = false;
			return 0;
		}
		void MPG123_free(mpg123_handle* _decoder)
		{
			if (_decoder) {
				mpg123.mpg123_close(_decoder);
				mpg123.mpg123_delete(_decoder);
			}
		}
	public:
		// 导出接口

		static void* load()
		{
			return new mpg123_t();
		}
		static void unload(void* ph)
		{
			mpg123_t* p = (mpg123_t*)ph;
			if (p)
			{
				delete p;
			}
		}
		static void* open_memory(const void* data, int len, void* handle)
		{
			mpg123_t* ctx = (mpg123_t*)handle;
			if (!len || !data || !ctx)
			{
				return nullptr;
			}
			rw_t* src = new rw_t((char*)data, len);
			auto p = ctx->MPG123_CreateFromRW(src);
			return p;
		}
		static int get_audio(void* music, void* data, int bytes)
		{
			int ret = 0;

			music_de_t* mp = (music_de_t*)music;
			mpg123_t* ctx = nullptr;
			if (mp && (ctx = (mpg123_t*)mp->ctx))
			{
				ret = ctx->MPG123_GetAudio(mp, data, bytes);
			}
			return ret;
		}

		/* Seek to a play position (in seconds) */
		static int seek(void* music, double position)
		{
			int ret = 0;
			music_de_t* mp = (music_de_t*)music;
			mpg123_t* ctx = nullptr;
			if (mp && (ctx = (mpg123_t*)mp->ctx))
			{
				ret = ctx->MPG123_Seek(music, position);
			}
			return ret;
		}
		static void free_m(void* context)
		{
			music_de_t* music = (music_de_t*)context;
			mpg123_t* ctx = nullptr;
			if (music && (ctx = (mpg123_t*)music->ctx))
			{
				ctx->MPG123_free((mpg123_handle*)music->_decoder);
				delete music;
			}
		}

	};

	coder_t* create_mpg123()
	{
		coder_t* ct = new coder_t();
		ct->tag = "mp3";
		memcpy(ct->tag2, "ID3", 3);
		ct->open_memory = mpg123_t::open_memory;
		ct->get_audio = mpg123_t::get_audio;
		ct->seek = mpg123_t::seek;
		ct->free_m = mpg123_t::free_m;
		ct->get_info = music_de_t::get_info;
		ct->load = mpg123_t::load;
		ct->unload = mpg123_t::unload;
		ct->handle = ct->load();
		return ct;
	}

	//Mix_MusicInterface Mix_MusicInterface_MPG123 =
	//{
	//	"MPG123",
	//	MIX_MUSIC_MPG123,
	//	MUS_MP3,
	//	SDL_FALSE,
	//	SDL_FALSE,
	//
	//	MPG123_Load,
	//	MPG123_Open,
	//	MPG123_CreateFromRW,
	//	NULL,   /* CreateFromFile */
	//	MPG123_SetVolume,
	//	MPG123_Play,
	//	NULL,   /* IsPlaying */
	//	MPG123_GetAudio,
	//	MPG123_Seek,
	//	NULL,   /* Pause */
	//	NULL,   /* Resume */
	//	NULL,   /* Stop */
	//	MPG123_Delete,
	//	MPG123_Close,
	//	MPG123_Unload
	//};

#endif /* NO_MP3_MPG123 */

	coder_t* create_flac_de()
	{
		coder_t* ct = new coder_t();
		ct->tag = "flac";
		memcpy(ct->tag2, "fLaC", 4);
		ct->open_memory = flac_de_t::open_memory;
		ct->get_audio = flac_de_t::get_audio;
		ct->seek = flac_de_t::seek;
		ct->free_m = flac_de_t::free_m;
		ct->load = flac_de_t::load;
		ct->unload = flac_de_t::unload;
		ct->handle = ct->load();
		ct->get_info = get_info_ty;
		ct->encoder = (encoder_func)flac_en_t::encoder;
		return ct;
	}
	coder_t* create_ogg_de()
	{
		coder_t* ct = new coder_t();
		ct->tag = "ogg";
		memcpy(ct->tag2, "OggS", 4);
		ct->open_memory = ogg_open_memory;
		ct->get_audio = ogg_get_audio;
		ct->free_m = free_ogg;
		ct->get_info = get_info_ogg;
		return ct;
	}
}
//!hz

coders_t* new_coders()
{
	coder_t* flac = hz::create_flac_de();
	coder_t* ogg = hz::create_ogg_de();
	coder_t* mp3 = hz::create_mpg123();
	auto p = new coders_t();
	if (p) {
		if (flac)
			p->codes.push_back(flac);
		if (ogg)
			p->codes.push_back(ogg);
		if (mp3)
			p->codes.push_back(mp3);
	}
	return p;
}
void free_coders(coders_t* p)
{
	if (p)delete p;
}

coders_t::coders_t()
{
}

coders_t::~coders_t()
{
	for (auto p : codes) {
		if (p->unload)
		{
			p->unload(p->handle);
		}
	}
	codes.clear();
}
int decoder_data(audio_data_t* p)
{
	int rc = 0;
	if (p && p->data && p->cap > 0 && p->code && p->desize <= p->cap && (!p->desize || p->desize != p->len))
	{
		auto mf = (hz::mfile_t*)p->mf;
		auto ptr = (hz::music_de_t*)p->ptr;
		rc = p->code->get_audio(p->ptr, (char*)p->data + p->desize, p->freq * 4);
		if (rc > 0)
		{
			p->desize += rc;
		}
		else {
			p->len = p->desize;
		}
	}
	return rc;
}
audio_data_t* new_audio_data(coders_t* pc, const std::string& fn)
{
	audio_data_t* r = 0;
	hz::mfile_t mf;
	auto md = mf.open_d(fn, true);
	if (md)
	{
		r = new_audio_data(pc, md, mf.size(), false);
		if (r)
		{
			r->mf = new hz::mfile_t();
			*((hz::mfile_t*)r->mf) = mf;
			mf.clear_ptr();
		}
	}
	return r;
}
audio_data_t* new_audio_data(coders_t* pc, const char* data, int len, bool iscopy)
{
	audio_data_t* r = 0;
	if (!pc || !data || len < 10)return r;
	coder_t* c = 0;
	std::string exfn;
	for (auto it : pc->codes)
	{
		auto n = strlen(it->tag2);
		exfn.assign(data, n);
		if (exfn == it->tag2)
		{
			c = it; break;//找到解码器
		}
	}
	if (c)
	{
		void* m = 0;
		r = new audio_data_t();
		if (c->open_memory) {
			if (iscopy) {
				r->dataold = (char*)malloc(len);
				if (r->dataold)
				{
					memcpy(r->dataold, data, len);
					data = r->dataold;
				}
			}
			m = c->open_memory(data, len, c->handle);
			if (!m)
			{
				if (r->dataold)free(r->dataold);
				r->dataold = 0;
			}
		}
		if (m)
		{
			auto mp = (hz::music_de_t*)m;
			audio_spec_t as = {};
			c->get_info(m, &as);
			r->channels = as.channels;
			r->code = c;
			r->format = as.format;
			r->freq = as.sample_rate;
			r->sample_rate = as.sample_rate;
			r->bits_per_sample = as.bits_per_sample;
			int bs = as.channels * (r->format == 0 ? sizeof(int16_t) : sizeof(int32_t));
			if (r->format == 3)
			{
				bs = as.channels * sizeof(double);
			}
			auto cap = bs * as.total_samples;
			r->cap = align_up(cap, 512);
			r->total_samples = as.total_samples;
			r->data = malloc(r->cap);
			r->ptr = m;
		}
	}
	return r;
}
void free_audio_data(audio_data_t* p)
{
	if (p) {
		auto mf = (hz::mfile_t*)p->mf;
		if (mf)
		{
			delete mf; p->mf = 0;
		}
		if (p->code && p->ptr)
		{
			p->code->free_m(p->ptr);
		}
		if (p->dataold)
			free(p->dataold);
		p->dataold = 0;
		if (p->data)
			free(p->data);

		delete p;
	}
}

/**
 * @Description : 使用FFT进行滤波
 *                使用示例：
 *                  原始采样频率为100kHz，采集了10000个点，保存为单精度浮点数。滤除其中20kHz~30kHz的频率
 *                  fft_filter_f(10000, in, out, 100e3, 20e3, 30e3)
 * @Input       : n             输入数据个数
 *                in            输入数据
 *                              in和out指向不同位置，不改变输入
 *                              in和out指向相同位置，输出将覆盖输入
 *                sample_rate   原始数据采样频率 Hz
 *                freq_start    需过滤的起始频率 Hz
 *                freq_end      需过滤的截止频率 Hz
 *                              若freq_end大于采样频率的50%，则将滤除大于freq_start的所有高频信号
 * @Output      : out           输出数据
 *                              in和out指向不同位置，不改变输入
 *                              in和out指向相同位置，输出将覆盖输入
 * @Return      : 无
*/
void fft_filter_f(int n, const float* in, float* out, float sample_rate, float freq_start, float freq_end)
{
	int i, begin, end;
	double* real;
	fftw_complex* complex;
	fftw_plan plan;

	// fftw的内存分配方式和mallco类似，但使用SIMD（单指令多数据流）时，fftw_alloc会将数组以更高效的方式对齐
	real = fftw_alloc_real(n);
	complex = fftw_alloc_complex(n / 2 + 1);    // 实际只会用到(n/2)+1个complex对象

	// Step1：FFT实现时域到频域的转换
	plan = fftw_plan_dft_r2c_1d(n, real, complex, FFTW_ESTIMATE);
	for (i = 0; i < n; i++)
	{
		real[i] = in[i];
	}

	// 对长度为n的实数进行FFT，输出的长度为(n/2)-1的复数
	fftw_execute(plan);
	fftw_destroy_plan(plan);

	// Step2：计算需滤波的频率在频域数组中的下标
	begin = (int)((freq_start / sample_rate) * n);
	end = (int)((freq_end / sample_rate) * n);
	end = end < (n / 2 + 1) ? end : (n / 2 + 1);
	for (i = begin; i < end; i++)
	{
		// 对应的频率分量置为0，即去除该频率
		complex[i][0] = 0;
		complex[i][1] = 0;
	}

	// Step3：IFFT实现频域到时域的转换
	// 使用FFTW_ESTIMATE构建plan不会破坏输入数据
	plan = fftw_plan_dft_c2r_1d(n, complex, real, FFTW_ESTIMATE);

	fftw_execute(plan);
	fftw_destroy_plan(plan);

	// Step4：计算滤波后的时域值
	for (i = 0; i < n; i++)
	{
		// 需除以数据个数，得到滤波后的实数
		out[i] = real[i] / n;
	}

	fftw_free(real);
	fftw_free(complex);
}

namespace hz {
	fft_cx::fft_cx()
	{
	}

	fft_cx::~fft_cx()
	{
		auto complex = (fftw_complex*)_complex;
		if (real)
			fftw_free(real);
		if (complex)
			fftw_free(complex);
		_complex = 0;
		real = 0;
	}
	void fft_cx::init(float sample_rate0, int bits, float freq_start0, float freq_end0)
	{
		bits_per_sample = bits;
		sample_rate = sample_rate0;
		freq_start = freq_start0;
		freq_end = freq_end0;
	}
	double hanning_window(int i, int n) {
		return 0.5 * (1 - cos(i * M_PI / n));
	}
	// 一维fftshift（以偶数长度为例）
	void fftshift_1d(fftw_complex* data, size_t n, double f) {
		int half = n * f;
		std::rotate(data, data + half, data + n);
	}

	// 二维矩阵fftshift（以行优先处理）
	void fftshift_2d(fftw_complex* matrix, int rows, int cols) {
		// 行交换 
		for (int i = 0; i < rows; ++i) {
			auto row_start = matrix + i * cols;
			std::rotate(row_start, row_start + cols / 2, row_start + cols);
		}
		//// 列交换 
		//std::vector<fftw_complex> temp(rows);
		//int half_col = cols / 2;
		//for (int j = 0; j < half_col; ++j) {
		//	for (int i = 0; i < rows; ++i)
		//		temp[i] = matrix[i * cols + j];
		//	for (int i = 0; i < rows; ++i)
		//		matrix[i * cols + j] = matrix[i * cols + j + half_col];
		//	for (int i = 0; i < rows; ++i)
		//		matrix[i * cols + j + half_col] = temp[i];
		//}
	}
	float* fft_cx::fft(float* data, int n)
	{
		if (n < 2)return 0;
		int rcount = FRAME_SIZE / 2;
		if (count != n)
		{
			auto complex = (fftw_complex*)_complex;
			if (real)
				fftw_free(real);
			if (complex)
				fftw_free(complex);
			real = 0;
			_complex = 0;
			FRAME_SIZE = count = n;
			rcount = FRAME_SIZE / 2;
			// fftw的内存分配方式和mallco类似，但使用SIMD（单指令多数据流）时，fftw_alloc会将数组以更高效的方式对齐
			real = fftw_alloc_real(n);
			_complex = fftw_alloc_complex(rcount + 1);    // 实际只会用到(n/2)+1个complex对象
			outdata.resize(n);
			magnitudesv.resize(rcount);
			heights.resize(rcount);
		}
		auto complex = (fftw_complex*)_complex;
		int begin = 0, end = 0;
		// Step1：FFT实现时域到频域的转换 

		for (int i = 0; i < count; i++)
		{
			real[i] = data[i] * hanning_window(i, FRAME_SIZE);
		}
		//fftw_plan plan = fftw_plan_dft_1d(count, (fftw_complex*)real, complex, FFTW_FORWARD, FFTW_ESTIMATE);
		fftw_plan plan = fftw_plan_dft_r2c_1d(count, real, complex, FFTW_ESTIMATE);
		// 对长度为n的实数进行FFT，输出的长度为(n/2)-1的复数
		fftw_execute(plan);
		fftw_destroy_plan(plan);
		//fftshift_1d(complex, rcount, 0.5);
		// Step2：计算需滤波的频率在频域数组中的下标
		begin = (int)((freq_start / sample_rate) * count);
		end = (int)((freq_end / sample_rate) * count);
		end = end < (count / 2 + 1) ? end : (count / 2 + 1);
		if (begin != end)
		{
			for (int i = begin; i < end; i++)
			{
				// 对应的频率分量置为0，即去除该频率
				complex[i][0] = 0;
				complex[i][1] = 0;   // 虚部设置为零
			}
			// Step3：IFFT实现频域到时域的转换
			// 使用FFTW_ESTIMATE构建plan不会破坏输入数据
			auto plan = fftw_plan_dft_c2r_1d(count, complex, real, FFTW_ESTIMATE);
			fftw_execute(plan);
			fftw_destroy_plan(plan);
		}
		auto outd = outdata.data();
		// Step4：计算滤波后的时域值
		for (int i = 0; i < count; i++)
		{
			// 需除以数据个数，得到滤波后的实数
			outd[i] = real[i] / count;
		}
		// 计算幅度谱
		double max_mag = 0.0;
		double min_mag = 0.0;
		double* magnitudes = magnitudesv.data();
		auto _out = (fftw_complex*)_complex;
		//int rcount = FRAME_SIZE / 2;
		for (int k = 0; k < rcount; k++) {
			auto& it = magnitudes[k];
			it = sqrt(pow(_out[k][0], 2) + pow(_out[k][1], 2));
			double power = pow(it, 2);
			double phase = atan2(_out[k][1], _out[k][0]);
			it = 20 * log10(it); // 转分贝 
			//it *= phase;
			if (std::isnan(it) || isinf(it))
				it = 0.0;
			if (it > max_mag) max_mag = it;
			if (it < min_mag) min_mag = it;
		}

		max_mag -= min_mag;
		for (int k = 0; k < rcount; k++) {
			heights[k] = ((magnitudes[k] - min_mag) / max_mag); // 归一化高度  
		}
		return outd;
	}
	//升采样（低→高）
	float* upsample(float* input, int in_len, int factor, float* output) {
		int out_len = in_len * factor;
		//float* output = calloc(out_len, sizeof(float)); 
		double maxf = 0.0;
		// 线性插值实现 
		for (int i = 0; i < in_len - 1; i++) {
			float delta = (input[i + 1] - input[i]) / factor;
			for (int j = 0; j < factor; j++) {
				output[i * factor + j] = input[i] + delta * j;
				if (output[i * factor + j] > maxf)
				{
					maxf = output[i * factor + j];
				}
			}
		}
		for (size_t i = 0; i < out_len; i++)
		{
			output[i] /= maxf;
		}
		return output;
	}
	//降采样（高→低）
	float* downsample(float* input, int in_len, int factor, float* output) {
		int out_len = in_len / factor;
		//float* output = malloc(out_len * sizeof(float));
		double maxf = 0.0;
		// 均值降采样 
		for (int i = 0; i < out_len; i++) {
			float sum = 0;
			for (int j = 0; j < factor; j++) {
				sum += input[i * factor + j];
			}
			output[i] = sum / factor;
			if (output[i] > maxf)
			{
				maxf = output[i];
			}
		}
		for (size_t i = 0; i < out_len; i++)
		{
			output[i] /= maxf;
		}
		return output;
	}
	// 简易FIR低通滤波器（截止频率=目标采样率/2）
	void apply_lpf(float* data, int len, int taps, float* coeff, float* filtered, float* dst) {
		//float* coeff = malloc(taps * sizeof(float));
		// 生成窗函数系数（示例使用汉明窗）
		for (int i = 0; i < taps; i++) {
			coeff[i] = 0.54 - 0.46 * cos(2 * M_PI * i / (taps - 1));
		}

		// 实现卷积运算 
		//float* filtered = malloc(len * sizeof(float));
		for (int n = 0; n < len; n++) {
			filtered[n] = 0;
			for (int k = 0; k < taps; k++) {
				int idx = n - k;
				if (idx >= 0) filtered[n] += data[idx] * coeff[k];
			}
		}
		if (dst)
			data = dst;
		memcpy(data, filtered, len * sizeof(float));
		//free(coeff);
		//free(filtered);
	}

	class fft_tiny
	{
	public:
		std::vector<float> _data;
	public:
		fft_tiny();
		~fft_tiny();
		void fft(float* data, int count, int sample_rate);
	private:

		// 定义输入音频缓冲区
		std::vector<double> input_buffer;
		// 创建 FFTW 输入和输出数组
		fftw_complex* fft_input = 0;
		fftw_complex* fft_output = 0;
	};

	fft_tiny::fft_tiny()
	{
	}

	fft_tiny::~fft_tiny()
	{
		if (fft_input)
			fftw_free(fft_input);
		if (fft_output)
			fftw_free(fft_output);
		fft_input = 0;
		fft_output = 0;
	}

	void fft_tiny::fft(float* data, int count, int sample_rate)
	{
		// 定义输入音频缓冲区
		if (input_buffer.size() != count)
		{
			_data.resize(count);
			input_buffer.resize(count);
			if (fft_input)
				fftw_free(fft_input);
			if (fft_output)
				fftw_free(fft_output);
			// 创建 FFTW 输入和输出数组
			fft_input = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * count);
			fft_output = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * count);
		}

		// 读取音频数据并进行频谱计算
		{
			// 创建 FFTW 3.0 的计划
			fftw_plan plan = fftw_plan_dft_1d(count, fft_input, fft_output, FFTW_FORWARD, FFTW_ESTIMATE);

			// 将输入音频数据复制到 FFTW 输入数组
			for (int i = 0; i < count; i++) {
				fft_input[i][0] = data[i];
				fft_input[i][1] = 0.0;  // 虚部设置为零
			}

			// 执行 FFT 变换
			fftw_execute(plan);

			double max_mag = 0.0;
			double min_mag = 0.0;
			// 计算频谱
			for (int i = 0; i < count; i++) {
				double magnitude = sqrt(fft_output[i][0] * fft_output[i][0] + fft_output[i][1] * fft_output[i][1]);
				double frequency = ((double)i / count) * sample_rate;
				_data[i] = magnitude;
				if (std::isnan(magnitude) || isinf(magnitude))
					magnitude = 0.0;
				if (magnitude > max_mag) max_mag = magnitude;
				if (magnitude < min_mag) min_mag = magnitude;
				// 在这里可以对频谱数据做进一步处理，例如绘图、输出等
				//printf("Frequency: %.2f Hz, Magnitude: %.4f\n", frequency, magnitude);
			}
			max_mag -= min_mag;
			for (int k = 0; k < count; k++) {
				_data[k] = ((_data[k] - min_mag) / max_mag); // 归一化高度  
			}
			// 销毁 FFTW 相关资源
			fftw_destroy_plan(plan);
		}
	}

	void fft_cx::calculate_heights(std::vector<float>& dh, int dcount, std::vector<float>& oy, std::vector<float>& lastY, glm::vec4* rects, int x, bool is_reverse)
	{
		if (dcount > 2)
		{
			int len = dh.size();
			int dst_rate = dcount, src_rate = len;
			// 计算采样率转换因子 
			int factor = (dst_rate > src_rate) ?
				dst_rate / src_rate :
				src_rate / dst_rate;
			int ass = taps + len * (factor + 2);
			if (sample_tem.size() != ass)
			{
				sample_tem.resize(ass);
			}
			auto t = sample_tem.data();
			float* output = 0;
			size_t length = 0;
			if (dst_rate == src_rate)
			{
				output = dh.data();
			}
			else {
				output = t + taps + len * 2;
				if (dst_rate > src_rate) { // 升采样 
					apply_lpf(dh.data(), len, taps, t, t + taps, output); // 预滤波 
					output = upsample(output, len, factor, t + taps + len);
					length = len * factor;
				}
				else { // 降采样 
					apply_lpf(dh.data(), len, taps, t, t + taps, output); // 抗混叠滤波 
					output = downsample(output, len, factor, t + taps + len);
					length = len / factor;
				}
			}
			length = length > dcount ? dcount : length;
			if (oy.size() != length)
				oy.resize(length);
			int inc = is_reverse ? -1 : 1;
			auto doy = oy.data();
			if (inc < 0)
			{
				doy += length - 1;
			}
			for (size_t i = 0; i < length; i++)
			{
				auto height = output[i];
				if (isnan(height))
					height = 0;
				if (isinf(height))
					height = 1.0;
				*doy = height * draw_height;
				doy += inc;
			}

			int n = oy.size();
			double f = 0.5;
			int half = n * f;
			auto rd = oy.data();
			double mn = 0.0;
			double mx = 0.0;
			for (auto& it : oy) {
				if (it < mn)
					mn = it;
				if (it > mx)
					mx = it;
			}
			mx -= mn;
			for (auto& it : oy) {
				it -= mn;
				if (mx > 0) {
					it /= mx;
				}
				if (isnan(it)) {
					it = 0;
				}
				it *= draw_height;

			}

			auto& y = oy;
			if (lastY.size() && is_smooth) {
				if (length != lastY.size())
					lastY.resize(length);
				for (int i = 0; i < length; i++) {
					if (y[i] < lastY[i]) {
						lastY[i] = y[i] * smoothConstantDown + lastY[i] * (1 - smoothConstantDown);
					}
					else {
						lastY[i] = y[i] * smoothConstantUp + lastY[i] * (1 - smoothConstantUp);
					}
				}
			}
			else {
				lastY = y;
			}
			x *= bar_width;
			length = glm::clamp((int)length, 0, dcount);
			for (size_t i = 0; i < length; i++)
			{
				auto height = lastY[i];
				rects[i] = { x + draw_pos.x + i * bar_width, draw_pos.y - height, bar_width - bar_step, height };
			}
		}
	}
	void calculate_spectrum(float* audio_frame, int frame_size, std::vector<float>& spectrum) {
		// 初始化输入/输出数组和FFT计划 
		double* in = fftw_alloc_real(frame_size);
		fftw_complex* out = fftw_alloc_complex(frame_size / 2 + 1);
		fftw_plan plan = fftw_plan_dft_r2c_1d(frame_size, in, out, FFTW_MEASURE);
		// 应用汉明窗减少频谱泄漏 
		for (int i = 0; i < frame_size; i++) {
			in[i] = audio_frame[i] * (0.54 - 0.46 * cos(2 * M_PI * i / (frame_size - 1)));
		}
		fftw_execute(plan);  // 执行FFT
		;
		// 计算幅度并归一化
		if (spectrum.size() != frame_size / 2 + 1)
			spectrum.resize(frame_size / 2 + 1);
		for (int i = 0; i <= frame_size / 2; i++) {
			spectrum[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
			spectrum[i] /= frame_size;  // 归一化处理 
		}
		fftw_free(in);
		fftw_free(out);
		fftw_destroy_plan(plan);
	}
	float* fft_cx::calculate_heights(float* audio_frame, int frame_size, int dcount)
	{
		if (vd2.size() != frame_size)
		{
			vd2.resize(frame_size);
		}
		frame_size /= 2;
		auto t = vd2.data();
		auto t1 = vd2.data() + frame_size;
		auto s = audio_frame;
		auto s1 = audio_frame + 1;
		for (size_t i = 0; i < frame_size; i++)
		{
			t[i] = *s; s += 2;
			t1[i] = *s1; s1 += 2;
		}
		//fft_tiny tn;
		//tn.fft(vd2.data(), frame_size * 2, 44100);
		{
			int dct = dcount / 2;
			dcount = dct * 2;
			if (_rects.size() != dcount)
				_rects.resize(dcount);

			if (is_raw)
			{
				auto& h2 = heights;
				auto nd = vd2.data();
				h2.resize(frame_size);
				for (size_t i = 0; i < frame_size; i++)
				{
					h2[i] = nd[i];
				}
				calculate_heights(h2, dct, _oy, _lastY[0], _rects.data(), 0, true);
				nd += frame_size;
				for (size_t i = 0; i < frame_size; i++)
				{
					h2[i] = nd[i];
				}
				calculate_heights(h2, dct, _oy, _lastY[1], _rects.data() + dct, dct, false);
			}
			else {
				float* a = fft(vd2.data(), frame_size);
				calculate_heights(heights, dct, _oy, _lastY[0], _rects.data(), 0, true);
				a = fft(vd2.data() + frame_size, frame_size);
				calculate_heights(heights, dct, _oy, _lastY[1], _rects.data() + dct, dct, false);
			}
		}
		return heights.data();
	}
	float* fft_cx::calculate_heights(short* audio_frame, int frame_size, int dcount)
	{
		vd.resize(frame_size);
		auto df = vd.data();
		uint32_t bps = (1 << (bits_per_sample - 1));
		float norm = 1.0 / bps;
		{
			auto oldt = audio_frame;
			for (size_t i = 0; i < frame_size; i++)
			{
				auto b = *audio_frame; audio_frame++;
				*df = b * norm; df++;
			}
			calculate_heights(vd.data(), frame_size, dcount);
		}
		return heights.data();
	}

	audio_cx::audio_cx()
	{
		config = njson::object();
		coders = new_coders();
	}

	audio_cx::~audio_cx()
	{
		_run = false;
		save_config();
		free_coders(coders); coders = 0;
	}

	void audio_cx::init(audio_backend_t* p, const std::string& confn) {
		if (p && p->dev && p->new_audio_stream && p->free_audio_stream && p->unbindaudio && p->unbindaudios && p->get_audio_stream_queued && p->put_audio && p->clear_audio)
		{
			bk = *p;
		}
		if (confn.size())
		{
			_confn = confn;
			config = read_json(confn);
		}
	}
	void audio_cx::save_config()
	{
		if (_confn.size())
		{
			save_json(_confn, config, indent_cbor);
		}
	}
	uint64_t a_get_ticks() {
		auto now = std::chrono::high_resolution_clock::now();
		// 转换为毫秒级时间戳
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		return ms;
	}
	void a_sleep_ms(int ms)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	}
	void audio_cx::add_song(size_t idx, const std::string& uri)
	{
		if (_lists.empty())
		{
			audio_list* st = new audio_list();
			_lists.push_back(st);
		}
		if (idx < _lists.size())
		{
			auto* it = new audio_item();
			it->path = uri; it->data = new_audio_data(coders, uri);
			if (it->data)
			{

			}
			_lists[idx]->v.push_back(it);
		}
	}
	std::mt19937* get_mtr() {
		static std::random_device rd;
		static std::mt19937 mt(rd());
		return &mt;
	}
	// 设置播放歌单
	void audio_cx::set_gd(size_t idx) {
		if (idx != gd_idx)
		{
			if (idx < _lists.size())
			{
				gd_idx = idx;
				_playlist.clear();
				auto* it = _lists[idx];
				for (size_t i = 0; i < it->v.size(); i++)
				{
					_playlist.push_back(i);
				}
				if (ge_type == 4)
				{
					// 随机数生成器 
					std::mt19937& g = *get_mtr();
					std::shuffle(_playlist.begin(), _playlist.end(), g);
				}
			}
		}
	}
	// 播放当前歌单指定索引
	void audio_cx::play(size_t idx) {
		int pidx = -1;
		if (idx != -1) {
			std::lock_guard <std::mutex> lock(_gd_mutex);
			if (idx < _playlist.size() && idx >= 0 && _playlist.size())
			{
				pidx = _playlist[idx];
				gd_sidx = idx;
			}
		}
		play(gd_idx, pidx);
	}
	// 设置播放类型
	void audio_cx::set_type(int type) {
		std::lock_guard <std::mutex> lock(_gd_mutex);
		if (ge_type == 4 && type != 4)
		{
			_playlist.clear();
			if (gd_idx < _lists.size())
			{
				auto it = _lists[gd_idx];
				for (size_t i = 0; i < it->v.size(); i++)
				{
					_playlist.push_back(i);
				}
			}
		}
		ge_type = type;
		if (ge_type == 4 && _playlist.size())
		{
			// 随机数生成器 
			std::mt19937& g = *get_mtr();
			std::shuffle(_playlist.begin(), _playlist.end(), g);
		}
	}
	audio_list* audio_cx::get_list_it(size_t i)
	{
		if (_lists.empty() || i > _lists.size())
		{
			return 0;
		}
		std::lock_guard <std::mutex> lock(_gd_mutex);
		return _lists[i];
	}
	void audio_cx::play(size_t idx, size_t i)
	{
		if (idx < _lists.size() && i >= 0)
		{
			auto it = get_list_it(idx);
			if (it && i < it->v.size())
			{
				auto* pt = it->v[i];
				if (pt && pt->data)
				{
					if (_current && _current->st)
					{
						_current = 0;
					}
					auto p = pt->data;
					push_decoder(pt);
					glm::ivec3 sz = { p->format,p->channels, p->sample_rate };
					auto mt = _streams.find(sz);
					void* st = 0;
					if (mt == _streams.end())
					{
						st = bk.new_audio_stream(bk.dev, p->format, p->channels, p->sample_rate);
						_streams[sz] = st;
					}
					else
					{
						st = mt->second;
					}
					if (st)
					{
						pt->cpos = 0;	// 当前播放位置
						pt->ctime = 0.0;	// 当前播放时间
						pt->st = st;
						pt->atime = p->total_samples / p->sample_rate;
						_current = pt;
					}
				}
			}
		}
		else { _current = nullptr; }
	}
	void audio_cx::push_decoder(audio_item* p) {
		if (p && p->data)
		{
			std::lock_guard <std::mutex> lock(_de_mutex);
			_de_list.push(p);
		}
	}
	void audio_cx::play_thr()
	{
		auto curr_time = bk.get_ticks();
		if (prev_time > 0)
		{
			auto ctp = curr_time - prev_time;
			double delta = (float)(ctp) / 1000.0f;
			// todo put data
			do {
				if (!_current || !_current->data)break;
				auto pt = _current->data;
				auto psq = bk.get_audio_stream_queued(_current->st);
				if (frame_size == 0) {
					frame_size = bk.get_audio_dst_framesize(_current->st);
				}
				auto psa = bk.get_audio_stream_available(_current->st);
				int psa0 = psa / frame_size;
				if ((psa == 0 || psq == 0)/*||_current->ctime >= _current->atime*/ && _current->cpos > 0)
				{
					//if (tem_buf.size() < 10240)
					//	tem_buf.resize(10240);
					//int r = SDL_GetAudioStreamData((SDL_AudioStream*)_current->st, tem_buf.data(), tem_buf.size());
					printf((char*)u8"剩下:%d\t%d\t%d\n", psq, psa, psa0);
					// 0单曲播放，1单曲循环，2顺序播放，3循环播放，4随机播放
					switch (ge_type)
					{
					case 0:
						gd_sidx = -1;
						break;
					case 1:
						gd_sidx;
						break;
					case 2:
						gd_sidx++;
						if (gd_sidx >= _playlist.size())
						{
							gd_sidx = -1;
						}
						break;
					case 3:
						gd_sidx++;
						if (gd_sidx >= _playlist.size())
						{
							gd_sidx = 0;
						}
						break;
					case 4:
						gd_sidx++;
						if (gd_sidx >= _playlist.size())
						{
							gd_sidx = 0;
							// 重置随机播放
							set_type(4);
						}
						break;
					default:
						break;
					}
					play(gd_sidx);
					ct = 0;
					break;
				}
				int psa1 = 0;
				int psq1 = 0;
				auto len = pt->desize;
				auto plen = pt->sample_rate * pt->channels * (pt->bits_per_sample / 8);
				const int large_input_thresh = 64 * 1024 - 536;
				plen = large_input_thresh;
				if (_current->cpos > psq)
				{
					cti = _current->cpos - psq;
				}
				if (psq > 0) {
					ct += delta;
					_current->ctime += delta;
				}
				if (_current->cpos >= len)
				{
					break;
				}
				if (_current->cpos < len)
				{
					char* putd = (char*)(pt->data) + _current->cpos;
					if (len == pt->len)
					{
						auto kl = len - _current->cpos;
						if (kl < plen)
							plen = kl;
					}
					if (plen > 0) {
						bk.put_audio(_current->st, putd, plen);
						_current->cpos += plen;
						psq1 = bk.get_audio_stream_queued(_current->st);
						psa1 = bk.get_audio_stream_available(_current->st) / frame_size;
						//printf("push %d\n", plen);
					}
				}
			} while (0);
		}
		prev_time = curr_time;
		bk.sleep_ms(waitms);
	}
	void audio_cx::run_thread()
	{
		if (!bk.sleep_ms)bk.sleep_ms = a_sleep_ms;
		if (!bk.get_ticks)bk.get_ticks = a_get_ticks;
		if (!bk.put_audio)return;
		if (!put_jt.native_handle()) {
			std::jthread j([=](std::stop_token st)
				{
					while (!st.stop_requested()) { play_thr(); }
				});
			put_jt.swap(j);// 音频推送播放线程
		}
		if (!de_jt.native_handle()) {
			std::jthread dj([=](std::stop_token st)
				{
					while (!st.stop_requested()) {
						audio_item* pt = 0;
						if (_de_list.size()) {
							std::lock_guard <std::mutex> lock(_de_mutex);
							pt = _de_list.front();
							_de_list.pop();
						}
						if (pt && pt->data)
						{
							while (1) {
								int rc1 = decoder_data(pt->data);
								if (rc1 <= 0)
								{
									break;
								}
							}
						}
						bk.sleep_ms(waitms);
					}
				});
			de_jt.swap(dj);// 音频解码线程
		}
	}
	void test_audiofft(audio_data_t* mad1) {
#if 0
		hz::audio_backend_t abc = { app->get_audio_device(),app_cx::new_audio_stream,app_cx::free_audio_stream,app_cx::bindaudio,app_cx::unbindaudio,app_cx::unbindaudios
	,app_cx::get_audio_stream_queued,app_cx::get_audio_stream_available,app_cx::get_audio_dst_framesize
	,app_cx::put_audio,app_cx::pause_audio,app_cx::mix_audio,app_cx::clear_audio,app_cx::sleep_ms,app_cx::get_ticks };
		auto audio_ctx = new hz::audio_cx();
		audio_ctx->init(&abc, "data/config_music.json");
		audio_ctx->run_thread();
		audio_ctx->add_song(0, R"(E:\song\陈奕迅-好久不见.flac)");
		// 设置播放歌单，只有一个歌单，所以设置0
		audio_ctx->set_gd(0);
		// 设置播放类型: 0单曲播放，1单曲循环，2顺序播放，3循环播放，4随机播放
		audio_ctx->set_type(3);
		// 播放当前歌单指定索引开始播放
		audio_ctx->play(0);
#endif
		hz::fft_cx* fft = new hz::fft_cx();
		hz::fft_cx* fft1 = new hz::fft_cx();
		double dtime = 0.06;
		int fs = mad1->sample_rate * dtime;
		{
			while (1)
			{
				int rc1 = decoder_data(mad1);
				if (rc1 <= 0)
				{
					break;
				}
			}
			fft->init(mad1->sample_rate, mad1->bits_per_sample, 0, 0);
			fft->draw_pos;
			fft->is_raw = true;
			fft1->init(mad1->sample_rate, mad1->bits_per_sample, 0, 0);
			fft1->draw_pos.y += 110;
			fft1->bar_width = 6;
			fft1->bar_step = 0;
			fft->bar_width = 6;
			fft->bar_step = 3;
		}
		{
			// 渲染回调
			SDL_Renderer* renderer = 0; double delta = 0.0;
			static double deltas = 0;
			deltas += delta;
			if (deltas > dtime)
			{
				static int64_t kn = 0;
				static int64_t kn1 = 0;
				static int64_t sn = 256;
				static int64_t sn1 = 2048;
				deltas = 0;
				fft->calculate_heights((short*)mad1->data + kn, sn * 2, 100);
				fft1->calculate_heights((short*)mad1->data + kn1, sn1 * 2, 100);
				kn += fs;
				kn1 += fs;
				if (kn >= mad1->total_samples)
				{
					kn = 0;
				}
				if (kn1 >= mad1->total_samples)
				{
					kn1 = 0;
				}
			}
			glm::vec4 color = { 0,0.5,1.0,0.8 };
			//form0->draw_rects(fft->_rects.data(), fft->_rects.size(), color);
			//form0->draw_rects(fft1->_rects.data(), fft1->_rects.size(), color);
			static std::vector<SDL_Vertex> vertices;
			//gen_rects(fft->_rects, vertices, { 0.1, 1, 0.1, 0.9 }, { 1, 0.5, 0, 1 });
			SDL_RenderGeometry(renderer, nullptr, vertices.data(), vertices.size(), nullptr, 0);
			//gen_rects(fft1->_rects, vertices, { 0.5, 0.0, 0, 0.00 }, { 1, 0.15, 0, 0.9 });
			SDL_RenderGeometry(renderer, nullptr, vertices.data(), vertices.size(), nullptr, 0);
		}
	}
}
//!hz