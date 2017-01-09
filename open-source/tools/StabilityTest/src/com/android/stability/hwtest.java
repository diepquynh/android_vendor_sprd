package com.android.stability;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.StatFs;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class hwtest extends Activity {
	
	private static final String TAG = "hwtest";
	
	private static final String mFolder = "/testtemp";
	private static final String mTempFileFolder = Environment.getExternalStorageDirectory()+mFolder;
	private static final String mTestFilePatch = mTempFileFolder+"/testfile";
	private static final int mTempFileSize = 2;//the temp file size is 2M
	private boolean mStopFlag = false;
	private  int mWorkcount = 0;
	private int mFreerate = 0;
	private long mFreesize = 0;
	private boolean mOprateResult = true;
	private int mStatus = 0;//0:idle;1:copy;2:delete;3:no_card;4,stopping
	
	
	private Button stop_button;
	private TextView mCountView;
	
	private Thread mWorkThread ;
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.tf_layout);
        mCountView = (TextView)findViewById(R.id.count_and_show);
        stop_button = (Button)findViewById(R.id.Button01);
		stop_button.setText("Start");
        stop_button.setOnClickListener(new Button.OnClickListener() {

			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				if(mStatus == 1 ||mStatus == 2){
					mStopFlag = true;
					mStatus = 4;
					mHander.sendEmptyMessage(TestHandler.UPDATE_MSG_UPDATE);
				}else if(mStatus == 0){			
					if(mWorkThread != null){
						try{
							mWorkThread.join();
						}
						catch(Exception e){
							Log.d(TAG,String.format("%s join failed; E: ", mWorkThread.getName()), e);
						}
						mWorkThread = null;
					}
					mWorkThread = new Thread(new Worker());
					mWorkThread.start();					
					stop_button.setText("Stop");
				}

			}
        	
        	
        });
     
    }
    
    @Override
    protected void onDestroy() {
	 super.onDestroy();
	 Log.d(TAG, "onDestroy()");
	 mStopFlag = true;
	 if(mWorkThread!=null){
		try{
			mWorkThread.join();
		}
		catch(Exception e){
			Log.d(TAG,String.format("%s join failed; E: ", mWorkThread.getName()), e);
		}
		mWorkThread = null;	
	 }
    }

	
    @Override
    protected void onStart() {
	 super.onStart();
	 Log.d(TAG, "onStart()");
	 mHander.sendEmptyMessage(TestHandler.UPDATE_MSG_UPDATE);
    }

    private class Worker implements Runnable {

		@Override
		public void run() {
			// TODO Auto-generated method stub
			Log.d(TAG,"the worker thread start to running! ");
			mStopFlag = false;
			mOprateResult = true;
			if(!checkTFCardStatus()){
				Log.e(TAG,"the TF card is not mounted!!");
				mStatus = 3;
				mHander.sendEmptyMessage(TestHandler.UPDATE_MSG_NOCARD);
				return;
			}
			mHander.sendEmptyMessage(TestHandler.UPDATE_MSG_START);

			while(!mStopFlag && mOprateResult){
				mWorkcount++;
				mHander.sendEmptyMessageDelayed(TestHandler.UPDATE_MSG_UPDATE,
						10);
				deleteTempFile();	//delete files from TF Card
				fillTFCard();//write TF Card
				Log.d(TAG,"the "+mWorkcount+" times test has finished!");
			}
			
			deleteTempFile();//when stop the test,delete all temp file
			mStatus = 0;
			mHander.sendEmptyMessage(TestHandler.UPDATE_MSG_UPDATE);

			
		}
    	
    	
    }
    
    private final TestHandler mHander = new TestHandler();
    
    private class TestHandler extends Handler {
    	
    	static final int UPDATE_MSG_START = 1;
    	//static final int UPDATE_MSG_STOP = 2;
    	static final int UPDATE_MSG_UPDATE = 3;
    	//static final int UPDATE_MSG_ERROR = 4;
    	static final int UPDATE_MSG_NOCARD = 5;
    	
    	TestHandler(){}
    	 @Override
         public void handleMessage(Message msg) {
    		 int what = msg.what;
    		 switch(what){
	    		 case UPDATE_MSG_START:
	    			 mCountView.setText("the work thread is starting! ");
	    			 break;
	    	
	    		 case UPDATE_MSG_UPDATE:
	    			 if(mStatus == 1){//Writing message
	    				 mCountView.setText("the work thread is doing the "+
		    					 mWorkcount+" times copy test,the free_ratio of TFCard is "+
		    					 				 mFreerate+"% and the free size is "+mFreesize/1024/1024+"M");
	    			 }else if(mStatus == 2){//deleting message
	    				 mCountView.setText("the work thread is doing the "+
		    					 mWorkcount+" times delete test,the free_ratio of TFCard is "+
		    					 mFreerate+"% and the free size is "+mFreesize/1024/1024+"M");
	    			 }else if(mStatus == 4){//stopping
	    				 mCountView.setText("the test is stopping"); 
	    			 }else if(mStatus == 0){// idle status
	    				 if(mOprateResult){
	    					 mCountView.setText("the test is stopped,there is no error happened in "+mWorkcount +" times test!"); 
	    				 }else{
	    					 mCountView.setText("the test is stopped,there is errors happened in "+mWorkcount +" times test!");
	    				 }
						 stop_button.setText("Start");
	    			 }
	    			 
	    		 	 break;
	    		 case UPDATE_MSG_NOCARD:
	    			 mCountView.setText("this is no TF Card,please check!");
					 //finish();
	    			 break;
	    		 default:
	    			 break;
    		 }
    		 
    	 }
    }

    private boolean checkTFCardStatus(){
    	Log.d(TAG,"checkTFCardStatus");
    	boolean result = false;
    	String state = Environment.getExternalStorageState();
        if (Environment.MEDIA_MOUNTED.equals(state)) {
            result =checkFsWritable(Environment.getExternalStorageDirectory());
        }
		return result;
    	
    	
    }
    
    private static boolean checkFsWritable(File path) {
        Log.d(TAG, "----- checkFsWritable() -----");
        boolean result = false;
        // Create a temporary file to see whether a volume is really writable.
        // It's important not to put it in the root directory which may have a
        // limit on the number of files.
        if (path != null) {
            File dir = new File(path, mFolder);
            if (ensureDirectoryExists(dir)) {
                result = dir.canWrite();
            }
        }
        Log.d(TAG, String.format("return checkFsWritable(%b)", result));
        return result;
    }
    
    private static boolean ensureDirectoryExists(File path) {
        Log.d(TAG, "----- ensureDirectoryExists() -----");
        boolean result = (path != null);
        if (result) {
            if (!path.isDirectory() || !path.exists()) {
                result = path.mkdirs();
                Log.d(TAG, String.format("result ensureDirectoryExists execute mkdir = %b", result));
                // if current bucket name is "CAMERA_IMAGE_INTERNAL_BUCKET" then set permission
            }
            Log.d(TAG, String.format("result ensureDirectoryExists path = %s", path));
        }
        Log.d(TAG, String.format("return ensureDirectoryExists(%b)", result));
        return result;
    }

    private void fillTFCard(){
    	//String folderName = Environment.getExternalStorageDirectory()+mFolder;
    	File srcFile = new File(mTestFilePatch);
    	long filesize = mTempFileSize*1024*1024;
    	mFreesize = getFreeSize(mTempFileFolder);
    	long count = 1;
		mStatus = 1;
    	while(!mStopFlag && (mFreesize>2*filesize)&& mOprateResult){
    		Log.d(TAG,"the "+ count +"times copy file; the free size is "+mFreesize);
			Log.d(TAG,"the src file path is "+ mTestFilePatch +", the dest file path is "+mTempFileFolder+"/"+count);
    		mOprateResult = copyFile(mTestFilePatch,mTempFileFolder+"/"+count);
			Log.d(TAG,"this time opration result is "+mOprateResult);
    		mFreesize = getFreeSize(mTempFileFolder);
			count++;
			mHander.sendEmptyMessage(TestHandler.UPDATE_MSG_UPDATE);
    		if(mFreesize<filesize){
				Log.e(TAG,"the free size is "+mFreesize+" and the filesize is "+filesize);
				int size = (int)(mFreesize/1024/1024);
				if(size>0){
					mOprateResult = newFile(new File(mTempFileFolder+"/"+count),size);
					Log.e(TAG,"New the last file !");
				}
    			break;
    		}

    	}
		//mStatus = 0;
    }
    
    private void deleteTempFile(){
    	mStatus = 2;
    	//String path = Environment.getDataDirectory()+mFolder;
    	File  file  =  new  File(mTempFileFolder);  
        if  (!file.exists())  {  
            return;  
        }  
        if  (!file.isDirectory())  {  
            return;  
        }  
        String[]  tempList  =  file.list();  
        File  temp  =  null;  
		Log.d(TAG,"will delete "+tempList.length +" files!");
        for  (int  i  =  0;  (i  <  tempList.length)/*&& !mStopFlag */&& mOprateResult;  i++)  {  
            if  (mTempFileFolder.endsWith(File.separator))  {  
                temp  =  new  File(mTempFileFolder  +  tempList[i]);  
            }  
            else  {  
                temp  =  new  File(mTempFileFolder  +  File.separator  +  tempList[i]);  
            }  
            if  (temp.isFile())  {  
            	try{
                	temp.delete();
					Log.d(TAG,"the delete file name is "+tempList[i]);
                	mFreesize = getFreeSize(mTempFileFolder);
             	    mHander.sendEmptyMessage(TestHandler.UPDATE_MSG_UPDATE);
            	}catch(Exception  e){
            		 e.printStackTrace();
            		 mOprateResult = false;
            		 return;
            	}

            }  
        } 
		//mStatus = 0;
		
    	
    }
    
    
    public static boolean copyFile(String srcFileName, String destFileName){
    	 File srcFile = new File(srcFileName);
        if (!srcFile.exists()) {
			 Log.e(TAG,"the src file is not existed!!");
			 srcFile.delete();
			 //make the src file
			 if(!newFile(srcFile,mTempFileSize)){
			 	Log.e(TAG,"can't new file!!");
				return false;
			 }
             //return false;
        } else if (!srcFile.isFile()) {
        	 Log.e(TAG,"the src file is not a file!!");
             return false;
         }
        File destFile = new File(destFileName);
        if (destFile.exists()) {
        		new File(destFileName).delete();
        } else {
            if (!destFile.getParentFile().exists()) {
                if (!destFile.getParentFile().mkdirs()) {
                    return false;
                }
            }
        }
        int byteread = 0; 
        InputStream in = null;
        OutputStream out = null;

	    try {
	        in = new FileInputStream(srcFile);
	        out = new FileOutputStream(destFile);
	        byte[] buffer = new byte[1024];
	
	        while ((byteread = in.read(buffer)) != -1) {
	            out.write(buffer, 0, byteread);
	        }
	        return true;
	    } catch (FileNotFoundException e) {
	    	 e.printStackTrace();  
	        return false;
	    } catch (IOException e) {
	    	 e.printStackTrace();  
	        return false;
	    } finally {
	        try {
	            if (out != null)
	                out.close();
	            if (in != null)
	                in.close();
	        } catch (IOException e) {
	            e.printStackTrace();
	        }
	    }
    
    }
    
    
    public long getFreeSize(String path){
    	StatFs stat = new StatFs(path);
    	long total = (((long) stat.getBlockCount()) * ((long) stat.getBlockSize()));
       stat.restat(path);
    	long free = (((long) stat.getAvailableBlocks()) * ((long) stat.getBlockSize()));
    	float free_ratio = (((float) free) / ((float) total));
    	mFreerate = (int)( free_ratio *100);
    	return free;
    }

	public static boolean newFile(File path,int size){
		 if (path.exists()) {
        		//new File(destFileName).delete();
        		return true;
         } else {
            if (!path.getParentFile().exists()) {
                if (!path.getParentFile().mkdirs()) {
                    return false;
                }
            }
        }
		OutputStream out = null;
		try {
	        out = new FileOutputStream(path);
	        byte[] buffer = new byte[1024];
			for(int i=0;i<size*1024;i++){//this file is 2M 
				out.write(buffer, 0, 1024);
			}
	        return true;
	    } catch (FileNotFoundException e) {
	    	 e.printStackTrace();  
	        return false;
	    } catch (IOException e) {
	    	 e.printStackTrace();  
	        return false;
	    } finally {
	        try {
	            if (out != null)
	                out.close();
	        } catch (IOException e) {
	            e.printStackTrace();
	        }
	    }
	}
}
