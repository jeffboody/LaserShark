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

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "a3d/a3d_GL.h"
#include "texgz/texgz_tex.h"
#include <GLES/glext.h>
#include <math.h>

#define LOG_TAG "LaserShark"
#include "a3d/a3d_log.h"

/***********************************************************
* private                                                  *
***********************************************************/

#define SCREEN_W 800.0f
#define SCREEN_H 480.0f

#define BALL_RADIUS  128.0f
#define LASER_RADIUS 64.0f

// 8 + 4*4 + 2*8 + 1*8 + 0.5*4 = 50
static float MASK[] =
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

typedef struct
{
	GLuint       texid;
	float        ball_x;
	float        ball_y;
	texgz_tex_t* ball_buffer;
	float        laser_x;
	float        laser_y;
	texgz_tex_t* laser_buffer;
} lzs_renderer_t;

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

		texgz_tex_t* b = self->ball_buffer;
		glReadPixels(self->ball_x - BALL_RADIUS, (SCREEN_H - self->ball_y - 1) - BALL_RADIUS,
		             b->width, b->height, b->format, b->type, (void*) b->pixels);
		//if(texgz_tex_export(b, "/sdcard/laser-shark/ball.texgz") == 0)
		//{
		//	LOGE("texgz_tex_export failed");
		//}
		texgz_tex_t* l = self->laser_buffer;
		glReadPixels(self->laser_x - LASER_RADIUS, (SCREEN_H - self->laser_y - 1) - LASER_RADIUS,
		             l->width, l->height, l->format, l->type, (void*) l->pixels);
		//if(texgz_tex_export(l, "/sdcard/laser-shark/laser.texgz") == 0)
		//{
		//	LOGE("texgz_tex_export failed");
		//}

		// convolve the mask with the laser buffer
		int x;
		int y;
		int i;
		int j;
		int w = l->width;
		GLint format = TEXGZ_LUMINANCE;
		GLint type   = GL_UNSIGNED_BYTE;
		texgz_tex_t* s = texgz_tex_convertcopy(l, type, format);
		texgz_tex_t* m = texgz_tex_new(w, w, w, w, type, format, NULL);
		for(x = 2; x < l->width - 3; ++x)
		{
			for(y = 2; y < l->width - 3; ++y)
			{
				for(i = 0; i < 5; ++i)
				{
					for(j = 0; j < 5; ++j)
					{
						int row = y + i - 2;
						int col = x + j - 2;
						m->pixels[w*y + x] += s->pixels[w*row + col] * MASK[5*i + j];
					}
				}
			}
		}
		//if(texgz_tex_export(s, "/sdcard/laser-shark/laser-gray.texgz") == 0)
		//{
		//	LOGE("texgz_tex_export failed");
		//}
		//if(texgz_tex_export(m, "/sdcard/laser-shark/laser-peak.texgz") == 0)
		//{
		//	LOGE("texgz_tex_export failed");
		//}
		int   peak_x = 0;
		int   peak_y = 0;
		float peak   = 0.0f;
		for(x = 2; x < l->width - 3; ++x)
		{
			for(y = 2; y < l->width - 3; ++y)
			{
				if(m->pixels[w*y + x] > peak)
				{
					peak_x = x;
					peak_y = y;
					peak   = m->pixels[w*y + x];
				}
			}
		}
		// rotate and flip due to read pixels
		LOGI("peak_x=%i, peak_y=%i, peak=%f", peak_x, peak_y, peak);
		if(peak >= 128.0f)
		{
			self->laser_x += (float) peak_x - (float) l->width / 2.0f;
			self->laser_y -= (float) peak_y - (float) l->height / 2.0f;
		}
		texgz_tex_delete(&s);
		texgz_tex_delete(&m);
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
	//{
	//	float x = self->ball_x;
	//	float y = self->ball_y;
	//	float r = BALL_RADIUS;
	//	lzs_renderer_drawbox(y - r, x - r, y + r, x + r, 0.0f, 1.0f, 0.0f, 0);
	//}

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

static lzs_renderer_t* lzs_renderer = NULL;

/***********************************************************
* public                                                   *
***********************************************************/

JNIEXPORT void JNICALL Java_com_jeffboody_a3d_A3DNativeRenderer_NativeCreate(JNIEnv* env)
{
	assert(env);
	LOGD("debug");

	if(lzs_renderer != NULL)
	{
		LOGE("renderer already exists");
		return;
	}

	if(a3d_GL_load() == 0)
	{
		LOGE("a3d_GL_load failed");
		return;
	}

	lzs_renderer = lzs_renderer_new("/data/data/com.jeffboody.LaserShark/files/whitrabt.tex.gz");
}

JNIEXPORT void JNICALL Java_com_jeffboody_a3d_A3DNativeRenderer_NativeDestroy(JNIEnv* env)
{
	assert(env);
	LOGD("debug");

	if(lzs_renderer)
	{
		lzs_renderer_delete(&lzs_renderer);
		a3d_GL_unload();
	}
}

JNIEXPORT void JNICALL Java_com_jeffboody_a3d_A3DNativeRenderer_NativeChangeSurface(JNIEnv* env, jobject  obj, jint w, jint h)
{
	assert(env);
	LOGD("debug");

	if(lzs_renderer)
	{
		lzs_renderer_resize(lzs_renderer, w, h);
	}
}

JNIEXPORT void JNICALL Java_com_jeffboody_a3d_A3DNativeRenderer_NativeDraw(JNIEnv* env)
{
	assert(env);
	LOGD("debug");

	if(lzs_renderer)
	{
		a3d_GL_frame_begin();
		lzs_renderer_draw(lzs_renderer);
		a3d_GL_frame_end();
	}
}

JNIEXPORT int JNICALL Java_com_jeffboody_a3d_A3DNativeRenderer_NativeClientVersion(JNIEnv* env)
{
	assert(env);
	LOGD("debug");
	return 1;
}

JNIEXPORT int JNICALL Java_com_jeffboody_LaserShark_LaserSharkRenderer_NativeGetTexture(JNIEnv* env)
{
	assert(env);
	LOGD("debug");

	if(lzs_renderer)
	{
		return lzs_renderer->texid;
	}
	return 0;
}

JNIEXPORT void JNICALL Java_com_jeffboody_LaserShark_LaserShark_NativeTouchOne(JNIEnv* env, jobject obj, jfloat x1, jfloat y1)
{
	assert(env);
	LOGD("debug x1=%f, y1=%f", x1, y1);

	if(lzs_renderer)
	{
		lzs_renderer_searchlaser(lzs_renderer, x1, y1);
	}
}

JNIEXPORT void JNICALL Java_com_jeffboody_LaserShark_LaserShark_NativeTouchTwo(JNIEnv* env, jobject obj, jfloat x1, jfloat y1, jfloat x2, jfloat y2)
{
	assert(env);
	LOGD("debug x1=%f, y1=%f, x2=%f, y2=%f", x1, y1, x2, y2);

	// TODO - match virtual screen with physical screen
	if(lzs_renderer)
	{
		lzs_renderer_searchball(lzs_renderer, x1, y1, x2, y2);
	}
}
