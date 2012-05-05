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

package com.jeffboody.LaserShark;

import android.util.Log;
import java.util.List;
import android.view.SurfaceHolder;
import android.content.Context;
import android.hardware.Camera;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import com.jeffboody.a3d.A3DNativeRenderer;

public class LaserSharkRenderer extends A3DNativeRenderer
{
	private static final String TAG = "LaserSharkRenderer";

	// Camera state
	private Camera         mCamera;
	private int            mTex;
	private SurfaceTexture mSurfaceTexture;
	private LaserShark     mLaserShark;

	// Native interface
	private native int NativeGetTexture();

	// Renderer implementation
	public LaserSharkRenderer(Context context, LaserShark laser_shark)
	{
		super(context);
		mLaserShark = laser_shark;
	}

	@Override
	public void CreateSurface(SurfaceHolder surface_holder)
	{
		mCamera = Camera.open();
		Camera.Parameters p = mCamera.getParameters();

		// find the nearest supported camera to 480p
		// assume sorted largest-to-smallest
		int width  = 720;
		int height = 480;
		List<Camera.Size> ps = p.getSupportedPreviewSizes();
		for(int i = 0; i < ps.size(); ++i)
		{
			Camera.Size psi = ps.get(i);
			// Log.i(TAG, "width=" + psi.width + ", height=" + psi.height);
			if(psi.height <= 480)
			{
				width  = psi.width;
				height = psi.height;
				break;
			}
		}

		// List<Integer> pf = p.getSupportedPreviewFormats();
		// for(int i = 0; i < pf.size(); ++i)
		// {
		// 	Integer pfi = pf.get(i);
		// 	Log.i(TAG, "format=" + pfi);
		// }

		super.CreateSurface(surface_holder);

		// create the surface texture
		mTex = NativeGetTexture();
		mSurfaceTexture = new SurfaceTexture(mTex);

		// initialize the camera
		Log.i(TAG, "setPreviewSize width=" + width + ", height=" + height);
		p.setPreviewSize(width, height);
		p.setPreviewFormat(ImageFormat.NV21);
		mCamera.setParameters(p);
		try
		{
			mCamera.setPreviewTexture(mSurfaceTexture);
		}
		catch(Exception e)
		{
			Log.e(TAG, "CreateSurface", e);
			mSurfaceTexture = null;
			mCamera.release();
			mCamera = null;
			return;
		}
		mCamera.startPreview();
	}

	@Override
	public void DestroySurface()
	{
		mCamera.stopPreview();
		mCamera.setPreviewCallback(null);
		mCamera.release();
		mCamera = null;
		mSurfaceTexture = null;
		super.DestroySurface();
	}

	@Override
	public void ChangeSurface(int format, int width, int height)
	{
		super.ChangeSurface(format, width, height);
	}

	@Override
	public void Draw()
	{
		mSurfaceTexture.updateTexImage();
		super.Draw();
	}
}
