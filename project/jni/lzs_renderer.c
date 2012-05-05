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

#include "lzs_renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <GLES/glext.h>

#define LOG_TAG "LaserShark"
#include "a3d/a3d_log.h"

/***********************************************************
* private                                                  *
***********************************************************/

#define SCREEN_W 800.0f
#define SCREEN_H 480.0f

#define BALL_RADIUS  128.0f
#define LASER_RADIUS 64.0f

// tangent derivitives
static float SOBEL_X[] =
{
	-1.0f / 4.0f, 0.0f, 1.0f / 4.0f,
	-2.0f / 4.0f, 0.0f, 2.0f / 4.0f,
	-1.0f / 4.0f, 0.0f, 1.0f / 4.0f,
};

static float SOBEL_Y[] =
{
	 1.0f / 4.0f,  2.0f / 4.0f,  1.0f / 4.0f,
	        0.0f,         0.0f,         0.0f,
	-1.0f / 4.0f, -2.0f / 4.0f, -1.0f / 4.0f,
};

// 8 + 4*4 + 2*8 + 1*8 + 0.5*4 = 50
static float PEAK[] =
{
	0.5f / 50.0f, 1.0f / 50.0f, 2.0f / 50.0f, 1.0f / 50.0f, 0.5f / 50.0f,
	1.0f / 50.0f, 2.0f / 50.0f, 4.0f / 50.0f, 2.0f / 50.0f, 1.0f / 50.0f,
	2.0f / 50.0f, 4.0f / 50.0f, 8.0f / 50.0f, 4.0f / 50.0f, 2.0f / 50.0f,
	1.0f / 50.0f, 2.0f / 50.0f, 4.0f / 50.0f, 2.0f / 50.0f, 1.0f / 50.0f,
	0.5f / 50.0f, 1.0f / 50.0f, 2.0f / 50.0f, 1.0f / 50.0f, 0.5f / 50.0f,
};

static GLfloat BOX[] =
{
	0.0f, 0.0f, -1.0f,   // 0
	0.0f, 1.0f, -1.0f,   // 1
	1.0f, 1.0f, -1.0f,   // 2
	1.0f, 0.0f, -1.0f,   // 3
};

static GLfloat VERTEX[] =
{
	    0.0f,     0.0f, -1.0f,   // 0
	    0.0f, SCREEN_H, -1.0f,   // 1
	SCREEN_W, SCREEN_H, -1.0f,   // 2
	SCREEN_W,     0.0f, -1.0f,   // 3
};

static GLfloat COORDS[] =
{
	0.0f, 0.0f,   // 0
	0.0f, 1.0f,   // 1
	1.0f, 1.0f,   // 2
	1.0f, 0.0f,   // 3
};

/***********************************************************
* public                                                   *
***********************************************************/

lzs_renderer_t* lzs_renderer_new(const char* s)
{
	assert(s);
	LOGD("debug");

	lzs_renderer_t* self = (lzs_renderer_t*) malloc(sizeof(lzs_renderer_t));
	if(self == NULL)
	{
		LOGE("malloc failed");
		return NULL;
	}

	self->ball_x  = 400.0f;
	self->ball_y  = 240.0f;
	self->laser_x = 400.0f;
	self->laser_y = 240.0f;
	a3d_mat4f_identity(&self->phone_gyro);

	// allocate the buffer(s)
	GLint bsize  = 2 * ((int) BALL_RADIUS);
	GLint lsize  = 2 * ((int) LASER_RADIUS);
	GLint format = TEXGZ_BGRA;
	GLint type   = GL_UNSIGNED_BYTE;
	self->ball_buffer  = texgz_tex_new(bsize, bsize, bsize, bsize, type, format, NULL);
	if(self->ball_buffer == NULL)
	{
		goto fail_ball;
	}
	self->laser_buffer = texgz_tex_new(lsize, lsize, lsize, lsize, type, format, NULL);
	if(self->laser_buffer == NULL)
	{
		goto fail_laser;
	}

	glGenTextures(1, &self->texid);
	glEnableClientState(GL_VERTEX_ARRAY);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glDisable(GL_DEPTH_TEST);

	// success
	return self;

	// failure
	fail_laser:
		texgz_tex_delete(&self->ball_buffer);
	fail_ball:
		free(self);
	return NULL;
}

void lzs_renderer_delete(lzs_renderer_t** _self)
{
	assert(_self);

	lzs_renderer_t* self = *_self;
	if(self)
	{
		LOGD("debug");
		texgz_tex_delete(&self->laser_buffer);
		texgz_tex_delete(&self->ball_buffer);
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
		glDeleteTextures(1, &self->texid);
		free(self);
		*_self = NULL;
	}
}

void lzs_renderer_resize(lzs_renderer_t* self, int w, int h)
{
	assert(self);
	LOGI("%ix%i", w, h);

	glViewport(0, 0, w, h);
}

void lzs_renderer_drawbox(float top, float left, float bottom, float right, float r, float g, float b, int filled)
{
	LOGD("top=%f, left=%f, bottom=%f, right=%f, r=%f, g=%f, b=%f", top, left, bottom, right, r, g, b);

	BOX[0]  = left;
	BOX[1]  = top;
	BOX[3]  = left;
	BOX[4]  = bottom;
	BOX[6]  = right;
	BOX[7]  = bottom;
	BOX[9]  = right;
	BOX[10] = top;

	glColor4f(r, g, b, 1.0f);
	glVertexPointer(3, GL_FLOAT, 0, BOX);
	if(filled)
	{
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
	else
	{
		glDrawArrays(GL_LINE_LOOP, 0, 4);
	}
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void lzs_renderer_draw(lzs_renderer_t* self)
{
	assert(self);
	LOGD("debug");

	// stretch screen to 800x480
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0.0f, SCREEN_W, SCREEN_H, 0.0f, 0.0f, 2.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// draw camera
	glEnable(GL_TEXTURE_EXTERNAL_OES);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, self->texid);
	glVertexPointer(3, GL_FLOAT, 0, VERTEX);
	glTexCoordPointer(2, GL_FLOAT, 0, COORDS);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_EXTERNAL_OES);

	// capture buffers
	GLint format = TEXGZ_BGRA;
	GLint type   = GL_UNSIGNED_BYTE;
	glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES, &format);
	glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE_OES, &type);
	if((format == TEXGZ_BGRA) && (type == GL_UNSIGNED_BYTE))
	{
		LOGD("readpixels format=0x%X, type=0x%X", format, type);

		// TODO - check for texgz errors

		texgz_tex_t* b = self->ball_buffer;
		glReadPixels(self->ball_x - BALL_RADIUS, (SCREEN_H - self->ball_y - 1) - BALL_RADIUS,
		             b->width, b->height, b->format, b->type, (void*) b->pixels);
		texgz_tex_t* l = self->laser_buffer;
		glReadPixels(self->laser_x - LASER_RADIUS, (SCREEN_H - self->laser_y - 1) - LASER_RADIUS,
		             l->width, l->height, l->format, l->type, (void*) l->pixels);

		// process ball
		texgz_tex_t* bg  = texgz_tex_convertcopy(b, TEXGZ_FLOAT, TEXGZ_LUMINANCE);
		texgz_tex_t* bsx = texgz_tex_convolvecopy(bg, SOBEL_X, 3, 1);
		texgz_tex_t* bsy = texgz_tex_convolvecopy(bg, SOBEL_Y, 3, 1);
		//texgz_tex_export(b,   "/sdcard/laser-shark/ball.texgz");
		//texgz_tex_export(bg,  "/sdcard/laser-shark/ball-gray.texgz");
		//texgz_tex_export(bsx, "/sdcard/laser-shark/ball-sx.texgz");
		//texgz_tex_export(bsy, "/sdcard/laser-shark/ball-sy.texgz");

		// process laser
		texgz_tex_t* lg = texgz_tex_convertcopy(l, TEXGZ_FLOAT, TEXGZ_LUMINANCE);
		texgz_tex_t* lp = texgz_tex_convolvecopy(lg, PEAK, 5, 0);
		//texgz_tex_export(l, "/sdcard/laser-shark/laser.texgz");
		//texgz_tex_export(lg,  "/sdcard/laser-shark/laser-gray.texgz");
		//texgz_tex_export(lp,  "/sdcard/laser-shark/laser-peak.texgz");

		// compute laser peak
		int    x;
		int    y;
		int    peak_x  = 0;
		int    peak_y  = 0;
		float  peak    = 0.0f;
		float* fpixels = (float*) lp->pixels;
		for(x = 0; x < lp->width; ++x)
		{
			for(y = 0; y < lp->height; ++y)
			{
				if(fpixels[lp->width*y + x] > peak)
				{
					peak_x = x;
					peak_y = y;
					peak   = fpixels[lp->width*y + x];
				}
			}
		}

		// move laser center to match peak
		if(peak >= 0.5f)
		{
			self->laser_x += (float) peak_x - (float) l->width / 2.0f;
			self->laser_y -= (float) peak_y - (float) l->height / 2.0f;
		}

		texgz_tex_delete(&bg);
		texgz_tex_delete(&bsx);
		texgz_tex_delete(&bsy);
		texgz_tex_delete(&lg);
		texgz_tex_delete(&lp);
	}
	else
	{
		LOGE("unsupported format=0x%X, type=0x%X", format, type);
	}

	// draw laser search box
	{
		float x = self->laser_x;
		float y = self->laser_y;
		float r = LASER_RADIUS;
		lzs_renderer_drawbox(y - r, x - r, y + r, x + r, 1.0f, 0.0f, 0.0f, 0);
	}

	// draw ball search box
	{
		float x = self->ball_x;
		float y = self->ball_y;
		float r = BALL_RADIUS;
		lzs_renderer_drawbox(y - r, x - r, y + r, x + r, 0.0f, 1.0f, 0.0f, 0);
	}

	A3D_GL_GETERROR();
}

void lzs_renderer_searchlaser(lzs_renderer_t* self, float x, float y)
{
	assert(self);
	LOGD("debug x=%f, y=%f", x, y);

	// limit search region to screen
	float r = LASER_RADIUS;
	if(x - r < 0.0f)
	{
		x = r;
	}
	if(x + r >= SCREEN_W)
	{
		x = SCREEN_W - r - 1;
	}
	if(y - r < 0.0f)
	{
		y = r;
	}
	if(y + r >= SCREEN_H)
	{
		y = SCREEN_H - r - 1;
	}

	self->laser_x = x;
	self->laser_y = y;
}

void lzs_renderer_searchball(lzs_renderer_t* self, float x1, float y1, float x2, float y2)
{
	assert(self);
	LOGD("debug x=%f, y=%f, radius=%f", x, y, radius);

	// sort left-to-right and top-to-bottom
	if(x1 > x2)
	{
		float x = x1;
		x1 = x2;
		x2 = x;
	}
	if(y1 > y2)
	{
		float y = y1;
		y1 = y2;
		y2 = y;
	}

	float dx = x2 - x1;
	float dy = y2 - y1;
	float x  = x1 + dx / 2.0f;
	float y  = y1 + dy / 2.0f;
	float r  = BALL_RADIUS;

	// limit search region to screen
	if(x - r < 0.0f)
	{
		x = r;
	}
	if(x + r >= SCREEN_W)
	{
		x = SCREEN_W - r - 1;
	}
	if(y - r < 0.0f)
	{
		y = r;
	}
	if(y + r >= SCREEN_H)
	{
		y = SCREEN_H - r - 1;
	}

	self->ball_x = x;
	self->ball_y = y;
}

void lzs_renderer_phonegyro(lzs_renderer_t* self, float v1, float v2, float v3, float dt)
{
	assert(self);
	LOGD("v1=%f, v2=%f, v3=%f, dt=%f", v1, v2, v3, dt);

	// v1 is positive rotation about y
	// v2 is negative rotation about x
	// v3 is positive rotation about z
	//float ax = 0.0f;
	//float ay = 0.0f;
	//float az = 0.0f;
	//a3d_mat4f_rotate(&self->phone_gyro, 0, ax, 1.0f, 0.0f, 0.0f);
	//a3d_mat4f_rotate(&self->phone_gyro, 0, ay, 0.0f, 1.0f, 0.0f);
	//a3d_mat4f_rotate(&self->phone_gyro, 0, az, 0.0f, 0.0f, 1.0f);
}

void lzs_renderer_spheroorientation(lzs_renderer_t* self, float pitch, float roll, float yaw)
{
	assert(self);
	LOGI("pitch=%f, roll=%f, yaw=%f", pitch, roll, yaw);
}
