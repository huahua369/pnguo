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


int main()
{
	return test_create_outstream();
}
