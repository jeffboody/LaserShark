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
#include "lzs_renderer.h"

#define LOG_TAG "LaserShark"
#include "a3d/a3d_log.h"

/***********************************************************
* private                                                  *
***********************************************************/

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

JNIEXPORT void JNICALL Java_com_jeffboody_LaserShark_LaserShark_NativeGyroEvent(JNIEnv* env, jobject obj, jfloat v1, jfloat v2, jfloat v3, jfloat dt)
{
	assert(env);
	LOGD("debug v1=%f, v2=%f, v3=%f, dt=%f", v1, v2, v3, dt);

	if(lzs_renderer)
	{
		lzs_renderer_phonegyro(lzs_renderer, v1, v2, v3, dt);
	}
}

JNIEXPORT void JNICALL Java_com_jeffboody_LaserShark_LaserShark_NativeSpheroOrientation(JNIEnv* env, jobject obj, jfloat pitch, jfloat roll, jfloat yaw)
{
	assert(env);
	LOGD("debug pitch=%f, roll=%f, roll=%f", pitch, roll, yaw);

	if(lzs_renderer)
	{
		lzs_renderer_spheroorientation(lzs_renderer, pitch, roll, yaw);
	}
}

JNIEXPORT void JNICALL Java_com_jeffboody_LaserShark_LaserShark_NativePhoneOrientation(JNIEnv* env, jobject obj, jfloat pitch, jfloat roll, jfloat yaw)
{
	assert(env);
	LOGD("debug pitch=%f, roll=%f, roll=%f", pitch, roll, yaw);

	if(lzs_renderer)
	{
		lzs_renderer_phoneorientation(lzs_renderer, pitch, roll, yaw);
	}
}

JNIEXPORT int JNICALL Java_com_jeffboody_LaserShark_LaserShark_NativeSpheroHeading(JNIEnv* env)
{
	assert(env);
	LOGD("debug");

	if(lzs_renderer)
	{
		return lzs_renderer_spheroheading(lzs_renderer);
	}
	return 0;
}
