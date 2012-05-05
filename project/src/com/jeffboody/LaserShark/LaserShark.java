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
import android.app.Activity;
import android.os.Bundle;
import android.os.PowerManager;
import android.content.Context;
import android.view.Window;
import android.view.WindowManager;
import android.hardware.SensorManager;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.view.MotionEvent;
import com.jeffboody.a3d.A3DSurfaceView;
import com.jeffboody.a3d.A3DResource;

public class LaserShark extends Activity implements SensorEventListener
{
	private static final String TAG = "LaserShark";

	private LaserSharkRenderer Renderer;
	private A3DSurfaceView     Surface;

	// keep screen awake
	private PowerManager.WakeLock mWakeLock;

	// sensors
	final float NS2S = 1.0f / 1000000000.0f;
	private Sensor        mGyro;
	private long          mGyroTimestamp;

	// touch events
	private final int INIT_STATE = 0;
	private final int ONE_STATE  = 1;
	private final int TWO_STATE  = 2;
	private int   State = INIT_STATE;
	private float X1    = 0.0F;
	private float Y1    = 0.0F;
	private float X2    = 0.0F;
	private float Y2    = 0.0F;

	// Native interface
	private native void NativeTouchOne(float x1, float y1);
	private native void NativeTouchTwo(float x1, float y1, float x2, float y2);
	private native void NativeGyroEvent(float v0, float v1, float v2, float dt);

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		A3DResource r = new A3DResource(this, R.raw.timestamp);
		r.Add(R.raw.whitrabt, "whitrabt.tex.gz");

		// Make window fullscreen
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
		                     WindowManager.LayoutParams.FLAG_FULLSCREEN);

		Renderer = new LaserSharkRenderer(this);
		Surface  = new A3DSurfaceView(Renderer, r, this);
		setContentView(Surface);

		PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		mWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, "RoboJoystick");
	}

	@Override
	protected void onResume()
	{
		super.onResume();
		Surface.ResumeRenderer();
		mWakeLock.acquire();

		SensorManager sm = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
		mGyro = sm.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
		if(mGyro != null)
		{
			sm.registerListener(this,
			                    mGyro,
			                    SensorManager.SENSOR_DELAY_NORMAL);
		}
		mGyroTimestamp = 0L;
	}

	@Override
	protected void onPause()
	{
		if(mGyro != null)
		{
			SensorManager sm = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
			sm = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
			sm.unregisterListener(this);
			mGyro = null;
		}

		mWakeLock.release();
		Surface.PauseRenderer();
		super.onPause();
	}

	@Override
	protected void onDestroy()
	{
		Surface.StopRenderer();
		Surface = null;
		Renderer = null;
        super.onDestroy();
	}

	@Override
	public boolean onTouchEvent(MotionEvent event)
	{
		try
		{
			int action = event.getAction();
			int count  = event.getPointerCount();

			if(action == MotionEvent.ACTION_UP)
			{
				// Do nothing
				State = INIT_STATE;
			}
			else if(count == 1)
			{
				if(action == MotionEvent.ACTION_DOWN)
				{
					X1 = event.getX();
					Y1 = event.getY();
					State = ONE_STATE;
				}
				else if((action == MotionEvent.ACTION_MOVE) && (State == ONE_STATE))
				{
					X1 = event.getX();
					Y1 = event.getY();
					NativeTouchOne(X1, Y1);
				}
			}
			else if(count == 2)
			{
				if((action == MotionEvent.ACTION_DOWN) ||
				   (action == MotionEvent.ACTION_POINTER_1_DOWN) ||
				   (action == MotionEvent.ACTION_POINTER_2_DOWN))
				{
					X1 = event.getX(event.findPointerIndex(0));
					Y1 = event.getY(event.findPointerIndex(0));
					X2 = event.getX(event.findPointerIndex(1));
					Y2 = event.getY(event.findPointerIndex(1));
					State = TWO_STATE;
				}
				else if((action == MotionEvent.ACTION_MOVE) && (State == TWO_STATE))
				{
					X1 = event.getX(event.findPointerIndex(0));
					Y1 = event.getY(event.findPointerIndex(0));
					X2 = event.getX(event.findPointerIndex(1));
					Y2 = event.getY(event.findPointerIndex(1));
					NativeTouchTwo(X1, Y1, X2, Y2);
				}
			}
			// limit touch events to 30Hz
			Thread.sleep((long) (1000.0F/30.0F));
		}
		catch(Exception e)
		{
			// ignore
		}

		return true;
	}

	/*
	 * SensorEventListener interface
	 */

	public void onSensorChanged(SensorEvent event)
	{
		if(event.sensor.getType() == Sensor.TYPE_GYROSCOPE)
		{
			if(mGyroTimestamp != 0L)
			{
				float dt = (event.timestamp - mGyroTimestamp) * NS2S;
				NativeGyroEvent(event.values[0], event.values[1], event.values[2], dt);
			}
			mGyroTimestamp = event.timestamp;
		}
	}

	public void onAccuracyChanged(Sensor sensor, int accuracy)
	{
	}

	static
	{
		System.loadLibrary("texgz");
		System.loadLibrary("a3d");
		System.loadLibrary("LaserShark");
	}
}
