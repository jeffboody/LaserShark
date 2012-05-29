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
#include <math.h>
#include <GLES/glext.h>
#include "a3d/a3d_time.h"

#define LOG_TAG "LaserShark"
#include "a3d/a3d_log.h"

/***********************************************************
* private                                                  *
***********************************************************/

#define SCREEN_W  800.0f
#define SCREEN_H  480.0f
#define SCREEN_CX 400.0f
#define SCREEN_CY 240.0f

#define RADIUS_BALL  64.0f
#define RADIUS_CROSS 24.0f

//#define DEBUG_TIME
//#define DEBUG_BUFFERS

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

static float fix_angle(float angle)
{
	while(angle >= 360.0f)
	{
		angle -= 360.0f;
	}
	while(angle < 0.0f)
	{
		angle += 360.0f;
	}
	return angle;
}

static void lzs_renderer_step(lzs_renderer_t* self)
{
	assert(self);
	LOGD("debug");

	double t     = a3d_utime();
	double dt0   = t - self->t0;
	++self->frames;

	// don't update fps every frame
	if(dt0 >= 1.0 * A3D_USEC)
	{
		double seconds = dt0 / A3D_USEC;
		double fps     = (double) self->frames / seconds;

		// LOGI("%i frames in %.2lf seconds = %.2lf FPS", self->frames, seconds, fps);
		a3d_texstring_printf(self->string_fps, "%i fps", (int) fps);

		self->t0     = t;
		self->frames = 0;
	}
}

/***********************************************************
* public                                                   *
***********************************************************/

lzs_renderer_t* lzs_renderer_new(const char* font)
{
	assert(s);
	LOGD("debug");

	lzs_renderer_t* self = (lzs_renderer_t*) malloc(sizeof(lzs_renderer_t));
	if(self == NULL)
	{
		LOGE("malloc failed");
		return NULL;
	}

	self->sphero_x              = 400.0f;
	self->sphero_y              = 240.0f;
	self->sphero_X              = 0.0f;
	self->sphero_Y              = 0.0f;
	self->sphero_speed          = 0.0f;
	self->sphero_heading        = 0.0f;
	self->sphero_heading_offset = 0.0f;
	self->sphero_goal           = 0.0f;
	self->phone_heading         = 0.0f;
	self->phone_slope           = 0.0f;
	self->phone_height          = 5.0f;
	self->t0                    = a3d_utime();
	self->frames                = 0;

	// allocate the buffer(s)
	GLint bsize         = 2 * ((int) RADIUS_BALL);
	GLint format        = TEXGZ_BGRA;
	GLint type          = TEXGZ_UNSIGNED_BYTE;
	self->buffer_color = texgz_tex_new(bsize, bsize, bsize, bsize, type, format, NULL);
	if(self->buffer_color == NULL)
	{
		goto fail_color;
	}
	format = TEXGZ_LUMINANCE;
	type   = TEXGZ_FLOAT;
	self->buffer_gray = texgz_tex_new(bsize, bsize, bsize, bsize, type, format, NULL);
	if(self->buffer_gray == NULL)
	{
		goto fail_gray;
	}
	self->buffer_sx = texgz_tex_new(bsize, bsize, bsize, bsize, type, format, NULL);
	if(self->buffer_sx == NULL)
	{
		goto fail_sx;
	}
	self->buffer_sy = texgz_tex_new(bsize, bsize, bsize, bsize, type, format, NULL);
	if(self->buffer_sy == NULL)
	{
		goto fail_sy;
	}

	// create the font
	self->font = a3d_texfont_new(font);
	if(self->font == NULL)
	{
		goto fail_font;
	}

	// create the string(s)
	self->string_sphero = a3d_texstring_new(self->font, 64, 24, A3D_TEXSTRING_TOP_CENTER, 1.0f, 1.0f, 0.235f, 1.0f);
	if(self->string_sphero == NULL)
	{
		goto fail_string_sphero;
	}
	self->string_phone = a3d_texstring_new(self->font, 64, 24, A3D_TEXSTRING_TOP_CENTER, 1.0f, 1.0f, 0.235f, 1.0f);
	if(self->string_phone == NULL)
	{
		goto fail_string_phone;
	}
	self->string_fps = a3d_texstring_new(self->font, 16, 24, A3D_TEXSTRING_BOTTOM_RIGHT, 1.0f, 1.0f, 0.235f, 1.0f);
	if(self->string_fps == NULL)
	{
		goto fail_string_fps;
	}

	glGenTextures(1, &self->texid);
	glEnableClientState(GL_VERTEX_ARRAY);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glDisable(GL_DEPTH_TEST);

	// success
	return self;

	// failure
	fail_string_fps:
		a3d_texstring_delete(&self->string_phone);
	fail_string_phone:
		a3d_texstring_delete(&self->string_sphero);
	fail_string_sphero:
		a3d_texfont_delete(&self->font);
	fail_font:
		texgz_tex_delete(&self->buffer_sy);
	fail_sy:
		texgz_tex_delete(&self->buffer_sx);
	fail_sx:
		texgz_tex_delete(&self->buffer_gray);
	fail_gray:
		texgz_tex_delete(&self->buffer_color);
	fail_color:
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
		a3d_texstring_delete(&self->string_fps);
		a3d_texstring_delete(&self->string_phone);
		a3d_texstring_delete(&self->string_sphero);
		a3d_texfont_delete(&self->font);
		texgz_tex_delete(&self->buffer_sy);
		texgz_tex_delete(&self->buffer_sx);
		texgz_tex_delete(&self->buffer_gray);
		texgz_tex_delete(&self->buffer_color);
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

void lzs_renderer_crosshair(float top, float left, float bottom, float right, float r, float g, float b)
{
	LOGD("top=%f, left=%f, bottom=%f, right=%f, r=%f, g=%f, b=%f", top, left, bottom, right, r, g, b);

	BOX[0]  = left + (right - left) / 2.0f;
	BOX[1]  = top;
	BOX[3]  = left + (right - left) / 2.0f;
	BOX[4]  = bottom;
	BOX[6]  = left;
	BOX[7]  = top + (bottom - top) / 2.0f;
	BOX[9]  = right;
	BOX[10] = top + (bottom - top) / 2.0f;

	glColor4f(r, g, b, 1.0f);
	glVertexPointer(3, GL_FLOAT, 0, BOX);
	glDrawArrays(GL_LINES, 0, 4);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void limit_position(float r, float* x, float* y)
{
	// limit search region to screen
	if(*x - r < 0.0f)
	{
		*x = r;
	}
	if(*x + r >= SCREEN_W)
	{
		*x = SCREEN_W - r - 1;
	}
	if(*y - r < 0.0f)
	{
		*y = r;
	}
	if(*y + r >= SCREEN_H)
	{
		*y = SCREEN_H - r - 1;
	}
}

static void utime_update(const char* name, double* _t0)
{
	double t1 = a3d_utime();
	#ifdef DEBUG_TIME
	LOGI("%s dt=%lf", name, t1 - *_t0);
	#endif
	*_t0 = t1;
}

static void compute_position(lzs_renderer_t* self, float x, float y, float* X, float* Y)
{
	assert(self);

	float h      = self->phone_height;
	float ph     = self->phone_heading * M_PI / 180.0f;
	float ps     = self->phone_slope   * M_PI / 180.0f;
	float scalew = 27.7f * M_PI / 180.0f;
	float scaleh = 19.0f * M_PI / 180.0f;
	float aph    = ph + scalew * (x - SCREEN_CX) / SCREEN_CX;
	float aps    = ps - scaleh * (y - SCREEN_CY) / SCREEN_CY;
	float aphyp  = h * tanf(aps);     // hypotenuse
	float apX    = aphyp * sinf(aph);  // X
	float apY    = aphyp * cosf(aph);  // Y
	*X           = apX;
	*Y           = apY;

	LOGD("x=%f, y=%f, apX=%f, apY=%f", x, y, apX, apY);
}

void lzs_renderer_draw(lzs_renderer_t* self)
{
	assert(self);
	LOGD("debug");

	float speed = 0.4f;

	double t0 = a3d_utime();

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
	utime_update("setup", &t0);

	// capture buffers
	GLint format = TEXGZ_BGRA;
	GLint type   = GL_UNSIGNED_BYTE;
	glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES, &format);
	glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE_OES, &type);
	if((format == TEXGZ_BGRA) && (type == GL_UNSIGNED_BYTE))
	{
		LOGD("readpixels format=0x%X, type=0x%X", format, type);

		// TODO - check for texgz errors

		// process buffers
		texgz_tex_t* bc  = self->buffer_color;
		texgz_tex_t* bg  = self->buffer_gray;
		texgz_tex_t* bsx = self->buffer_sx;
		texgz_tex_t* bsy = self->buffer_sy;
		glReadPixels(self->sphero_x - RADIUS_BALL, (SCREEN_H - self->sphero_y - 1) - RADIUS_BALL,
		             bc->width, bc->height, bc->format, bc->type, (void*) bc->pixels);
		#ifdef DEBUG_BUFFERS
			texgz_tex_export(bc, "/sdcard/laser-shark/color.texgz");
		#endif
		utime_update("readpixels", &t0);

		texgz_tex_computegray(bc, bg);
		utime_update("computegray", &t0);

		texgz_tex_computeedges3x3(bg, bsx, bsy);
		utime_update("computeedges", &t0);

		// compute peak
		{
			int    x;
			int    y;
			int    peak_x  = 0;
			int    peak_y  = 0;
			float  peak    = 0.0f;
			float* gpixels = (float*) bg->pixels;
			float* xpixels = (float*) bsx->pixels;
			float* ypixels = (float*) bsy->pixels;
			for(x = 0; x < bg->width; ++x)
			{
				for(y = 0; y < bg->height; ++y)
				{
					int idx      = bg->width*y + x;
					// compute magnitude squared
					float magsq = xpixels[idx]*xpixels[idx] + ypixels[idx]*ypixels[idx];
					gpixels[idx] = magsq;
					if(magsq > peak)
					{
						peak_x = x;
						peak_y = y;
						peak   = magsq;
					}
				}
			}
			LOGD("peak=%f, peak_x=%i, peak_y=%i", peak, peak_x, peak_y);
			utime_update("computepeak", &t0);
			#ifdef DEBUG_BUFFERS
				texgz_tex_export(bg, "/sdcard/laser-shark/peak.texgz");
			#endif

			// move sphero center to match peak
			self->sphero_x += (float) peak_x - (float) bg->width / 2.0f;
			self->sphero_y -= (float) peak_y - (float) bg->height / 2.0f;
		}
	}
	else
	{
		LOGE("unsupported format=0x%X, type=0x%X", format, type);
	}

	// compute phone X, Y center
	compute_position(self, SCREEN_CX, SCREEN_CY, &self->phone_X, &self->phone_Y);
	speed = 0.4f;

	// compute sphero X, Y
	compute_position(self, self->sphero_x, self->sphero_y, &self->sphero_X, &self->sphero_Y);
	utime_update("computeposition", &t0);

	// compute goal
	if(speed > 0.0f)
	{
		float dx           = self->phone_X - self->sphero_X;
		float dy           = self->phone_Y - self->sphero_Y;
		float a            = fix_angle(atan2f(dx, dy) * 180.0f / M_PI);
		self->sphero_goal  = a - self->sphero_heading_offset;
		self->sphero_speed = speed;
	}
	else
	{
		self->sphero_goal = 0.0f;
		self->sphero_speed = 0.0f;
	}

	// draw camera cross-hair
	{
		float x = SCREEN_CX;
		float y = SCREEN_CY;
		float r = RADIUS_CROSS;
		limit_position(r, &x, &y);
		lzs_renderer_crosshair(y - r, x - r, y + r, x + r, 1.0f, 0.0f, 0.0f);
	}

	// draw sphero search box
	{
		float r = RADIUS_BALL;
		limit_position(r, &self->sphero_x, &self->sphero_y);
		float x = self->sphero_x;
		float y = self->sphero_y;
		lzs_renderer_drawbox(y - r, x - r, y + r, x + r, 0.0f, 1.0f, 0.0f, 0);
	}

	lzs_renderer_step(self);

	// draw string
	a3d_texstring_printf(self->string_sphero, "sphero: head=%i, x=%0.1f, y=%0.1f, spd=%0.2f, goal=%i", (int) fix_angle(self->sphero_heading + self->sphero_heading_offset), self->sphero_X, self->sphero_Y, self->sphero_speed, (int) fix_angle(self->sphero_goal));
	a3d_texstring_printf(self->string_phone, "phone: heading=%i, slope=%i, x=%0.1f, y=%0.1f", (int) fix_angle(self->phone_heading), (int) fix_angle(self->phone_slope), self->phone_X, self->phone_Y);
	a3d_texstring_draw(self->string_sphero, 400.0f, 16.0f, 800, 480);
	a3d_texstring_draw(self->string_phone,  400.0f, 16.0f + self->string_sphero->size, 800, 480);
	a3d_texstring_draw(self->string_fps, (float) SCREEN_W - 16.0f, (float) SCREEN_H - 16.0f, SCREEN_W, SCREEN_H);
	utime_update("draw", &t0);

	//texgz_tex_t* screen = texgz_tex_new(SCREEN_W, SCREEN_H, SCREEN_W, SCREEN_H, TEXGZ_UNSIGNED_BYTE, TEXGZ_BGRA, NULL);
	//glReadPixels(0, 0, screen->width, screen->height, screen->format, screen->type, (void*) screen->pixels);
	//texgz_tex_export(screen, "/sdcard/laser-shark/screen.texgz");
	//texgz_tex_delete(&screen);

	A3D_GL_GETERROR();
}

void lzs_renderer_searchsphero(lzs_renderer_t* self, float x, float y)
{
	assert(self);
	LOGD("debug x=%f, y=%f", x, y);

	limit_position(RADIUS_BALL, &x, &y);
	self->sphero_x = x;
	self->sphero_y = y;
}

void lzs_renderer_calibratesphero(lzs_renderer_t* self, float x1, float y1, float x2, float y2)
{
	assert(self);
	LOGD("debug x=%f, y=%f, radius=%f", x, y, radius);

	// try to adjust sphero heading to match compass
	self->sphero_heading_offset = self->phone_heading - self->sphero_heading;
}

void lzs_renderer_spheroorientation(lzs_renderer_t* self, float pitch, float roll, float yaw)
{
	assert(self);
	LOGD("pitch=%f, roll=%f, yaw=%f", pitch, roll, yaw);
	self->sphero_heading = -yaw;
}

void lzs_renderer_phoneorientation(lzs_renderer_t* self, float pitch, float roll, float yaw)
{
	assert(self);
	LOGD("pitch=%f, roll=%f, yaw=%f", pitch, roll, yaw);
	self->phone_heading = yaw;
	self->phone_slope   = 360.0f - roll;
}

int lzs_renderer_spheroheading(lzs_renderer_t* self)
{
	assert(self);
	LOGD("debug");

	return self->sphero_goal;
}

float lzs_renderer_spherospeed(lzs_renderer_t* self)
{
	assert(self);
	LOGD("debug");

	return self->sphero_speed;
}
