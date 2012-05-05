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
#include "texgz/texgz_tex.h"

/***********************************************************
* public                                                   *
***********************************************************/

typedef struct
{
	GLuint       texid;
	float        ball_x;
	float        ball_y;
	texgz_tex_t* ball_buffer;
	float        laser_x;
	float        laser_y;
	texgz_tex_t* laser_buffer;
	a3d_mat4f_t  phone_gyro;
} lzs_renderer_t;

lzs_renderer_t* lzs_renderer_new(const char* s);
void            lzs_renderer_delete(lzs_renderer_t** _self);
void            lzs_renderer_resize(lzs_renderer_t* self, int w, int h);
void            lzs_renderer_drawbox(float top, float left, float bottom, float right, float r, float g, float b, int filled);
void            lzs_renderer_draw(lzs_renderer_t* self);
void            lzs_renderer_searchlaser(lzs_renderer_t* self, float x, float y);
void            lzs_renderer_searchball(lzs_renderer_t* self, float x1, float y1, float x2, float y2);
void            lzs_renderer_phonegyro(lzs_renderer_t* self, float v1, float v2, float v3, float dt);
void            lzs_renderer_spheroorientation(lzs_renderer_t* self, float pitch, float roll, float yaw);

#endif
