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
#define LASER_RADIUS 32.0f

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
	float        laser_x;
	float        laser_y;
	texgz_tex_t* buffer;   // may be NULL
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

	self->ball_x       = 400.0f;
	self->ball_y       = 240.0f;
	self->laser_x      = 400.0f;
	self->laser_y      = 240.0f;

	self->buffer = NULL;
	glGenTextures(1, &self->texid);
	glEnableClientState(GL_VERTEX_ARRAY);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glDisable(GL_DEPTH_TEST);

	return self;
}

void lzs_renderer_delete(lzs_renderer_t** _self)
{
	assert(_self);

	lzs_renderer_t* self = *_self;
	if(self)
	{
		LOGD("debug");
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
		glDeleteTextures(1, &self->texid);
		texgz_tex_delete(&self->buffer);
		free(self);
		*_self = NULL;
	}
}

void lzs_renderer_resize(lzs_renderer_t* self, int w, int h)
{
	assert(self);
	LOGI("%ix%i", w, h);

	glViewport(0, 0, w, h);

	// reallocate the glReadPixels buffer
	GLint format = GL_RGB;
	GLint type   = GL_UNSIGNED_SHORT_5_6_5;
	glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES, &format);
	glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE_OES, &type);
	texgz_tex_delete(&self->buffer);
	self->buffer = texgz_tex_new(w, h, w, h, type, format, NULL);
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

	// take snapshot
	if(self->buffer)
	{
		texgz_tex_t* b = self->buffer;
		glReadPixels(0, 0, b->width, b->height, b->format, b->type, (void*) b->pixels);
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
	float r = BALL_RADIUS;

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

	self->ball_x      = x;
	self->ball_y      = y;
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
