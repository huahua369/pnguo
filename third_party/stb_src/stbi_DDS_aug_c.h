
///	DDS file support, does decoding, _not_ direct uploading
///	(use SOIL for that ;-)

///	A bunch of DirectDraw Surface structures and flags
typedef struct {
	unsigned int    dwMagic;
	unsigned int    dwSize;
	unsigned int    dwFlags;
	unsigned int    dwHeight;
	unsigned int    dwWidth;
	unsigned int    dwPitchOrLinearSize;
	unsigned int    dwDepth;
	unsigned int    dwMipMapCount;
	unsigned int    dwReserved1[11];

	//  DDPIXELFORMAT
	struct {
		unsigned int    dwSize;
		unsigned int    dwFlags;
		unsigned int    dwFourCC;
		unsigned int    dwRGBBitCount;
		unsigned int    dwRBitMask;
		unsigned int    dwGBitMask;
		unsigned int    dwBBitMask;
		unsigned int    dwAlphaBitMask;
	}               sPixelFormat;

	//  DDCAPS2
	struct {
		unsigned int    dwCaps1;
		unsigned int    dwCaps2;
		unsigned int    dwDDSX;
		unsigned int    dwReserved;
	}               sCaps;
	unsigned int    dwReserved2;
} DDS_header;

//	the following constants were copied directly off the MSDN website

//	The dwFlags member of the original DDSURFACEDESC2 structure
//	can be set to one or more of the following values.
#define DDSD_CAPS	0x00000001
#define DDSD_HEIGHT	0x00000002
#define DDSD_WIDTH	0x00000004
#define DDSD_PITCH	0x00000008
#define DDSD_PIXELFORMAT	0x00001000
#define DDSD_MIPMAPCOUNT	0x00020000
#define DDSD_LINEARSIZE	0x00080000
#define DDSD_DEPTH	0x00800000

//	DirectDraw Pixel Format
#define DDPF_ALPHAPIXELS	0x00000001
#define DDPF_FOURCC	0x00000004
#define DDPF_RGB	0x00000040

//	The dwCaps1 member of the DDSCAPS2 structure can be
//	set to one or more of the following values.
#define DDSCAPS_COMPLEX	0x00000008
#define DDSCAPS_TEXTURE	0x00001000
#define DDSCAPS_MIPMAP	0x00400000

//	The dwCaps2 member of the DDSCAPS2 structure can be
//	set to one or more of the following values.
#define DDSCAPS2_CUBEMAP	0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX	0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX	0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY	0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY	0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ	0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ	0x00008000
#define DDSCAPS2_VOLUME	0x00200000
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "stbi_DDS_aug.h"
typedef struct
{
	uint32_t img_x, img_y;
	int img_n, img_out_n;

#ifndef STBI_NO_STDIO
	FILE* img_file;
#endif
	uint8_t* img_buffer, * img_buffer_end;
} stbi;

// this is not threadsafe
static char* failure_reason1 = 0;

char* stbi_failure_reason1(void)
{
	return failure_reason1;
}
static int e(const char* str)
{
	failure_reason1 = (char*)str;
	return 0;
}

#ifdef STBI_NO_FAILURE_STRINGS
#define e(x,y)  0
#elif defined(STBI_FAILURE_USERMSG)
#define e(x,y)  e(y)
#else
#define e(x,y)  e(x)
#endif
#define epf(x,y)   ((float *) (e(x,y)?NULL:NULL))
#define epuc(x,y)  ((unsigned char *) (e(x,y)?NULL:NULL))
//////////////////////////////////////////////////////////////////////////////
//
//  generic converter from built-in img_n to req_comp
//    individual types do this automatically as much as possible (e.g. jpeg
//    does all cases internally since it needs to colorspace convert anyway,
//    and it never has alpha, so very few cases ). png can automatically
//    interleave an alpha=255 channel, but falls back to this for other cases
//
//  assume data buffer is malloced, so malloc a new one and free that one
//  only failure mode is malloc failing

static uint8_t compute_y(int r, int g, int b)
{
	return (uint8_t)(((r * 77) + (g * 150) + (29 * b)) >> 8);
}

static unsigned char* convert_format(unsigned char* data, int img_n, int req_comp, uint32_t x, uint32_t y)
{
	int i, j;
	unsigned char* good;

	if (req_comp == img_n) return data;
	assert(req_comp >= 1 && req_comp <= 4);

	good = (unsigned char*)malloc(req_comp * x * y);
	if (good == NULL) {
		free(data);
		return epuc("outofmem", "Out of memory");
	}

	for (j = 0; j < (int)y; ++j) {
		unsigned char* src = data + j * x * img_n;
		unsigned char* dest = good + j * x * req_comp;

#define COMBO(a,b)  ((a)*8+(b))
#define CASE(a,b)   case COMBO(a,b): for(i=x-1; i >= 0; --i, src += a, dest += b)
		// convert source image with img_n components to one with req_comp components;
		// avoid switch per pixel, so use switch per scanline and massive macros
		switch (COMBO(img_n, req_comp)) {
			CASE(1, 2) dest[0] = src[0], dest[1] = 255; break;
			CASE(1, 3) dest[0] = dest[1] = dest[2] = src[0]; break;
			CASE(1, 4) dest[0] = dest[1] = dest[2] = src[0], dest[3] = 255; break;
			CASE(2, 1) dest[0] = src[0]; break;
			CASE(2, 3) dest[0] = dest[1] = dest[2] = src[0]; break;
			CASE(2, 4) dest[0] = dest[1] = dest[2] = src[0], dest[3] = src[1]; break;
			CASE(3, 4) dest[0] = src[0], dest[1] = src[1], dest[2] = src[2], dest[3] = 255; break;
			CASE(3, 1) dest[0] = compute_y(src[0], src[1], src[2]); break;
			CASE(3, 2) dest[0] = compute_y(src[0], src[1], src[2]), dest[1] = 255; break;
			CASE(4, 1) dest[0] = compute_y(src[0], src[1], src[2]); break;
			CASE(4, 2) dest[0] = compute_y(src[0], src[1], src[2]), dest[1] = src[3]; break;
			CASE(4, 3) dest[0] = src[0], dest[1] = src[1], dest[2] = src[2]; break;
		default: assert(0);
		}
#undef CASE
	}

	free(data);
	return good;
}
#ifndef STBI_NO_STDIO
static void start_file(stbi* s, FILE* f)
{
	s->img_file = f;
}
#endif

static void start_mem(stbi* s, uint8_t const* buffer, int len)
{
#ifndef STBI_NO_STDIO
	s->img_file = NULL;
#endif
	s->img_buffer = (uint8_t*)buffer;
	s->img_buffer_end = (uint8_t*)buffer + len;
}

__forceinline static int get8n(stbi* s)
{
#ifndef STBI_NO_STDIO
	if (s->img_file) {
		int c = fgetc(s->img_file);
		return c == EOF ? 0 : c;
	}
#endif
	if (s->img_buffer < s->img_buffer_end)
		return *s->img_buffer++;
	return 0;
}

__forceinline static int at_eof(stbi* s)
{
#ifndef STBI_NO_STDIO
	if (s->img_file)
		return feof(s->img_file);
#endif
	return s->img_buffer >= s->img_buffer_end;
}

__forceinline static uint8_t get8u(stbi* s)
{
	return (uint8_t)get8n(s);
}

static void skip1(stbi* s, int n)
{
#ifndef STBI_NO_STDIO
	if (s->img_file)
		fseek(s->img_file, n, SEEK_CUR);
	else
#endif
		s->img_buffer += n;
}

static int get16(stbi* s)
{
	int z = get8n(s);
	return (z << 8) + get8n(s);
}

static uint32_t get32n(stbi* s)
{
	uint32_t z = get16(s);
	return (z << 16) + get16(s);
}

static int get16le(stbi* s)
{
	int z = get8n(s);
	return z + (get8n(s) << 8);
}

static uint32_t get32le(stbi* s)
{
	uint32_t z = get16le(s);
	return z + (get16le(s) << 16);
}

typedef unsigned char stbi_uc;

static void getn1(stbi* s, stbi_uc* buffer, int n)
{
#ifndef STBI_NO_STDIO
	if (s->img_file) {
		fread(buffer, 1, n, s->img_file);
		return;
	}
#endif
	memcpy(buffer, s->img_buffer, n);
	s->img_buffer += n;
}

static int dds_test(stbi* s)
{
	//	check the magic number
	if (get8n(s) != 'D') return 0;
	if (get8n(s) != 'D') return 0;
	if (get8n(s) != 'S') return 0;
	if (get8n(s) != ' ') return 0;
	//	check header size
	if (get32le(s) != 124) return 0;
	return 1;
}
#ifndef STBI_NO_STDIO
int      stbi_dds_test_file(FILE* f)
{
	stbi s;
	int r, n = ftell(f);
	start_file(&s, f);
	r = dds_test(&s);
	fseek(f, n, SEEK_SET);
	return r;
}
#endif

int      stbi_dds_test_memory(stbi_uc const* buffer, int len)
{
	stbi s;
	start_mem(&s, buffer, len);
	return dds_test(&s);
}

inline int stbi_dds_free_memory(stbi_uc* d)
{
	if (d)
		free(d);
	return 0;
}

//	helper functions
int stbi_convert_bit_range(int c, int from_bits, int to_bits)
{
	int b = (1 << (from_bits - 1)) + c * ((1 << to_bits) - 1);
	return (b + (b >> from_bits)) >> from_bits;
}
void stbi_rgb_888_from_565(unsigned int c, int* r, int* g, int* b)
{
	*r = stbi_convert_bit_range((c >> 11) & 31, 5, 8);
	*g = stbi_convert_bit_range((c >> 05) & 63, 6, 8);
	*b = stbi_convert_bit_range((c >> 00) & 31, 5, 8);
}
void stbi_decode_DXT1_block(
	unsigned char uncompressed[16 * 4],
	unsigned char compressed[8])
{
	int next_bit = 4 * 8;
	int i, r, g, b;
	int c0, c1;
	unsigned char decode_colors[4 * 4];
	//	find the 2 primary colors
	c0 = compressed[0] + (compressed[1] << 8);
	c1 = compressed[2] + (compressed[3] << 8);
	stbi_rgb_888_from_565(c0, &r, &g, &b);
	decode_colors[0] = r;
	decode_colors[1] = g;
	decode_colors[2] = b;
	decode_colors[3] = 255;
	stbi_rgb_888_from_565(c1, &r, &g, &b);
	decode_colors[4] = r;
	decode_colors[5] = g;
	decode_colors[6] = b;
	decode_colors[7] = 255;
	if (c0 > c1)
	{
		//	no alpha, 2 interpolated colors
		decode_colors[8] = (2 * decode_colors[0] + decode_colors[4]) / 3;
		decode_colors[9] = (2 * decode_colors[1] + decode_colors[5]) / 3;
		decode_colors[10] = (2 * decode_colors[2] + decode_colors[6]) / 3;
		decode_colors[11] = 255;
		decode_colors[12] = (decode_colors[0] + 2 * decode_colors[4]) / 3;
		decode_colors[13] = (decode_colors[1] + 2 * decode_colors[5]) / 3;
		decode_colors[14] = (decode_colors[2] + 2 * decode_colors[6]) / 3;
		decode_colors[15] = 255;
	}
	else
	{
		//	1 interpolated color, alpha
		decode_colors[8] = (decode_colors[0] + decode_colors[4]) / 2;
		decode_colors[9] = (decode_colors[1] + decode_colors[5]) / 2;
		decode_colors[10] = (decode_colors[2] + decode_colors[6]) / 2;
		decode_colors[11] = 255;
		decode_colors[12] = 0;
		decode_colors[13] = 0;
		decode_colors[14] = 0;
		decode_colors[15] = 0;
	}
	//	decode the block
	for (i = 0; i < 16 * 4; i += 4)
	{
		int idx = ((compressed[next_bit >> 3] >> (next_bit & 7)) & 3) * 4;
		next_bit += 2;
		uncompressed[i + 0] = decode_colors[idx + 0];
		uncompressed[i + 1] = decode_colors[idx + 1];
		uncompressed[i + 2] = decode_colors[idx + 2];
		uncompressed[i + 3] = decode_colors[idx + 3];
	}
	//	done
}
void stbi_decode_DXT23_alpha_block(
	unsigned char uncompressed[16 * 4],
	unsigned char compressed[8])
{
	int i, next_bit = 0;
	//	each alpha value gets 4 bits
	for (i = 3; i < 16 * 4; i += 4)
	{
		uncompressed[i] = stbi_convert_bit_range(
			(compressed[next_bit >> 3] >> (next_bit & 7)) & 15,
			4, 8);
		next_bit += 4;
	}
}
void stbi_decode_DXT45_alpha_block(
	unsigned char uncompressed[16 * 4],
	unsigned char compressed[8])
{
	int i, next_bit = 8 * 2;
	unsigned char decode_alpha[8];
	//	each alpha value gets 3 bits, and the 1st 2 bytes are the range
	decode_alpha[0] = compressed[0];
	decode_alpha[1] = compressed[1];
	if (decode_alpha[0] > decode_alpha[1])
	{
		//	6 step intermediate
		decode_alpha[2] = (6 * decode_alpha[0] + 1 * decode_alpha[1]) / 7;
		decode_alpha[3] = (5 * decode_alpha[0] + 2 * decode_alpha[1]) / 7;
		decode_alpha[4] = (4 * decode_alpha[0] + 3 * decode_alpha[1]) / 7;
		decode_alpha[5] = (3 * decode_alpha[0] + 4 * decode_alpha[1]) / 7;
		decode_alpha[6] = (2 * decode_alpha[0] + 5 * decode_alpha[1]) / 7;
		decode_alpha[7] = (1 * decode_alpha[0] + 6 * decode_alpha[1]) / 7;
	}
	else
	{
		//	4 step intermediate, pluss full and none
		decode_alpha[2] = (4 * decode_alpha[0] + 1 * decode_alpha[1]) / 5;
		decode_alpha[3] = (3 * decode_alpha[0] + 2 * decode_alpha[1]) / 5;
		decode_alpha[4] = (2 * decode_alpha[0] + 3 * decode_alpha[1]) / 5;
		decode_alpha[5] = (1 * decode_alpha[0] + 4 * decode_alpha[1]) / 5;
		decode_alpha[6] = 0;
		decode_alpha[7] = 255;
	}
	for (i = 3; i < 16 * 4; i += 4)
	{
		int idx = 0, bit;
		bit = (compressed[next_bit >> 3] >> (next_bit & 7)) & 1;
		idx += bit << 0;
		++next_bit;
		bit = (compressed[next_bit >> 3] >> (next_bit & 7)) & 1;
		idx += bit << 1;
		++next_bit;
		bit = (compressed[next_bit >> 3] >> (next_bit & 7)) & 1;
		idx += bit << 2;
		++next_bit;
		uncompressed[i] = decode_alpha[idx & 7];
	}
	//	done
}
void stbi_decode_DXT_color_block(
	unsigned char uncompressed[16 * 4],
	unsigned char compressed[8])
{
	int next_bit = 4 * 8;
	int i, r, g, b;
	int c0, c1;
	unsigned char decode_colors[4 * 3];
	//	find the 2 primary colors
	c0 = compressed[0] + (compressed[1] << 8);
	c1 = compressed[2] + (compressed[3] << 8);
	stbi_rgb_888_from_565(c0, &r, &g, &b);
	decode_colors[0] = r;
	decode_colors[1] = g;
	decode_colors[2] = b;
	stbi_rgb_888_from_565(c1, &r, &g, &b);
	decode_colors[3] = r;
	decode_colors[4] = g;
	decode_colors[5] = b;
	//	Like DXT1, but no choicees:
	//	no alpha, 2 interpolated colors
	decode_colors[6] = (2 * decode_colors[0] + decode_colors[3]) / 3;
	decode_colors[7] = (2 * decode_colors[1] + decode_colors[4]) / 3;
	decode_colors[8] = (2 * decode_colors[2] + decode_colors[5]) / 3;
	decode_colors[9] = (decode_colors[0] + 2 * decode_colors[3]) / 3;
	decode_colors[10] = (decode_colors[1] + 2 * decode_colors[4]) / 3;
	decode_colors[11] = (decode_colors[2] + 2 * decode_colors[5]) / 3;
	//	decode the block
	for (i = 0; i < 16 * 4; i += 4)
	{
		int idx = ((compressed[next_bit >> 3] >> (next_bit & 7)) & 3) * 3;
		next_bit += 2;
		uncompressed[i + 0] = decode_colors[idx + 0];
		uncompressed[i + 1] = decode_colors[idx + 1];
		uncompressed[i + 2] = decode_colors[idx + 2];
	}
	//	done
}
int get_format_bgr(DDS_header* h)
{
	int ret = 0;
	if (h->sPixelFormat.dwFlags & 0x00000004)   //DDPF_FOURCC
	{
		// Check for D3DFORMAT enums being set here
		switch (h->sPixelFormat.dwFourCC)
		{
			//case '1TXD':         return DXGI_FORMAT_BC1_UNORM;
			//case '3TXD':         return DXGI_FORMAT_BC2_UNORM;
			//case '5TXD':         return DXGI_FORMAT_BC3_UNORM;
			//case 'U4CB':         return DXGI_FORMAT_BC4_UNORM;
			//case 'A4CB':         return DXGI_FORMAT_BC4_SNORM;
			//case '2ITA':         return DXGI_FORMAT_BC5_UNORM;
			//case 'S5CB':         return DXGI_FORMAT_BC5_SNORM;
			//case 'GBGR':         return DXGI_FORMAT_R8G8_B8G8_UNORM;
			//case 'BGRG':         return DXGI_FORMAT_G8R8_G8B8_UNORM;
			//case 36:             return DXGI_FORMAT_R16G16B16A16_UNORM;
			//case 110:            return DXGI_FORMAT_R16G16B16A16_SNORM;
			//case 111:            return DXGI_FORMAT_R16_FLOAT;
			//case 112:            return DXGI_FORMAT_R16G16_FLOAT;
			//case 113:            return DXGI_FORMAT_R16G16B16A16_FLOAT;
			//case 114:            return DXGI_FORMAT_R32_FLOAT;
			//case 115:            return DXGI_FORMAT_R32G32_FLOAT;
			//case 116:            return DXGI_FORMAT_R32G32B32A32_FLOAT;
		default:             return 0;
		}
	}
	else
	{
		switch (h->sPixelFormat.dwRBitMask)
		{
		case 0xff:        ret = 0; break;// DXGI_FORMAT_R8G8B8A8_UNORM;
		case 0x00ff0000:  ret = 1; break;// DXGI_FORMAT_B8G8R8A8_UNORM;
		//case 0xffff:      return DXGI_FORMAT_R16G16_UNORM;
		//case 0x3ff:       return DXGI_FORMAT_R10G10B10A2_UNORM;
		//case 0x7c00:      return DXGI_FORMAT_B5G5R5A1_UNORM;
		//case 0xf800:      return DXGI_FORMAT_B5G6R5_UNORM;
		//case 0:           return DXGI_FORMAT_A8_UNORM;
		default:          return 0;
		};
	}
	return ret;
}
static stbi_uc* dds_load(stbi* s, int req_comp, stbi_info_t* p)
{
	//	all variables go up front
	stbi_uc* dds_data = NULL;
	stbi_uc block[16 * 4];
	stbi_uc compressed[8];
	int flags, DXT_family;
	int has_alpha, has_mipmap;
	int is_compressed, cubemap_faces;
	int block_pitch, num_blocks;
	DDS_header header;
	int i, sz, cf;
	//	load the header
	if (sizeof(DDS_header) != 128)
	{
		return NULL;
	}
	getn1(s, (stbi_uc*)(&header), 128);
	//	and do some checking
	if (header.dwMagic != (('D' << 0) | ('D' << 8) | ('S' << 16) | (' ' << 24))) return NULL;
	if (header.dwSize != 124) return NULL;
	flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	if ((header.dwFlags & flags) != flags) return NULL;
	/*	According to the MSDN spec, the dwFlags should contain
		DDSD_LINEARSIZE if it's compressed, or DDSD_PITCH if
		uncompressed.  Some DDS writers do not conform to the
		spec, so I need to make my reader more tolerant	*/
	if (header.sPixelFormat.dwSize != 32) return NULL;
	flags = DDPF_FOURCC | DDPF_RGB;
	if ((header.sPixelFormat.dwFlags & flags) == 0) return NULL;
	if ((header.sCaps.dwCaps1 & DDSCAPS_TEXTURE) == 0) return NULL;
	//	get the image data
	s->img_x = header.dwWidth;
	s->img_y = header.dwHeight;
	s->img_n = 4;
	is_compressed = (header.sPixelFormat.dwFlags & DDPF_FOURCC) / DDPF_FOURCC;
	has_alpha = (header.sPixelFormat.dwFlags & DDPF_ALPHAPIXELS) / DDPF_ALPHAPIXELS;
	has_mipmap = (header.sCaps.dwCaps1 & DDSCAPS_MIPMAP) && (header.dwMipMapCount > 1);
	cubemap_faces = (header.sCaps.dwCaps2 & DDSCAPS2_CUBEMAP) / DDSCAPS2_CUBEMAP;
	/*	I need cubemaps to have square faces	*/
	cubemap_faces &= (s->img_x == s->img_y);
	cubemap_faces *= 5;
	cubemap_faces += 1;
	block_pitch = (s->img_x + 3) >> 2;
	num_blocks = block_pitch * ((s->img_y + 3) >> 2);
	/*	let the user know what's going on	*/
	//*x = s->img_x;
	//*y = s->img_y;
	//*comp = s->img_n;
	/*	is this uncompressed?	*/
	if (is_compressed)
	{
		/*	compressed	*/
		//	note: header.sPixelFormat.dwFourCC is something like (('D'<<0)|('X'<<8)|('T'<<16)|('1'<<24))
		DXT_family = 1 + (header.sPixelFormat.dwFourCC >> 24) - '1';
		if ((DXT_family < 1) || (DXT_family > 5)) return NULL;
		/*	check the expected size...oops, nevermind...
			those non-compliant writers leave
			dwPitchOrLinearSize == 0	*/
			//	passed all the tests, get the RAM for decoding
		sz = (s->img_x) * (s->img_y) * 4 * cubemap_faces;
		dds_data = (unsigned char*)malloc(sz);
		/*	do this once for each face	*/
		for (cf = 0; cf < cubemap_faces; ++cf)
		{
			//	now read and decode all the blocks
			for (i = 0; i < num_blocks; ++i)
			{
				//	where are we?
				int bx, by, bw = 4, bh = 4;
				int ref_x = 4 * (i % block_pitch);
				int ref_y = 4 * (i / block_pitch);
				//	get the next block's worth of compressed data, and decompress it
				if (DXT_family == 1)
				{
					//	DXT1
					getn1(s, compressed, 8);
					stbi_decode_DXT1_block(block, compressed);
				}
				else if (DXT_family < 4)
				{
					//	DXT2/3
					getn1(s, compressed, 8);
					stbi_decode_DXT23_alpha_block(block, compressed);
					getn1(s, compressed, 8);
					stbi_decode_DXT_color_block(block, compressed);
				}
				else
				{
					//	DXT4/5
					getn1(s, compressed, 8);
					stbi_decode_DXT45_alpha_block(block, compressed);
					getn1(s, compressed, 8);
					stbi_decode_DXT_color_block(block, compressed);
				}
				//	is this a partial block?
				if (ref_x + 4 > s->img_x)
				{
					bw = s->img_x - ref_x;
				}
				if (ref_y + 4 > s->img_y)
				{
					bh = s->img_y - ref_y;
				}
				//	now drop our decompressed data into the buffer
				for (by = 0; by < bh; ++by)
				{
					int idx = 4 * ((ref_y + by + cf * s->img_x) * s->img_x + ref_x);
					for (bx = 0; bx < bw * 4; ++bx)
					{

						dds_data[idx + bx] = block[by * 16 + bx];
					}
				}
			}
			/*	done reading and decoding the main image...
				skip MIPmaps if present	*/
			if (has_mipmap)
			{
				int block_size = 16;
				if (DXT_family == 1)
				{
					block_size = 8;
				}
				for (i = 1; i < header.dwMipMapCount; ++i)
				{
					int mx = s->img_x >> (i + 2);
					int my = s->img_y >> (i + 2);
					if (mx < 1)
					{
						mx = 1;
					}
					if (my < 1)
					{
						my = 1;
					}
					skip1(s, mx * my * block_size);
				}
			}
		}/* per cubemap face */
	}
	else
	{
		/*	uncompressed	*/
		DXT_family = 0;
		s->img_n = 3;
		if (has_alpha)
		{
			s->img_n = 4;
		}
		//*comp = s->img_n;
		sz = s->img_x * s->img_y * s->img_n * cubemap_faces;
		dds_data = (unsigned char*)malloc(sz);
		/*	do this once for each face	*/
		for (cf = 0; cf < cubemap_faces; ++cf)
		{
			/*	read the main image for this face	*/
			getn1(s, &dds_data[cf * s->img_x * s->img_y * s->img_n], s->img_x * s->img_y * s->img_n);
			/*	done reading and decoding the main image...
				skip MIPmaps if present	*/
			if (has_mipmap)
			{
				for (i = 1; i < header.dwMipMapCount; ++i)
				{
					int mx = s->img_x >> i;
					int my = s->img_y >> i;
					if (mx < 1)
					{
						mx = 1;
					}
					if (my < 1)
					{
						my = 1;
					}
					skip1(s, mx * my * s->img_n);
				}
			}
		}
		int isbgr = get_format_bgr(&header);
		/*	data was BGR, I need it RGB	*/
		if (isbgr)
		{
			for (i = 0; i < sz; i += s->img_n)
			{
				unsigned char temp = dds_data[i];
				dds_data[i] = dds_data[i + 2];
				dds_data[i + 2] = temp;
			}
		}
	}
	/*	finished decompressing into RGBA,
		adjust the y size if we have a cubemap
		note: sz is already up to date	*/
	s->img_y *= cubemap_faces;
	//*y = s->img_y;
	//	did the user want something else, or
	//	see if all the alpha values are 255 (i.e. no transparency)
	has_alpha = 0;
	int comp = s->img_n;
	if (s->img_n == 4)
	{
		for (i = 3; (i < sz) && (has_alpha == 0); i += 4)
		{
			has_alpha |= (dds_data[i] < 255);
		}
	}
	if ((req_comp <= 4) && (req_comp >= 1))
	{
		//	user has some requirements, meet them
		if (req_comp != s->img_n)
		{
			dds_data = convert_format(dds_data, s->img_n, req_comp, s->img_x, s->img_y);
			comp = s->img_n;
		}
	}
	else
	{
		//	user had no requirements, only drop to RGB is no alpha
		if ((has_alpha == 0) && (s->img_n == 4))
		{
			dds_data = convert_format(dds_data, 4, 3, s->img_x, s->img_y);
			comp = 3;
		}
	}
	if (p)
	{
		p->comp = comp;
		p->arraySize = cubemap_faces;
		p->mipMapCount = 1;// skip header.dwMipMapCount;
		p->width = s->img_x;
		p->height = header.dwHeight;
		p->depth = header.dwDepth;
		p->data = dds_data;
		p->size = req_comp * s->img_x * s->img_y;
	}
	//	OK, done
	return dds_data;
}

#ifndef STBI_NO_STDIO
stbi_uc* stbi_dds_load_from_file(FILE* f, int req_comp, stbi_info_t* p)
{
	stbi s;
	start_file(&s, f);
	return dds_load(&s, req_comp, p);
}

stbi_uc* stbi_dds_load(char* filename, int req_comp, stbi_info_t* p)
{
	stbi_uc* data;
	FILE* f = fopen(filename, "rb");
	if (!f) return NULL;
	data = stbi_dds_load_from_file(f, req_comp, p);
	fclose(f);
	return data;
}
#endif

stbi_uc* stbi_dds_load_from_memory(stbi_uc const* buffer, int len, int req_comp, stbi_info_t* p)
{
	stbi s;
	start_mem(&s, buffer, len);
	return dds_load(&s, req_comp, p);
}
