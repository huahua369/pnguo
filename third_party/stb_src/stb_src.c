
//只能在一个cpp文件定义STB_IMPLEMENTATION
//#define STB_IMPLEMENTATION
//https://github.com/nothings/stb/
//#define STB_C_LEXER_IMPLEMENTATION
#define STB_IMPLEMENTATION
#define _VORBIS_SRC


#ifndef __stb_inc_h__
#define __stb_inc_h__

//#ifndef nSTB_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#define STB_IMAGE_RESIZE_IMPLEMENTATION
//#define STB_TRUETYPE_IMPLEMENTATION
//#define STB_RECT_PACK_IMPLEMENTATION
//#define STB_ONLY
//#endif
//#include "stb/stb_image.h"
//#include "stb/stb_image_resize.h"
//#include "stb/stb_image_write.h"
//#include "stb/stb_rect_pack.h"
//#include "stb/stb_truetype.h"
//#include "stb/stb_vorbis.h"
#endif // !__stb_inc_h__

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#define STB_ONLY
#include "stb_image.h"
//#include "stb_image_resize.h"
#include "stb_image_write.h"
#include "stb_rect_pack.h"
#include "stb_truetype.h"
#ifdef _VORBIS_SRC
#include "stb_vorbis.c"
#endif

#include "stbi_DDS_aug.h"
#include "stbi_DDS_aug_c.h"

#ifdef STB_IMPLEMENTATION12
static void* stbi__load_gif_main1(stbi__context* s, int** delays, int* x, int* y, int* z, int* comp, int req_comp, int* nsize)
{
	if (stbi__gif_test(s)) {
		int layers = 0;
		stbi_uc* u = 0;
		stbi_uc* out = 0;
		stbi_uc* two_back = 0;
		stbi__gif g;
		int stride;
		int out_size = 0;
		int delays_size = 0;
		STBI_NOTUSED(out_size);
		STBI_NOTUSED(delays_size);

		memset(&g, 0, sizeof(g));
		if (delays) {
			*delays = 0;
		}

		do {
			u = stbi__gif_load_next(s, &g, comp, req_comp, two_back);
			if (u == (stbi_uc*)s) {
				if (nsize)*nsize = s->img_buffer - s->img_buffer_original;
				u = 0;  // end of animated gif marker
			}
			if (u) {
				*x = g.w;
				*y = g.h;
				++layers;
				stride = g.w * g.h * 4;

				if (out) {
					void* tmp = (stbi_uc*)STBI_REALLOC_SIZED(out, out_size, layers * stride);
					if (!tmp)
						return stbi__load_gif_main_outofmem(&g, out, delays);
					else {
						out = (stbi_uc*)tmp;
						out_size = layers * stride;
					}

					if (delays) {
						int* new_delays = (int*)STBI_REALLOC_SIZED(*delays, delays_size, sizeof(int) * layers);
						if (!new_delays)
							return stbi__load_gif_main_outofmem(&g, out, delays);
						*delays = new_delays;
						delays_size = layers * sizeof(int);
					}
				}
				else {
					out = (stbi_uc*)stbi__malloc(layers * stride);
					if (!out)
						return stbi__load_gif_main_outofmem(&g, out, delays);
					out_size = layers * stride;
					if (delays) {
						*delays = (int*)stbi__malloc(layers * sizeof(int));
						if (!*delays)
							return stbi__load_gif_main_outofmem(&g, out, delays);
						delays_size = layers * sizeof(int);
					}
				}
				memcpy(out + ((layers - 1) * stride), u, stride);
				if (layers >= 2) {
					two_back = out - 2 * stride;
				}

				if (delays) {
					(*delays)[layers - 1U] = g.delay;
				}
			}
		} while (u != 0);

		// free temp buffer;
		STBI_FREE(g.out);
		STBI_FREE(g.history);
		STBI_FREE(g.background);

		// do the final conversion after loading everything;
		if (req_comp && req_comp != 4)
			out = stbi__convert_format(out, 4, req_comp, layers * g.w, g.h);

		*z = layers;
		return out;
	}
	else {
		return stbi__errpuc("not GIF", "Image was not as a gif type.");
	}
}
STBIDEF stbi_uc* stbi_load_gif_from_memory1(stbi_uc const* buffer, int len, int** delays, int* x, int* y, int* z, int* comp, int req_comp, int* nsize)
{
	unsigned char* result;
	stbi__context s;
	stbi__start_mem(&s, buffer, len);

	result = (unsigned char*)stbi__load_gif_main1(&s, delays, x, y, z, comp, req_comp, nsize);
	if (stbi__vertically_flip_on_load) {
		stbi__vertical_flip_slices(result, *x, *y, *z, *comp);
	}

	return result;
}
#endif


#ifdef no_STBIMP

struct active_edge_t
{
	struct active_edge_t* next;
	float fx, fdx, fdy;
	float direction;
	float sy;
	float ey;
};
typedef struct active_edge_t active_edge_t;
active_edge_t* new_active(stbtt__hheap* hh, stbtt__edge* e, int off_x, float start_point, void* userdata)
{
	active_edge_t* z = (active_edge_t*)stbtt__hheap_alloc(hh, sizeof(*z), userdata);
	float dxdy = (e->x1 - e->x0) / (e->y1 - e->y0);
	STBTT_assert(z != NULL);
	//STBTT_assert(e->y0 <= start_point);
	if (!z) return z;
	z->fdx = dxdy;
	z->fdy = dxdy != 0.0f ? (1.0f / dxdy) : 0.0f;
	z->fx = e->x0 + dxdy * (start_point - e->y0);
	z->fx -= off_x;
	z->direction = e->invert ? 1.0f : -1.0f;
	z->sy = e->y0;
	z->ey = e->y1;
	z->next = 0;
	return z;
}static void  handle_clipped_edge(float* scanline, int x, active_edge_t* e, float x0, float y0, float x1, float y1)
{
	if (y0 == y1) return;
	STBTT_assert(y0 < y1);
	STBTT_assert(e->sy <= e->ey);
	if (y0 > e->ey) return;
	if (y1 < e->sy) return;
	if (y0 < e->sy) {
		x0 += (x1 - x0) * (e->sy - y0) / (y1 - y0);
		y0 = e->sy;
	}
	if (y1 > e->ey) {
		x1 += (x1 - x0) * (e->ey - y1) / (y1 - y0);
		y1 = e->ey;
	}

	if (x0 == x)
		STBTT_assert(x1 <= x + 1);
	else if (x0 == x + 1)
		STBTT_assert(x1 >= x);
	else if (x0 <= x)
		STBTT_assert(x1 <= x);
	else if (x0 >= x + 1)
		STBTT_assert(x1 >= x + 1);
	else
		STBTT_assert(x1 >= x && x1 <= x + 1);

	if (x0 <= x && x1 <= x)
		scanline[x] += e->direction * (y1 - y0);
	else if (x0 >= x + 1 && x1 >= x + 1)
		;
	else {
		STBTT_assert(x0 >= x && x0 <= x + 1 && x1 >= x && x1 <= x + 1);
		scanline[x] += e->direction * (y1 - y0) * (1 - ((x0 - x) + (x1 - x)) / 2); // coverage = 1 - average x position
	}
}

static float  sized_trapezoid_area(float height, float top_width, float bottom_width)
{
	STBTT_assert(top_width >= 0);
	STBTT_assert(bottom_width >= 0);
	return (top_width + bottom_width) / 2.0f * height;
}

static float  position_trapezoid_area(float height, float tx0, float tx1, float bx0, float bx1)
{
	return stbtt__sized_trapezoid_area(height, tx1 - tx0, bx1 - bx0);
}

static float sized_triangle_area(float height, float width)
{
	return height * width / 2;
}

static void  fill_active_edges_new(float* scanline, float* scanline_fill, int len, active_edge_t* e, float y_top)
{
	float y_bottom = y_top + 1;

	while (e) {
		// brute force every pixel

		// compute intersection points with top & bottom
		STBTT_assert(e->ey >= y_top);

		if (e->fdx == 0) {
			float x0 = e->fx;
			if (x0 < len) {
				if (x0 >= 0) {
					handle_clipped_edge(scanline, (int)x0, e, x0, y_top, x0, y_bottom);
					handle_clipped_edge(scanline_fill - 1, (int)x0 + 1, e, x0, y_top, x0, y_bottom);
				}
				else {
					handle_clipped_edge(scanline_fill - 1, 0, e, x0, y_top, x0, y_bottom);
				}
			}
		}
		else {
			float x0 = e->fx;
			float dx = e->fdx;
			float xb = x0 + dx;
			float x_top, x_bottom;
			float sy0, sy1;
			float dy = e->fdy;
			STBTT_assert(e->sy <= y_bottom && e->ey >= y_top);

			// compute endpoints of line segment clipped to this scanline (if the
			// line segment starts on this scanline. x0 is the intersection of the
			// line with y_top, but that may be off the line segment.
			if (e->sy > y_top) {
				x_top = x0 + dx * (e->sy - y_top);
				sy0 = e->sy;
			}
			else {
				x_top = x0;
				sy0 = y_top;
			}
			if (e->ey < y_bottom) {
				x_bottom = x0 + dx * (e->ey - y_top);
				sy1 = e->ey;
			}
			else {
				x_bottom = xb;
				sy1 = y_bottom;
			}

			if (x_top >= 0 && x_bottom >= 0 && x_top < len && x_bottom < len) {
				// from here on, we don't have to range check x values

				if ((int)x_top == (int)x_bottom) {
					float height;
					// simple case, only spans one pixel
					int x = (int)x_top;
					height = (sy1 - sy0) * e->direction;
					STBTT_assert(x >= 0 && x < len);
					scanline[x] += stbtt__position_trapezoid_area(height, x_top, x + 1.0f, x_bottom, x + 1.0f);
					scanline_fill[x] += height; // everything right of this pixel is filled
				}
				else {
					int x, x1, x2;
					float y_crossing, y_final, step, sign, area;
					// covers 2+ pixels
					if (x_top > x_bottom) {
						// flip scanline vertically; signed area is the same
						float t;
						sy0 = y_bottom - (sy0 - y_top);
						sy1 = y_bottom - (sy1 - y_top);
						t = sy0, sy0 = sy1, sy1 = t;
						t = x_bottom, x_bottom = x_top, x_top = t;
						dx = -dx;
						dy = -dy;
						t = x0, x0 = xb, xb = t;
					}
					STBTT_assert(dy >= 0);
					STBTT_assert(dx >= 0);

					x1 = (int)x_top;
					x2 = (int)x_bottom;
					// compute intersection with y axis at x1+1
					y_crossing = y_top + dy * (x1 + 1 - x0);

					// compute intersection with y axis at x2
					y_final = y_top + dy * (x2 - x0);

					//           x1    x_top                            x2    x_bottom
					//     y_top  +------|-----+------------+------------+--------|---+------------+
					//            |            |            |            |            |            |
					//            |            |            |            |            |            |
					//       sy0  |      Txxxxx|............|............|............|............|
					// y_crossing |            *xxxxx.......|............|............|............|
					//            |            |     xxxxx..|............|............|............|
					//            |            |     /-   xx*xxxx........|............|............|
					//            |            | dy <       |    xxxxxx..|............|............|
					//   y_final  |            |     \-     |          xx*xxx.........|............|
					//       sy1  |            |            |            |   xxxxxB...|............|
					//            |            |            |            |            |            |
					//            |            |            |            |            |            |
					//  y_bottom  +------------+------------+------------+------------+------------+
					//
					// goal is to measure the area covered by '.' in each pixel

					// if x2 is right at the right edge of x1, y_crossing can blow up, github #1057
					// @TODO: maybe test against sy1 rather than y_bottom?
					if (y_crossing > y_bottom)
						y_crossing = y_bottom;

					sign = e->direction;

					// area of the rectangle covered from sy0..y_crossing
					area = sign * (y_crossing - sy0);

					// area of the triangle (x_top,sy0), (x1+1,sy0), (x1+1,y_crossing)
					scanline[x1] += sized_triangle_area(area, x1 + 1 - x_top);

					// check if final y_crossing is blown up; no test case for this
					if (y_final > y_bottom) {
						y_final = y_bottom;
						dy = (y_final - y_crossing) / (x2 - (x1 + 1)); // if denom=0, y_final = y_crossing, so y_final <= y_bottom
					}

					// in second pixel, area covered by line segment found in first pixel
					// is always a rectangle 1 wide * the height of that line segment; this
					// is exactly what the variable 'area' stores. it also gets a contribution
					// from the line segment within it. the THIRD pixel will get the first
					// pixel's rectangle contribution, the second pixel's rectangle contribution,
					// and its own contribution. the 'own contribution' is the same in every pixel except
					// the leftmost and rightmost, a trapezoid that slides down in each pixel.
					// the second pixel's contribution to the third pixel will be the
					// rectangle 1 wide times the height change in the second pixel, which is dy.

					step = sign * dy * 1; // dy is dy/dx, change in y for every 1 change in x,
					// which multiplied by 1-pixel-width is how much pixel area changes for each step in x
					// so the area advances by 'step' every time

					for (x = x1 + 1; x < x2; ++x) {
						scanline[x] += area + step / 2; // area of trapezoid is 1*step/2
						area += step;
					}
					STBTT_assert(STBTT_fabs(area) <= 1.01f); // accumulated error from area += step unless we round step down
					STBTT_assert(sy1 > y_final - 0.01f);

					// area covered in the last pixel is the rectangle from all the pixels to the left,
					// plus the trapezoid filled by the line segment in this pixel all the way to the right edge
					scanline[x2] += area + sign * stbtt__position_trapezoid_area(sy1 - y_final, (float)x2, x2 + 1.0f, x_bottom, x2 + 1.0f);

					// the rest of the line is filled based on the total height of the line segment in this pixel
					scanline_fill[x2] += sign * (sy1 - sy0);
				}
			}
			else {
				// if edge goes outside of box we're drawing, we require
				// clipping logic. since this does not match the intended use
				// of this library, we use a different, very slow brute
				// force implementation
				// note though that this does happen some of the time because
				// x_top and x_bottom can be extrapolated at the top & bottom of
				// the shape and actually lie outside the bounding box
				int x;
				for (x = 0; x < len; ++x) {
					// cases:
					//
					// there can be up to two intersections with the pixel. any intersection
					// with left or right edges can be handled by splitting into two (or three)
					// regions. intersections with top & bottom do not necessitate case-wise logic.
					//
					// the old way of doing this found the intersections with the left & right edges,
					// then used some simple logic to produce up to three segments in sorted order
					// from top-to-bottom. however, this had a problem: if an x edge was epsilon
					// across the x border, then the corresponding y position might not be distinct
					// from the other y segment, and it might ignored as an empty segment. to avoid
					// that, we need to explicitly produce segments based on x positions.

					// rename variables to clearly-defined pairs
					float y0 = y_top;
					float x1 = (float)(x);
					float x2 = (float)(x + 1);
					float x3 = xb;
					float y3 = y_bottom;

					// x = e->x + e->dx * (y-y_top)
					// (y-y_top) = (x - e->x) / e->dx
					// y = (x - e->x) / e->dx + y_top
					float y1 = (x - x0) / dx + y_top;
					float y2 = (x + 1 - x0) / dx + y_top;

					if (x0 < x1 && x3 > x2) {         // three segments descending down-right
						handle_clipped_edge(scanline, x, e, x0, y0, x1, y1);
						handle_clipped_edge(scanline, x, e, x1, y1, x2, y2);
						handle_clipped_edge(scanline, x, e, x2, y2, x3, y3);
					}
					else if (x3 < x1 && x0 > x2) {  // three segments descending down-left
						handle_clipped_edge(scanline, x, e, x0, y0, x2, y2);
						handle_clipped_edge(scanline, x, e, x2, y2, x1, y1);
						handle_clipped_edge(scanline, x, e, x1, y1, x3, y3);
					}
					else if (x0 < x1 && x3 > x1) {  // two segments across x, down-right
						handle_clipped_edge(scanline, x, e, x0, y0, x1, y1);
						handle_clipped_edge(scanline, x, e, x1, y1, x3, y3);
					}
					else if (x3 < x1 && x0 > x1) {  // two segments across x, down-left
						handle_clipped_edge(scanline, x, e, x0, y0, x1, y1);
						handle_clipped_edge(scanline, x, e, x1, y1, x3, y3);
					}
					else if (x0 < x2 && x3 > x2) {  // two segments across x+1, down-right
						handle_clipped_edge(scanline, x, e, x0, y0, x2, y2);
						handle_clipped_edge(scanline, x, e, x2, y2, x3, y3);
					}
					else if (x3 < x2 && x0 > x2) {  // two segments across x+1, down-left
						handle_clipped_edge(scanline, x, e, x0, y0, x2, y2);
						handle_clipped_edge(scanline, x, e, x2, y2, x3, y3);
					}
					else {  // one segment
						handle_clipped_edge(scanline, x, e, x0, y0, x3, y3);
					}
				}
			}
		}
		e = e->next;
	}
}

static void rasterize_sorted_edges(stbtt__bitmap* result, stbtt__edge* e, int n, int vsubsample, int off_x, int off_y, void* userdata)
{
	stbtt__hheap hh = { 0, 0, 0 };
	active_edge_t* active = NULL;
	int y, j = 0, i;
	float scanline_data[129], * scanline, * scanline2;

	STBTT__NOTUSED(vsubsample);

	if (result->w > 64)
		scanline = (float*)STBTT_malloc((result->w * 2 + 1) * sizeof(float), userdata);
	else
		scanline = scanline_data;

	scanline2 = scanline + result->w;

	y = off_y;
	e[n].y0 = (float)(off_y + result->h) + 1;

	while (j < result->h) {
		// find center of pixel for this scanline
		float scan_y_top = y + 0.0f;
		float scan_y_bottom = y + 1.0f;
		active_edge_t** step = &active;

		STBTT_memset(scanline, 0, result->w * sizeof(scanline[0]));
		STBTT_memset(scanline2, 0, (result->w + 1) * sizeof(scanline[0]));

		// update all active edges;
		// remove all active edges that terminate before the top of this scanline
		while (*step) {
			active_edge_t* z = *step;
			if (z->ey <= scan_y_top) {
				*step = z->next; // delete from list
				STBTT_assert(z->direction);
				z->direction = 0;
				stbtt__hheap_free(&hh, z);
			}
			else {
				step = &((*step)->next); // advance through list
			}
		}

		// insert all edges that start before the bottom of this scanline
		while (e->y0 <= scan_y_bottom) {
			if (e->y0 != e->y1) {
				active_edge_t* z = new_active(&hh, e, off_x, scan_y_top, userdata);
				if (z != NULL) {
					if (j == 0 && off_y != 0) {
						if (z->ey < scan_y_top) {
							// this can happen due to subpixel positioning and some kind of fp rounding error i think
							z->ey = scan_y_top;
						}
					}
					STBTT_assert(z->ey >= scan_y_top); // if we get really unlucky a tiny bit of an edge can be out of bounds
					// insert at front
					z->next = active;
					active = z;
				}
			}
			++e;
		}

		// now process all active edges
		if (active)
			fill_active_edges_new(scanline, scanline2 + 1, result->w, active, scan_y_top);

		{
			float sum = 0;
			for (i = 0; i < result->w; ++i) {
				float k;
				int m;
				sum += scanline2[i];
				k = scanline[i] + sum;
				k = (float)STBTT_fabs(k) * 255 + 0.5f;
				m = (int)k;
				if (m > 255) m = 255;
				result->pixels[j * result->stride + i] = (unsigned char)m;
			}
		}
		// advance all the edges
		step = &active;
		while (*step) {
			active_edge_t* z = *step;
			z->fx += z->fdx; // advance to position for current scanline
			step = &((*step)->next); // advance through list
		}

		++y;
		++j;
	}

	stbtt__hheap_cleanup(&hh, userdata);

	if (scanline != scanline_data)
		STBTT_free(scanline, userdata);
}


void sort_edges(stbtt__edge* p, int n)
{
	stbtt__sort_edges(p, n);
}
stbtt__point* FlattenCurves(stbtt_vertex* vertices, int num_verts, float objspace_flatness, int** contour_lengths, int* num_contours, void* userdata)
{
	return stbtt_FlattenCurves(vertices, num_verts, objspace_flatness, contour_lengths, num_contours, userdata);
}
#endif // !no_STBIMP










// svg


#if (__has_include(<nanosvg.h>))
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"
#endif
