package com.android.stability;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.os.Handler;
import android.os.Message;
import android.view.OrientationEventListener;

import java.io.IOException;
import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.PictureCallback;
import android.hardware.Camera.Size;

import android.os.AsyncTask;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import android.content.ContentResolver;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import android.os.Environment;
import android.net.Uri;
import android.location.Location;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.media.ExifInterface;
import android.provider.DrmStore;
import android.provider.MediaStore;
import android.provider.MediaStore.Images;
import android.content.ContentValues;
import android.widget.Toast;


import com.android.stability.cameraStabilityActivity;
public class CameraTestActivity extends Activity implements SurfaceHolder.Callback {

    /** log name is "TAG" */
    private static final String TAG = "CameraBackTestActivity";



    private Camera mCamera = null;

    private int mCameraId = 0;
    private SurfaceView mSurfaceView = null;
    private TextView lightMsg = null;
    private SurfaceHolder holder = null;
    /** preview width */
    private static final int PREVIEW_WIDTH = 320;
    /** preview height */
    private static final int PREVIEW_HEIGHT = 240;

    private static final int ROTATION = 90;
    private static final int REVERT = 180;

    /** choose back camer */
    private static final int BACK_CAMERA = 0;
    private static final int FRONT_CAMERA = 1;


    private int mTestType = 0;
	private int mTestTimes = 0;

	private int mHaveTestTime = 0;
	private int mCameraStatus = 0;//0:idle 1:capture 2 previewed 3 stopping 4 stopped 5: error

	private boolean  mStopFlag = false;

	private Thread mWorkThread ;

	private final int mCameraNum = Camera.getNumberOfCameras();
	
	private ContentResolver mContentResolver;
  
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);	
        setTitle(getResources().getText(R.string.back_camera_title_text));
		
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		
        setContentView(R.layout.back_camera_result);
		
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
		
        setTitle(R.string.camera_test_title);
		
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
    
        mSurfaceView = (SurfaceView) findViewById(R.id.surfaceView);
        lightMsg = (TextView)findViewById(R.id.light_msg);

		mCameraId = getIntent().getIntExtra(cameraStabilityActivity.CAMERA_SELECT_TYPE, 0);
		String times = getIntent().getStringExtra(cameraStabilityActivity.CAMERA_REPEAT_TIMES);
		if(times == null){
			mTestTimes = cameraStabilityActivity.DefaultTestTimes;
		}else{
			try{
				mTestTimes = Integer.parseInt(times);
			}catch(NumberFormatException e){
				mTestTimes = cameraStabilityActivity.DefaultTestTimes;
			}
			
		}
		mTestType = getIntent().getIntExtra(cameraStabilityActivity.CAMERA_TEST_ITEM, 0);
		Log.d(TAG,"the camera test mCameraId is "+mCameraId+", the test times is "+mTestTimes+", the mTestType is "+mTestType);
		
		mHander.sendEmptyMessage(TestHandler.UPDATE_MSG);

		mContentResolver = getContentResolver();
		if(mTestType == cameraStabilityActivity.ChangeCameraTest){
			if(mCameraNum<=1){
				Toast.makeText(
                                this,
                                R.string.only_back_camera,
                                Toast.LENGTH_SHORT).show();
				finish();
			}else{
				mWorkThread = new Thread(new ChangerCameraWorker());
				mWorkThread.start();
			}
	
		}else if(mTestType == cameraStabilityActivity.RepeateTest){
			mWorkThread = new Thread(new RepeatCaptureWorker());
			mWorkThread.start();
		}else if(mTestType == cameraStabilityActivity.PoweroffTest){
			mWorkThread = new Thread(new PowerOffWorker());
			mWorkThread.start();
		}else{
			Toast.makeText(
                                this,
                                R.string.not_choose_testtype,
                                Toast.LENGTH_SHORT).show();
			finish();
		}

    }



    private void waitPreviewStatus(){
		while(!mStopFlag && mCameraStatus != 2 && mCameraStatus != 5){
			try{
				Thread.sleep(500L);//wait the camera is preview status
			} catch (InterruptedException e) {
                                // ignore
            }
			Log.e(TAG,"the camera already in preview status");
		}
	}

	private void waitSeconds(int second){
		int num = 2*second;
		while(!mStopFlag && num >0){
			try{
				Thread.sleep(500L);
				num--;
			}catch (InterruptedException e) {
                            // ignore
        	}
		}
	}
	
    private class ChangerCameraWorker implements Runnable {

		@Override
		public void run() {
			waitPreviewStatus();
			waitSeconds(1);
			boolean status = true;
			while(!mStopFlag && mHaveTestTime < mTestTimes){
				status = stopCamera();
				if(!status){
					break;
				}
				mCameraId = (mCameraId == BACK_CAMERA)?FRONT_CAMERA:BACK_CAMERA;
				waitSeconds(2);
				status = startCamera();	
				if(!status){
					break;
				}
				waitPreviewStatus();
				mHander.sendEmptyMessage(TestHandler.UPDATE_MSG);
				mHaveTestTime++;
				waitSeconds(3);
				
			}

			if(!status){
				Log.e(TAG,"ChangerCameraWorker ERROR!");
				stopCamera();
				mHander.sendEmptyMessage(TestHandler.ERROR_MSG);
			}
			
		}

	}

	private class PowerOffWorker implements Runnable {
		@Override
		public void run() {
			waitPreviewStatus();
			waitSeconds(1);
			boolean status = true;
			while(!mStopFlag && mHaveTestTime < mTestTimes){
				status = stopCamera();
				if(!status){
					break;
				}
				waitSeconds(2);
				status = startCamera();
				if(!status){
					break;
				}
				waitPreviewStatus();
				mHander.sendEmptyMessage(TestHandler.UPDATE_MSG);
				mHaveTestTime++;
				waitSeconds(3);
			}
			if(!status){
				Log.e(TAG,"PowerOffWorker ERROR!");
				stopCamera();
				mHander.sendEmptyMessage(TestHandler.ERROR_MSG);
			}
		}
	}

	private class RepeatCaptureWorker implements Runnable {
		@Override
		public void run() {
			waitPreviewStatus();
			waitSeconds(1);
			boolean status = true;
			while(!mStopFlag && status && mHaveTestTime < mTestTimes){
				status = capture();
				if(!status){
					break;
				}
				waitSeconds(1);
				status = startCamera();
				if(!status){
					break;
				}
				waitPreviewStatus();
				waitSeconds(3);
				mHander.sendEmptyMessage(TestHandler.UPDATE_MSG);
				mHaveTestTime++;
				//waitSeconds(3);
			}
			if(!status){
				stopCamera();
				Log.e(TAG,"RepeatCaptureWorker ERROR!");
				mHander.sendEmptyMessage(TestHandler.ERROR_MSG);
			}
		}

	}

    private boolean  capture() {
	    boolean status = false;
	    Camera.Parameters parameters = null;

	    parameters = mCamera.getParameters();
	    parameters.set("sensororientation", 0);

	    int rotation = 0;
		int mOrientation = 0;

	    if (mOrientation != OrientationEventListener.ORIENTATION_UNKNOWN) {
	        CameraInfo info = new CameraInfo();
			mCamera.getCameraInfo(mCameraId, info);
	        if (info.facing == CameraInfo.CAMERA_FACING_FRONT) {
	            rotation = (info.orientation - mOrientation + 360) % 360;
	        } else {  // back-facing camera
	            rotation = (info.orientation + mOrientation) % 360;
	        }
	    }
	    parameters.setRotation(rotation);

	    // Clear previous GPS location from the parameters.
	    parameters.removeGpsData();

	    mCamera.setParameters(parameters);

	    mCameraStatus = 1;
	    try{
	        mCamera.takePicture(null, null,
	                null, new JpegPictureCallback());
			status = true;
	    }catch(RuntimeException e){
	        if("takePicture failed".equals(e.getMessage())) {
	            Log.d(TAG, "baofeng is running,take photo error!!!!!!!");

	            startCamera();
	        } else {
	            throw e;
	        }
	    }

		return status;
	}

	
   private final TestHandler mHander = new TestHandler();
   private class TestHandler extends Handler {

		static final int START_PRIVIEW = 1;
		static final int UPDATE_MSG = 2;
		static final int ERROR_MSG = 3;
    	TestHandler(){}
    	 @Override
         public void handleMessage(Message msg) {
			int what = msg.what;
			switch(what){
				case START_PRIVIEW:
					try {
		                mCamera.setPreviewDisplay(holder);
		                Log.e(TAG, "start preview");
		                mCamera.startPreview();
						mCameraStatus = 2;
            		} catch (Exception e) {
		                mCamera.release();
						mCameraStatus = 5;
		                // mCamera = null;
		                mHander.sendEmptyMessage(TestHandler.ERROR_MSG);
            		}
					break;

				case UPDATE_MSG:
					if(mTestType == cameraStabilityActivity.ChangeCameraTest){
						lightMsg.setText("ChangeCameraTest "+mTestTimes+ " times! has tested "+ mHaveTestTime+"  times ");
					}else if(mTestType == cameraStabilityActivity.RepeateTest){
						lightMsg.setText("Repeat take picture Test "+mTestTimes+ " times! has tested "+ mHaveTestTime+"  times ");
					}else if(mTestType == cameraStabilityActivity.PoweroffTest){
						lightMsg.setText("PowerOn/Off Test "+mTestTimes+ " times! has tested "+ mHaveTestTime+"  times ");
					}
					break;
					
				case ERROR_MSG:
					if(mTestType == cameraStabilityActivity.ChangeCameraTest){
						lightMsg.setText("ChangeCameraTest ERROR! has tested "+ mHaveTestTime+"  times ");
					}else if(mTestType == cameraStabilityActivity.RepeateTest){
						lightMsg.setText("Repeat take picture Test ERROR!! has tested "+ mHaveTestTime+"  times ");
					}else if(mTestType == cameraStabilityActivity.PoweroffTest){
						lightMsg.setText("PowerOn/Off Test ERROR! has tested "+ mHaveTestTime+"  times ");
					}
					break;
					
			}
			
		 }
   }


	private final class JpegPictureCallback implements PictureCallback {
		public JpegPictureCallback(){}
		public void onPictureTaken(
                final byte [] jpegData, final android.hardware.Camera camera) {
                if ((jpegData != null) && (jpegData.length != 0)) {
                	StoreImageTask task = new StoreImageTask();
					task.execute(jpegData,null);
            	}

		}

	}

	private class ImageManager {
		
		private  final String CAMERA_DCIM_NAME = "/DCIM";
		// internal bucket path
		private  final String CAMERA_IMAGE_INTERNAL_BUCKET = "/data/internal_memory";
		// camera bcket name
		private  final String CAMERA_IMAGE_BUCKET = (CAMERA_DCIM_NAME + "/Camera");

		private  final Uri STORAGE_URI = Images.Media.EXTERNAL_CONTENT_URI;
	
		public  Uri addImage(ContentResolver cr, String title, long dateTaken,
	            Location location, String directory, String filename,
	            Bitmap source, byte[] jpegData, int[] degree) {
	        // We should store image data earlier than insert it to ContentProvider,
	        // otherwise we may not be able to generate thumbnail in time.
	        OutputStream outputStream = null;
	        String filePath = directory + "/" + filename;
	        try {
	            File dir = new File(directory);
	            if (!dir.exists()) dir.mkdirs();
	            File file = new File(directory, filename);
	            outputStream = new FileOutputStream(file);
	            if (source != null) {
	                source.compress(CompressFormat.JPEG, 75, outputStream);
	                degree[0] = 0;
	            } else {
	                outputStream.write(jpegData);
	                degree[0] = getExifOrientation(filePath);
	            }
	        } catch (FileNotFoundException ex) {
	            Log.w(TAG, ex);
	            return null;
	        } catch (IOException ex) {
	            Log.w(TAG, ex);
	            return null;
	        } finally {
			    if (outputStream != null){
			        try {
			            outputStream.close();
			        } catch (Throwable t) {
			            // do nothing
			        }
				}
	        }

	        // Read back the compressed file size.
	        File tmp = ensureFilePermission(directory, filename);
	        long size = (tmp == null ? 0 : tmp.length());

	        ContentValues values = new ContentValues(9);
	        values.put(Images.Media.TITLE, title);

	        values.put(Images.Media.DISPLAY_NAME, filename);
	        values.put(Images.Media.DATE_TAKEN, dateTaken);
	        values.put(Images.Media.MIME_TYPE, "image/jpeg");
	        values.put(Images.Media.ORIENTATION, degree[0]);
	        values.put(Images.Media.DATA, filePath);
	        values.put(Images.Media.SIZE, size);

	        if (location != null) {
	            values.put(Images.Media.LATITUDE, location.getLatitude());
	            values.put(Images.Media.LONGITUDE, location.getLongitude());
	        }

	        Log.d(TAG, String.format("storage uri = %s, values = %s", new Object[] { STORAGE_URI, values }));
	        
	        Uri result = cr.insert(STORAGE_URI, values);
	        Log.d(TAG, String.format("return insert uri = %s", result));
	        return result;
	        
	    }

	    public  File ensureFilePermission(String dir, String name) {
	        if (dir != null && name != null)
	            return ensureFilePermission(new File(dir, name));
	        Log.d(TAG, String.format("return NULL ensureFilePermission(dir = %s, name = %s)", new Object[] { dir, name }));
	        return null;
	    }
		public  String getCameraImageBucketPath() {
	        Log.d(TAG, "----- getCameraImageBucketPath() -----");
	        String result = null;
	        // get bucket root path
	        // external: "/mnt/sdcard/DCIM/Camera"
	        // internal: "/data/internal_sdcard/DCIM/Camera"
	        if ((result = getBucketRootPath()) != null) {
	            result = result.concat(CAMERA_IMAGE_BUCKET);
	            // if current bucket name is "CAMERA_IMAGE_INTERNAL_BUCKET"
	            // then ensure directory exists and set permission
	            ensureDirectoryExists(result);
	        }
	        Log.d(TAG, String.format("return getCameraImageBucketPath(%s)", result));
	        return result;
    	}

	    public  String getBucketRootPath() {
	        return getBucketRootPath(true);
	    }

		public  String getBucketRootPath(boolean requireWriteAccess) {
        	return getRootPath(requireWriteAccess);
    	}

    	private  String getRootPath(boolean requireWriteAccess) {
	        Log.d(TAG, "----- getBucketRootPath() -----");
	        String result = null;
	        // validate external sdcard
	        if (checkExternalStorage(requireWriteAccess)) {
	            result = Environment.getExternalStorageDirectory().toString();
	        }
	        // validate internal sdcard
	        else if (checkInternalStorage(requireWriteAccess)) {
	            result = CAMERA_IMAGE_INTERNAL_BUCKET;
	        }

	        if (result == null) {
	            Log.d(TAG, "getBucketRootPath() is NULL");
	        }

	        Log.d(TAG, String.format("return getBucketRootPath(%s)", result));
	        return result;
    	}
		
		private  boolean isInternalBucket = false;
		private  boolean checkExternalStorage(boolean requireWriteAccess) {
	        Log.d(TAG, "----- checkExternalStorage() -----");
	        boolean result = false;
	        // set "isInternalBucket" flag
	        isInternalBucket = false;
	        // get environment state and validate state
	        String state = Environment.getExternalStorageState();
	        // mounted state, we can read/write state
	        if (Environment.MEDIA_MOUNTED.equals(state)) {
	            result =
	                (requireWriteAccess ?
	                    checkFsWritable(Environment.getExternalStorageDirectory())
	                        : true);
	        }
	        // read-only state and requireWriteAccess is false, we can read state
	        else if (!requireWriteAccess &&
	                        Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
	            result = true;
	        }
	        Log.d(TAG, String.format("return checkExternalStorage(%b)", result));
	        return result;
    	}

		private  boolean checkInternalStorage(boolean requireWriteAccess) {
	        Log.d(TAG, "----- checkInternalStorage() -----");
	        boolean result = false;
	        // set "isInternalBucket" flag
	        isInternalBucket = true;
	        result =
	            (requireWriteAccess ?
	                checkFsWritable(CAMERA_IMAGE_INTERNAL_BUCKET) : // we can write state
	                    checkFsReadable(CAMERA_IMAGE_INTERNAL_BUCKET)); // we can read state
	        Log.d(TAG, String.format("return checkInternalStorage(%b)", result));
	        return result;
    	}

		private  boolean checkFsWritable(String path) {
	        boolean result = false;
	        if (path != null)
	            result = checkFsWritable(new File(path));
	        return result;
    	}

		private  boolean checkFsReadable(String path) {
	        boolean result = false;
	        if (path != null)
	            result = checkFsReadable(new File(path));
	        return result;
    	}

		private  boolean checkFsWritable(File path) {
	        Log.d(TAG, "----- checkFsWritable() -----");
	        boolean result = false;
	        // Create a temporary file to see whether a volume is really writable.
	        // It's important not to put it in the root directory which may have a
	        // limit on the number of files.
	        if (path != null) {
	            File dir = new File(path, CAMERA_DCIM_NAME);
	            if (ensureDirectoryExists(dir)) {
	                result = dir.canWrite();
	            }
	        }
	        Log.d(TAG, String.format("return checkFsWritable(%b)", result));
	        return result;
    	}

		private  boolean checkFsReadable(File path) {
	        Log.d(TAG, "----- checkFsReadable() -----");
	        boolean result = false;
	        // Create a temporary file to see whether a volume is really readable.
	        // It's important not to put it in the root directory which may have a
	        // limit on the number of files.
	        if (path != null) {
	            File dir = new File(path, CAMERA_DCIM_NAME);
	            if (ensureDirectoryExists(dir)) {
	                result = dir.canRead();
	            }
	        }
	        Log.d(TAG, String.format("return checkFsReadable(%b)", result));
	        return result;
    	}
		
	    public  boolean ensureDirectoryExists(String path) {
	        if (path != null)
	            return ensureDirectoryExists(new File(path));
	        Log.d(TAG, String.format("return false ensureDirectoryExists(dir = %s)", path));
	        return false;
	    }
		private  boolean ensureDirectoryExists(File path) {
	        Log.d(TAG, "----- ensureDirectoryExists() -----");
	        boolean result = (path != null);
	        if (result) {
	            if (!path.isDirectory() || !path.exists()) {
	                result = path.mkdirs();
	                Log.d(TAG, String.format("result ensureDirectoryExists execute mkdir = %b", result));
	                // if current bucket name is "CAMERA_IMAGE_INTERNAL_BUCKET" then set permission
	                if (result) ensureFilePermission(path);
	            }
	            Log.d(TAG, String.format("result ensureDirectoryExists path = %s", path));
	        }
	        Log.d(TAG, String.format("return ensureDirectoryExists(%b)", result));
	        return result;
    	}

		private  File ensureFilePermission(File path) {
	        Log.d(TAG, "----- ensureFilePermission() -----");
	        if (path != null) {
	            // if current bucket is "CAMERA_IMAGE_INTERNAL_BUCKET"
	            // then set permission is 777 to "result"
	            boolean need_set_perm = isInternalBucket;
	            if (need_set_perm && path.exists()) {
	                path.setReadable(need_set_perm, !need_set_perm);
	                path.setWritable(need_set_perm, !need_set_perm);
	                path.setExecutable(need_set_perm, !need_set_perm);
	            }
	            Log.d(TAG, String.format(
	                "result need_set_perm = %b, canRead = %b, canWrite = %b, canExecute = %b",
	                    new Object[] { need_set_perm, path.canRead(), path.canWrite(), path.canExecute()}));
	        }
	        Log.d(TAG, String.format("return ensureFilePermission(%s)", path));
	        return path;
    	}

		public  int getExifOrientation(String filepath) {
	        int degree = 0;
	        ExifInterface exif = null;
	        try {
	            exif = new ExifInterface(filepath);
	        } catch (IOException ex) {
	            Log.e(TAG, "cannot read exif", ex);
	        }
	        if (exif != null) {
	            int orientation = exif.getAttributeInt(
	                ExifInterface.TAG_ORIENTATION, -1);
	            if (orientation != -1) {
	                // We only recognize a subset of orientation tag values.
	                switch(orientation) {
	                    case ExifInterface.ORIENTATION_ROTATE_90:
	                        degree = 90;
	                        break;
	                    case ExifInterface.ORIENTATION_ROTATE_180:
	                        degree = 180;
	                        break;
	                    case ExifInterface.ORIENTATION_ROTATE_270:
	                        degree = 270;
	                        break;
	                }

	            }
	        }
	        return degree;
    	}
	}

	 private class StoreImageTask extends AsyncTask<Object,Object,Object>{
			@Override
			protected Object doInBackground(Object... params){
				Uri result= null;
				byte[] data = (byte[])params[0];
				Location loc = (Location)params[1];
	            try {
	                long dateTaken = System.currentTimeMillis();
	                String title = createName(dateTaken);
	                String filename = title + ".jpg";
	                int[] degree = new int[1];
					ImageManager manager = new ImageManager();
	                 result = manager.addImage(
	                        mContentResolver,
	                        title,
	                        dateTaken,
	                        loc, // location from gps/network
	                        manager.getCameraImageBucketPath(), filename,
	                   
	                        null, data,
	                        degree);
	
	            } catch (Exception ex) {
	                Log.e(TAG, "Exception while compressing image.", ex);
	            }
				Map<String,Object> map = new HashMap<String,Object>();
				map.put("result",result);
				//map.put("image",data);

				return map;
			}

			@Override
			protected void onPostExecute(Object result){
               Map<String,Object> map = (HashMap<String,Object>)result;
			   Uri content = (Uri)map.get("result");
			   if(content == null ){
			   		mCameraStatus = 5;
					mStopFlag = true;//stop the test!
					mHander.sendEmptyMessage(TestHandler.ERROR_MSG);
			   }
			   super.onPostExecute(result);
			}

		}


    /***/

	private String createName(long dateTaken) {
        Date date = new Date(dateTaken);
        SimpleDateFormat dateFormat = new SimpleDateFormat(
                getString(R.string.image_file_name_format));

        return dateFormat.format(date);
    }
    private boolean startCamera() {

		boolean status = false;

        if (mCamera != null) {
            mCamera.stopPreview();
            mCamera.release();
        }
			
        try {
            Log.e(TAG, "open");
            mCamera = Camera.open(mCameraId);

        } catch (RuntimeException e) {
            Log.e(TAG, "fail to open camera");
            e.printStackTrace();
            mCamera = null;
        }
        if (mCamera != null) {
            Camera.Parameters parameters = null;
            mCamera.setDisplayOrientation(mCameraId == BACK_CAMERA ? ROTATION : ROTATION);
            parameters = mCamera.getParameters();
            parameters.setPictureFormat(PixelFormat.JPEG);
            parameters.set("orientation", "portrait");
            parameters.setPreviewSize(PREVIEW_WIDTH, PREVIEW_HEIGHT);
            parameters.setRotation(mCameraId == BACK_CAMERA ? ROTATION : REVERT+ROTATION);
            parameters.setFlashMode(Camera.Parameters.FLASH_MODE_TORCH);
            mCamera.setParameters(parameters);

			mHander.sendEmptyMessage(TestHandler.START_PRIVIEW);

			status = true;
 
        }

		return status;
    }


    private void failureIntent() {
    }


    public void surfaceChanged(SurfaceHolder sholder, int format, int width, int height) {

    }

    /** @param sholder holder */
    public void surfaceCreated(SurfaceHolder sholder) {
        startCamera();
    }

    /** @param sholder holder */
    public void surfaceDestroyed(SurfaceHolder sholder) {

    }

    @Override
    protected void onResume() {
        super.onResume();
        holder = mSurfaceView.getHolder();
        holder.addCallback(this);
        holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
    }

    private void cameraNumber(){
    	if(mCameraId == BACK_CAMERA){
    		mCameraId = FRONT_CAMERA;
        if(lightMsg!=null){
              lightMsg.setVisibility(View.GONE);
                }
    	}else{
    		mCameraId = BACK_CAMERA;
         if(lightMsg!=null){
                lightMsg.setVisibility(View.VISIBLE);
            }
    	}
    	Log.d("donglin", "switched camera id = " + mCameraId);
    }
    @Override
    protected void onPause() {
        super.onPause();
		mStopFlag = true;

		if(mWorkThread!=null){
		try{
			mWorkThread.join();
		}
		catch(Exception e){
			Log.d(TAG,String.format("%s join failed; E: ", mWorkThread.getName()), e);
		}
		mWorkThread = null;	
		stopCamera();
	 }
    }

	private boolean stopCamera(){
		Log.e(TAG,"Stop camera!");

		boolean status = false;
		
        try {
            if (mCamera != null) {
                mCamera.stopPreview();
                mCamera.release();
                mCamera = null;
				status = true;
				mCameraStatus = 4;
            }
        } catch (Exception e) {
            e.printStackTrace();
			mCameraStatus = 5;
        }

		return status;
	}


}
