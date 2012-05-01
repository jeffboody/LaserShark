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

#define LOG_TAG "LaserShark"
#include "a3d/a3d_log.h"

/***********************************************************
* private                                                  *
***********************************************************/

typedef struct
{
	int stub;
} lzs_renderer_t;

lzs_renderer_t* lzs_renderer_new(const char* s)
{
	return NULL;
}

void lzs_renderer_delete(lzs_renderer_t** _self)
{
}

void lzs_renderer_resize(lzs_renderer_t* self, int w, int h)
{
}

void lzs_renderer_draw(lzs_renderer_t* self)
{
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
