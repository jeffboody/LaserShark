/*
 * Copyright (c) 2012 Jeff Boody
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef lzs_renderer_H
#define lzs_renderer_H

#include "a3d/a3d_GL.h"
#include "a3d/math/a3d_mat4f.h"
#include "a3d/a3d_texfont.h"
#include "a3d/a3d_texstring.h"
#include "texgz/texgz_tex.h"

/***********************************************************
* public                                                   *
***********************************************************/

typedef struct
{
	// x, y are in pixels
	// X, Y, height are in feet
	GLuint       texid;
	float        sphero_x;
	float        sphero_y;
	float        sphero_X;
	float        sphero_Y;
	float        sphero_speed;
	float        sphero_heading;
	float        sphero_heading_offset;
	float        sphero_goal;
	float        phone_heading;
	float        phone_slope;
	float        phone_height;
	float        phone_X;
	float        phone_Y;

	// buffers for image processing
	texgz_tex_t* buffer_color;   // RGBA-8888 or BGRA-8888
	texgz_tex_t* buffer_gray;    // Luminance-FLOAT
	texgz_tex_t* buffer_sx;      // Luminance-FLOAT
	texgz_tex_t* buffer_sy;      // Luminance-FLOAT

	// string(s)
	a3d_texfont_t*   font;
	a3d_texstring_t* string_sphero;
	a3d_texstring_t* string_phone;

	// fps state
	a3d_texstring_t* string_fps;
	double           t0;
	int              frames;
} lzs_renderer_t;

lzs_renderer_t* lzs_renderer_new(const char* font);
void            lzs_renderer_delete(lzs_renderer_t** _self);
void            lzs_renderer_resize(lzs_renderer_t* self, int w, int h);
void            lzs_renderer_drawbox(float top, float left, float bottom, float right, float r, float g, float b, int filled);
void            lzs_renderer_draw(lzs_renderer_t* self);
void            lzs_renderer_searchsphero(lzs_renderer_t* self, float x, float y);
void            lzs_renderer_calibratesphero(lzs_renderer_t* self, float x1, float y1, float x2, float y2);
void            lzs_renderer_spheroorientation(lzs_renderer_t* self, float pitch, float roll, float yaw);
void            lzs_renderer_phoneorientation(lzs_renderer_t* self, float pitch, float roll, float yaw);
int             lzs_renderer_spheroheading(lzs_renderer_t* self);
float           lzs_renderer_spherospeed(lzs_renderer_t* self);

#endif
