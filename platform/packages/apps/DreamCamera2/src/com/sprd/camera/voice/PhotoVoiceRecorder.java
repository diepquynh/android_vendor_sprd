package com.sprd.camera.voice;

import java.io.File;

import android.util.Log;
import android.widget.Toast;

import com.sprd.camera.storagepath.StorageUtil;
import com.android.camera.app.AppController;
import com.android.camera.app.MediaSaver;
import com.android.camera.app.MediaSaver.OnMediaSavedListener;
import com.android.camera.exif.ExifInterface;
import com.android.camera.ui.PhotoVoiceRecordProgress;
import com.android.camera2.R;

import android.location.Location;
import android.media.AudioManager;
import android.media.MediaRecorder;
import android.os.Handler;

public class PhotoVoiceRecorder {
    private static final String TAG = "PhotoVoiceRecorder";
    private MediaRecorder mMediaRecorder = null;
    private String mRecordAudioFile;
    private Handler mHandler = null;
    private byte[] mRecordJpeg;
    private String mRecordTitle;
    private long mRecordDate;
    private int mRecordWidth, mRecordHeight, mRecordOrientation;
    private ExifInterface mRecordExif;
    private Location mRecordLocation;
    private MediaSaver mMediaSaver;
    private OnMediaSavedListener mOnMediaSavedListener;
    private boolean mFilterHandle = false;
    private AppController mAppController;

    public PhotoVoiceRecorder(AppController app) {
        mAppController = app;
    }

    private void initlizeMediaRecorder() {
        mMediaRecorder = new MediaRecorder();
        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.AMR_NB);
        mMediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AMR_NB);
        mMediaRecorder.setMaxDuration(PhotoVoiceRecordProgress.LIMIT_TIME * 1000);
        mMediaRecorder.setOnErrorListener(new MediaRecorder.OnErrorListener() {

            @Override
            public void onError(MediaRecorder mr, int what, int extra) {
                // TODO Auto-generated method stub
                Log.i(TAG, "MediaRecorder error. what=" + what + ". extra=" + extra);
                if (what == MediaRecorder.MEDIA_RECORDER_ERROR_UNKNOWN) {
                     stopAudioRecord();
                }
            }
        });
        mMediaRecorder.setOnInfoListener(new MediaRecorder.OnInfoListener() {

            @Override
            public void onInfo(MediaRecorder mr, int what, int extra) {
                // TODO Auto-generated method stub
                if (what == MediaRecorder.MEDIA_RECORDER_INFO_MAX_DURATION_REACHED) {
                    stopAudioRecord();
                } else if (what == MediaRecorder.MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED) {
                    stopAudioRecord();
                }
            }
        });
        Log.i(TAG, "startRecordAudio initFinished , startRecord");
    }

    public boolean startAudioRecord() {
        Log.i(TAG, "startAudioRecord");
        if (mMediaRecorder != null) {
            Log.i(TAG, "startRecordAudio mMediaRecorder != null");
            return false;
        }
        StorageUtil util = StorageUtil.getInstance();
        String directory = util.getPhotoVoiceDirectory();
        try {
            File f = new File(directory);
            f.mkdirs();
        } catch (Exception e) {
            Log.i(TAG, "startRecordAudio mkdirs failed!");
            return false;
        }
        mRecordAudioFile = directory + "/" + mRecordTitle + ".amr";
        Log.i(TAG, "startRecordAudio mRecordAudioFile = " + mRecordAudioFile);
        if (mMediaRecorder == null) {
            initlizeMediaRecorder();
        }
        mMediaRecorder.setOutputFile(mRecordAudioFile);

        try {
            if (requestAudioFocus() == AudioManager.AUDIOFOCUS_REQUEST_FAILED) {
                Toast.makeText(
                        mAppController.getAndroidContext(),
                        mAppController.getAndroidContext().getResources()
                                .getString(R.string.camera_record_voice_failed),
                        Toast.LENGTH_LONG).show();
                mMediaRecorder = null;
                return false;
            }
            mMediaRecorder.prepare();
            Log.i(TAG, "startRecordAudio start");
            mMediaRecorder.start();
        } catch (IllegalStateException exception) {
            Log.e(TAG, "Could not start media recorder(start failed). ", exception);
            Toast.makeText(mAppController.getAndroidContext(),
                    mAppController.getAndroidContext().getResources().getString(R.string.camera_record_voice_error),
                    Toast.LENGTH_LONG).show();
            if (mMediaRecorder != null) {
                abandonAudioFocus();
                try {
                    mMediaRecorder.release();
                } catch (RuntimeException e) {
                    Log.e(TAG, "release fail", e);
                }
                mMediaRecorder = null;
                mHandler.sendEmptyMessage(PhotoVoiceMessage.MSG_RECORD_STOPPED);
            }
            return false;
        } catch (Exception e) {
            Log.i(TAG, "startRecordAudio Exception : e = " + e.toString());
            mMediaRecorder = null;
            return false;
        }
        Log.i(TAG, "startRecordAudio startRecord success!");
        return true;
    }

    public synchronized void stopAudioRecord() {
        Log.i(TAG, "stopRecordAudio");
        if (mMediaRecorder != null) {
            abandonAudioFocus();
            Log.i(TAG, "stopRecordAudio begin");
            try {
                mMediaRecorder.stop();//SPRD:fix bug598422
            } catch (RuntimeException e) {
                Log.e(TAG, "stop fail", e);
            }
            Log.i(TAG, "stopRecordAudio end");
            mMediaRecorder.release();
            mMediaRecorder = null;
            if (mFilterHandle) {
                savePhoto(mRecordAudioFile, mFilterHandle);
            } else {
                savePhoto(mRecordAudioFile);
            }
            mHandler.sendEmptyMessage(PhotoVoiceMessage.MSG_RECORD_STOPPED);
        }
    }

    public void setHandler(Handler h){
        mHandler = h;
    }

    public void setFilterHandle(boolean filterHandle) {
        mFilterHandle = filterHandle;
    }

    public void initData(final byte[] jpegData, String title, long date,
            int width, int height, int orientation, ExifInterface exif,
            Location location, OnMediaSavedListener l, MediaSaver mediaSaver) {
        mRecordJpeg = jpegData;
        mRecordTitle = title;
        mRecordDate = date;
        mRecordWidth = width;
        mRecordHeight = height;
        mRecordOrientation = orientation;
        mRecordExif = exif;
        mRecordLocation = location;
        mOnMediaSavedListener = l;
        mMediaSaver = mediaSaver;
    }

    public void savePhoto(String recordAudioFile) {
        mMediaSaver.addImage(mRecordJpeg, mRecordTitle,
                mRecordDate, mRecordLocation, mRecordWidth, mRecordHeight,
                mRecordOrientation, mRecordExif, mOnMediaSavedListener,
                recordAudioFile);
    }

    public void savePhoto(String recordAudioFile, boolean filterHandle) {
        mMediaSaver.addImage(mRecordJpeg, mRecordTitle,
                mRecordDate, mRecordLocation, mRecordWidth, mRecordHeight,
                mRecordOrientation, mRecordExif, mOnMediaSavedListener,
                recordAudioFile, filterHandle);
    }

    private int requestAudioFocus() {
        return ((AudioManager) mAppController.getAndroidContext()
                .getSystemService(
                        mAppController.getAndroidContext().AUDIO_SERVICE))
                .requestAudioFocus(null, AudioManager.STREAM_MUSIC,
                        AudioManager.AUDIOFOCUS_GAIN);
    }

    private void abandonAudioFocus() {
        ((AudioManager) mAppController.getAndroidContext().getSystemService(
                mAppController.getAndroidContext().AUDIO_SERVICE))
                .abandonAudioFocus(null);
    }
}
