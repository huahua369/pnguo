/*
音频实现
本文件创建时间：2025/3/19
*/
#include "pch1.h"
#include "audio.h"
#include "mapView.h"

#if 1

#include <FLAC/all.h>

namespace hz {

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

	class audio_data_t :public rw_t
	{
	public:
		audio_data_t()
		{
		}

		~audio_data_t()
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
		rw_t* src = nullptr;

		AudioSpec_t spec = {};
		// 总样本数，位
		int64_t total_samples = 0, bits_per_sample = 0;
		int64_t de_samples = 0;
		// 解码器
		void* _decoder = 0;
		//audio_data_t* stream = 0;
		int samples = 8192;
		double seconds = 0.0;
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

		static FLAC__StreamDecoderWriteStatus flac_write_music_cb(
			const FLAC__StreamDecoder* decoder,
			const FLAC__Frame* frame,
			const FLAC__int32* const buffer[],
			void* client_data)
		{
			music_de_t* music = (music_de_t*)client_data;
			rw_t* mv = (rw_t*)music->src;
			std::vector<int16_t> data;
			unsigned int i, j, channels;
			int shift_amount = 0;

			//if (!music->stream) {
			//	return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
			//}
			auto bp = music->get_bits_per();
			switch (bp) {
			case 16:
				shift_amount = 0;
				break;
			case 20:
				shift_amount = 4;
				break;
			case 24:
				shift_amount = 8;
				break;
			default:
				printf("FLAC decoder doesn't support %d bits_per_sample", music->get_bits_per());
				return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
			}

			if (music->get_channels() == 3) {
				/* We'll just drop the center channel for now */
				channels = 2;
			}
			else {
				channels = music->get_channels();
			}
			int64_t dl = frame->header.blocksize * channels;
			if (bp > 0)
			{
				data.resize(dl);
				//if (bp == 16)
				dl *= sizeof(int16_t);
				//else
				//	dl *= sizeof(int32_t);
				if (music->_buffer.size() != dl)
					music->_buffer.resize(dl);
				if (data.empty()) {
					printf("Couldn't allocate %d bytes stack memory", (int)dl);
					return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
				}

				if (music->get_channels() == 3) {
					//int16_t* dst = data.data();
					int16_t* dst = (int16_t*)music->_buffer.data();
					for (i = 0; i < frame->header.blocksize; ++i) {
						int16_t FL = (buffer[0][i] >> shift_amount);
						int16_t FR = (buffer[1][i] >> shift_amount);
						int16_t FCmix = (int16_t)((buffer[2][i] >> shift_amount) * 0.5f);
						int sample;

						sample = (FL + FCmix);
						if (sample > SHRT_MAX) {
							*dst = SHRT_MAX;
						}
						else if (sample < SHRT_MIN) {
							*dst = SHRT_MIN;
						}
						else {
							*dst = sample;
						}
						++dst;

						sample = (FR + FCmix);
						if (sample > SHRT_MAX) {
							*dst = SHRT_MAX;
						}
						else if (sample < SHRT_MIN) {
							*dst = SHRT_MIN;
						}
						else {
							*dst = sample;
						}
						++dst;
					}
				}
				else {
					//if (bp == 16)
					{
						auto dtt = (int16_t*)music->_buffer.data();
						for (i = 0; i < channels; ++i) {
							int16_t* dst = dtt + i;
							for (j = 0; j < frame->header.blocksize; ++j) {
								auto k = buffer[i][j];
								*dst = (k >> shift_amount);
								dst += channels;
							}
						}
					}
					//else
					//{
					//	auto dtt = (int32_t*)music->_buffer.data();
					//	for (i = 0; i < channels; ++i) {
					//		int32_t* dst = dtt + i;
					//		for (j = 0; j < frame->header.blocksize; ++j) {
					//			auto k = buffer[i][j];
					//			*dst = (k >> shift_amount);
					//			dst += channels;
					//		}
					//	}
					//}
				}
			}
			else {
				std::vector<int32_t> data32;
				data32.resize(dl);
				dl *= sizeof(int32_t);
				for (i = 0; i < channels; ++i) {
					auto dst = data32.data() + i;
					for (j = 0; j < frame->header.blocksize; ++j) {
						auto s = buffer[i][j];
						*dst = s >> shift_amount;
						dst += channels;
					}
				}
				if (music->_buffer.size() != dl)
					music->_buffer.resize(dl);
				memcpy(music->_buffer.data(), data32.data(), dl);
			}

			//music->stream->put(data.data(), dl);
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
			switch (music->bits_per_sample)
			{
			case 16:
				format = 0;
				break;
			case 24:
			case 32:
				format = 1;
				break;
			}
			format = 1;
			music->set_spec(metadata->data.stream_info.sample_rate, metadata->data.stream_info.channels, format);// SDL_AUDIO_S16);
			music->total_samples = metadata->data.stream_info.total_samples;
			double t = music->total_samples; t /= music->spec.freq;
			music->seconds = t;
			/*printf("FLAC: Sample rate = %d, channels = %d, bits_per_sample = %d\n", music->sample_rate, music->channels, music->bits_per_sample);*/

				/* SDL's channel mapping and FLAC channel mapping are the same,
				   except for 3 channels: SDL is FL FR LFE and FLAC is FL FR FC
				 */
			if (music->get_channels() == 3) {
				channels = 2;
			}
			else {
				channels = music->get_channels();
			}

			//music->stream = new audio_data_t();
			//if (!music->stream) {
			//	return;
			//}
			/* We check for NULL stream later when we get data */
			//SDL_assert(!music->stream);
			//music->stream = new audio_stream_t(music->get_format(), channels, music->get_rate(), music->bits_per_sample, music->total_samples);
			//music->stream = SDL_NewAudioStream(SDL_AUDIO_S16, channels, music->sample_rate, music_spec.format, music_spec.channels, music_spec.freq);
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
				//filled = music->stream->get(data, bytes);
				//if (filled != 0) {
				//	return filled;
				//}

				//if (music->stream->is_done()) {
				//	/* All done */
				//	if (done)
				//		*done = true;
				//	return 0;
				//}
				music->_buffer.clear();
				auto flac = (flac_loader*)music->ctx;
				if (!flac->FLAC__stream_decoder_process_single((FLAC__StreamDecoder*)music->_decoder)) {
					printf("FLAC__stream_decoder_process_single() failed");
					return -1;
				}

				if (flac->FLAC__stream_decoder_get_state((FLAC__StreamDecoder*)music->_decoder) == FLAC__STREAM_DECODER_END_OF_STREAM) {
					//music->stream->flush();
					music->_buffer.clear();
				}
				filled = music->_buffer.size();
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
		// func返回nullptr或长度小于0则跳出编码
		static size_t encoder(encoder_info_t* ep)
		{
			flac_loader* flac = ((flac_de_t*)ep->handle)->lib();
			if (!flac || !ep || (!ep->file_path && !ep->write_func))
			{
				return -1;
			}
			return wav2flac(ep, ep->file_src, ep->file_path, flac);
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

			mfile_t fmv;
			if (ep->file_path)// && fmv.createfile(ep->file_path))
			{
				//auto ks = flac->FLAC__stream_encoder_get_total_samples_estimate(_encoder);
				//out_stream.write_func = fwrite_func;
				//out_stream.seek_func = fseek_func;
				//out_stream.tell_func = ftell_func;
				//out_stream.ud = &fmv;
			}
			else
			{
				out_stream.ud = ep->userdata;
				out_stream.write_func = ep->write_func;
			}
			if (ep->file_path)
			{

				flac->FLAC__stream_encoder_init_file(_encoder, ep->file_path, progress_callback, NULL);

			}
			else {
				flac->FLAC__stream_encoder_init_stream(_encoder, enWriteCallback, enSeekCallback, enTellCallback, enMetadataCallback, &out_stream);
			}
			int64_t total_bytes_read = 0;

			int bytes_read = 0;
			const int pcmsize = flac->FLAC__stream_encoder_get_blocksize(_encoder);
			std::vector<FLAC__int32> pcmv(pcmsize);
			auto pcm = pcmv.data();
			if (ep->data && ep->data_size > 0)
			{
				encoder_process_interleaved(flac, _encoder, ep->data, ep->data_size, ep->channels, pcm, pcmsize);
				total_bytes_read += bytes_read;
			}
			else
			{
				do
				{
					auto data = ep->read_func(&bytes_read, ep->userdata);
					if (!data || bytes_read < 2)
					{
						break;
					}
					encoder_process_interleaved(flac, _encoder, data, bytes_read, ep->channels, pcm, pcmsize);
					total_bytes_read += bytes_read;
				} while (true);
			}
			//finish encoding the current flac file
			flac->FLAC__stream_encoder_finish(_encoder);
			flac->FLAC__stream_encoder_delete(_encoder);

			return total_bytes_read > 0 ? total_bytes_read : 0;
		}
		static bool encoder_process_interleaved(flac_loader* flac, FLAC__StreamEncoder* _encoder, const char* data, size_t bytes_read, int channels, FLAC__int32* pcm, int pcmsize)
		{
			bool ret = false;
			short* flacBytes = (short*)data;
			size_t need = (size_t)bytes_read / 2;
			int64_t as = need * channels;
			int ads = pcmsize * 2 / channels;
			int ic = 0;
			for (; as > 0;)
			{
				ic++;
				int64_t cs = as, inc = 0;
				if (cs > pcmsize)
				{
					cs = pcmsize;
					inc = ads;
				}
				else
				{
					inc = cs * 2 / channels;
				}
				for (int64_t i = 0; i < cs; i++) {
					pcm[i] = flacBytes[i];
				}
				ret = flac->FLAC__stream_encoder_process_interleaved(_encoder, pcm, cs);
				as -= cs;
				flacBytes += cs;
			}
			return ret;
		}
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
#define READSIZE 1024

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

	void get_info_ty(void* music, int* total_samples, int* channels, int* sample_rate, int* bits_per_sample)
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

	coder_t* create_flac_de()
	{
		coder_t* ct = new coder_t();
		ct->tag = "flac";
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

}
//!hz

#endif // 1

coders_t* new_coders()
{
	coder_t* flac = hz::create_flac_de();
	auto p = new coders_t();
	if (p) {
		if (flac)
			p->codes.push_back(flac);
	}
	return p;
}
void free_coders(coders_t* p)
{
	if (p)delete p;
}
int decoder_data(audio_data_t* p)
{
	int rc = 0;
	if (p && p->data && p->len > 0 && p->code && p->desize < p->len)
	{
		rc = p->code->get_audio(p->ptr, (char*)p->data + p->desize, p->len - p->desize);
		if (rc > 0)
		{
			p->desize += rc;
		}
	}
	return rc;
}
audio_data_t* new_audio_data(coders_t* pc, const std::string& fn)
{
	audio_data_t* r = 0;
	coder_t* c = 0;
	int total_samples = 0, channels = 0, sample_rate = 0, bits_per_sample = 0;
	std::string exfn;
	for (auto it : pc->codes)
	{
		auto n = strlen(it->tag);
		exfn = fn.substr(fn.size() - n, n);
		if (exfn == it->tag)
		{
			c = it; break;//找到解码器
		}
	}
	if (c)
	{
		void* m = 0;
		r = new audio_data_t();
		if (c->open_file) {
			m = c->open_file(fn.c_str(), c->handle);
		}
		else if (c->open_memory) {
			hz::mfile_t mf;
			auto md = mf.open_d(fn, true);
			if (md)
			{
				r->mf = new hz::mfile_t();
				*((hz::mfile_t*)r->mf) = mf;
				mf.clear_ptr();
			}
			m = c->open_memory(md, mf.size(), c->handle);
		}
		if (m)
		{
			c->get_info(m, &total_samples, &channels, &sample_rate, &bits_per_sample);
			r->channels = channels;
			r->code = c;
			r->freq = sample_rate;
			int bs = 2;// bits_per_sample == 16 ? 2 : 4;
			r->len = bs * total_samples * channels;
			int format = 0;
			switch (bits_per_sample)
			{
			case 16:
				format = 0;
				break;
				//case 24:
				//	format = 1;
				//	break;
				//case 32:
				//	format = 1;
				//	break;
			}
			r->data = malloc(r->len);
			r->format = format; // 32位播放失败
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
		if (p->data)
			free(p->data);
		delete p;
	}
}