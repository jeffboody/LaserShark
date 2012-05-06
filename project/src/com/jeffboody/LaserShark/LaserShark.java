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
import android.os.Handler;
import android.os.PowerManager;
import android.content.Context;
import android.content.Intent;
import android.view.Window;
import android.view.WindowManager;
import android.hardware.SensorManager;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.view.MotionEvent;
import com.jeffboody.a3d.A3DSurfaceView;
import com.jeffboody.a3d.A3DResource;
import orbotix.robot.app.StartupActivity;
import orbotix.robot.base.*;
import orbotix.robot.sensor.DeviceSensorsData;

public class LaserShark extends Activity implements SensorEventListener
{
	private static final String TAG = "LaserShark";

	// ID for launching the StartupActivity for result to connect to the robot
	private final static int STARTUP_ACTIVITY = 0;
	private Robot mRobot;

	private LaserSharkRenderer Renderer;
	private A3DSurfaceView     Surface;

	// keep screen awake
	private PowerManager.WakeLock mWakeLock;

	// sensors
	final float NS2S = 1.0f / 1000000000.0f;
	private Sensor mOrientation;
	private Sensor mGyro;
	private long   mGyroTimestamp;

	// calibration
	private boolean mIsCalibrated = false;

	private final DeviceMessenger.AsyncDataListener mDataListener = new DeviceMessenger.AsyncDataListener()
	{
		@Override
		public void onDataReceived(DeviceAsyncData data)
		{
			if (data instanceof DeviceSensorsAsyncData)
			{
				DeviceSensorsData ballData = ((DeviceSensorsAsyncData)data).getAsyncData().get(0);

				float[] sensorData = new float[3];
				float pitch = (float)ballData.getAttitudeData().getAttitudeSensor().pitch;
				float roll  = (float)ballData.getAttitudeData().getAttitudeSensor().roll;
				float yaw   = (float)ballData.getAttitudeData().getAttitudeSensor().yaw;
				NativeSpheroOrientation(pitch, roll, yaw);
			}
		}
	};

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
	private native void  NativeTouchOne(float x1, float y1);
	private native void  NativeTouchTwo(float x1, float y1, float x2, float y2);
	private native void  NativeGyroEvent(float v0, float v1, float v2, float dt);
	private native void  NativeSpheroOrientation(float pitch, float roll, float yaw);
	private native void  NativePhoneOrientation(float pitch, float roll, float yaw);
	private native int   NativeSpheroHeading();
	private native float NativeSpheroSpeed();

	public Robot getRobot()
	{
		return mRobot;
	}

	private void blink(final boolean lit)
	{
		if(mRobot != null)
		{
			// if not lit, send command to show blue light, or else, send command to show no light
			if(lit)
			{
				RGBLEDOutputCommand.sendCommand(mRobot, 0, 0, 0);
			}
			else
			{
				RGBLEDOutputCommand.sendCommand(mRobot, 0, 0, 255);
			}

			// send delayed message on a handler to run blink again
			final Handler handler = new Handler();
			handler.postDelayed(new Runnable() { public void run() { blink(!lit); } }, 1000);
		}
	}

	private void drive()
	{
		if(mRobot != null)
		{
			if(mIsCalibrated)
			{
				RollCommand.sendCommand(mRobot, (float) NativeSpheroHeading(), NativeSpheroSpeed());
			}
			else
			{
				RollCommand.sendStop(mRobot);
			}

			// send delayed message on a handler to drive
			final Handler handler = new Handler();
			handler.postDelayed(new Runnable() { public void run() { drive(); } }, 100);
		}
	}

	@Override
	protected void onStart()
	{
		super.onStart();

		// launch the StartupActivity to connect to the robot
		Intent i = new Intent(this, StartupActivity.class);
		startActivityForResult(i, STARTUP_ACTIVITY);
	}


	@Override
	protected void onStop()
	{

		// turn stabilization back on
		//StabilizationCommand.sendCommand(mRobot, true);

		// turn rear light off
		FrontLEDOutputCommand.sendCommand(mRobot, 0.0f);

		// stop the ball
		RollCommand.sendStop(mRobot);

		// stop the streaming data when we leave
		SetDataStreamingCommand.sendCommand(mRobot, 0, 0,
		SetDataStreamingCommand.DATA_STREAMING_MASK_OFF, 0);

		// unregister the async data listener to prevent a memory leak.
		DeviceMessenger.getInstance().removeAsyncDataListener(mRobot, mDataListener);

		// sleep
		SleepCommand.sendCommand(mRobot, 0, 0);

		// pause here for a tenth of a second to allow the previous commands to go through before we shutdown
		// the connection to the ball
		try {
			Thread.sleep(100);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}

		// disconnect from the ball
		RobotProvider.getDefaultProvider().disconnectControlledRobots();

		mRobot = null;
		super.onStop();
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		super.onActivityResult(requestCode, resultCode, data);

		if(requestCode == STARTUP_ACTIVITY && resultCode == RESULT_OK)
		{
			// get the robot
			mRobot = RobotProvider.getDefaultProvider().findRobot(data.getStringExtra(StartupActivity.EXTRA_ROBOT_ID));

			// register the async data listener
			DeviceMessenger.getInstance().addAsyncDataListener(mRobot, mDataListener);

			// turn rear light on
			FrontLEDOutputCommand.sendCommand(mRobot, 1.0f);

			// turn stabilization off
			//StabilizationCommand.sendCommand(mRobot, false);

			// turn data streaming on for the specific types we want
			//
			// Parameters:
			// (1) mRobot   - the robot from which we want the data
			// (2) 2        - this is the divisor applied to the maximum data rate of 400Hz coming back from the
			//                ball we want the data to come back at 200Hz so we use 2 (400Hz/2)
			// (3) 1        - this is how many sensor 'snapshots' are delivered every time we get a data packet
			//                from the ball. In this case, we only want 1, but if you don't need to process data
			//                in real time, you could slow down the data rate and increase the number of
			//                snapshots returned. The snapshots are always returned in an
			//                ArrayList<DeviceSensorData> in the order they were created.
			// (4) mask     - these are the different sensor values we would like to have returned. All of the
			//                available sensors are listed in SetDataStreamingCommand
			// (4) 0        - this is the total number of packets we want returned. If you just wanted a small
			//                window of sensor data from the ball, you could set this to a specific number of
			//                packets to cover that time period based on the divisor and snapshot count set
			//                in the previous parameters. You can also set this to 0 for infinite packets. This
			//                will stream information back to the phone until it is stopped (by sending 0 in the
			//                divisor parameter) or the ball shuts down.
			//
			SetDataStreamingCommand.sendCommand(mRobot, 2, 1,
			                                    SetDataStreamingCommand.DATA_STREAMING_MASK_IMU_PITCH_ANGLE_FILTERED |
			                                    SetDataStreamingCommand.DATA_STREAMING_MASK_IMU_ROLL_ANGLE_FILTERED |
			                                    SetDataStreamingCommand.DATA_STREAMING_MASK_IMU_YAW_ANGLE_FILTERED, 0);

			// start the ball
			drive();
		}
	}

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

		Renderer = new LaserSharkRenderer(this, this);
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

		mOrientation = sm.getDefaultSensor(Sensor.TYPE_ORIENTATION);
		if(mOrientation != null)
		{
			sm.registerListener(this,
			                    mOrientation,
			                    SensorManager.SENSOR_DELAY_GAME);
		}
	}

	@Override
	protected void onPause()
	{
		if((mGyro != null) || (mOrientation != null))
		{
			SensorManager sm = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
			sm = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
			sm.unregisterListener(this);
			mGyro = null;
			mOrientation = null;
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
				mIsCalibrated = true;
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
		else if(event.sensor.getType() == Sensor.TYPE_ORIENTATION)
		{
			float azmuth = event.values[0];
			float pitch  = event.values[1];
			float roll   = event.values[2];
			NativePhoneOrientation(pitch, roll, azmuth);
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
