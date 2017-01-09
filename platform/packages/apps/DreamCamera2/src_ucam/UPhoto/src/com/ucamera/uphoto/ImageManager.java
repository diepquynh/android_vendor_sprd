/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 *
 *
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.location.Location;
import android.media.ExifInterface;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.Parcel;
import android.os.Parcelable;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.provider.MediaStore.Images;
import android.text.TextUtils;
import android.util.Log;

import com.android.camera.util.CameraUtil;
import com.sprd.camera.storagepath.StorageUtil;
import com.sprd.camera.storagepath.StorageUtilProxy;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;

/**
 * {@code ImageManager} is used to retrieve and store images
 * in the media content provider.
 */
public class ImageManager {
    private static final String TAG = "ImageManager";

    private static final Uri STORAGE_URI = Images.Media.EXTERNAL_CONTENT_URI;
    private static final String MODEL_NAME = Build.MODEL.replace('-', '_').replace(' ', '_');
    private ImageManager() {
    }

    /**
     * {@code ImageListParam} specifies all the parameters we need to create an
     * image list (we also need a ContentResolver).
     */
    public static class ImageListParam implements Parcelable {
        public DataLocation mLocation;
        public int mInclusion;
        public int mSort;
        public String mBucketId;
        public String mDateTaken;

        // This is only used if we are creating an empty image list.
        public boolean mIsEmptyImageList;

        public ImageListParam() {
        }

        public void writeToParcel(Parcel out, int flags) {
            out.writeInt(mLocation.ordinal());
            out.writeInt(mInclusion);
            out.writeInt(mSort);
            out.writeString(mBucketId);
            out.writeString(mDateTaken);
            out.writeInt(mIsEmptyImageList ? 1 : 0);
        }

        private ImageListParam(Parcel in) {
            mLocation = DataLocation.values()[in.readInt()];
            mInclusion = in.readInt();
            mSort = in.readInt();
            mBucketId = in.readString();
            mDateTaken = in.readString();
            mIsEmptyImageList = (in.readInt() != 0);
        }

        @Override
        public String toString() {
            return String.format("ImageListParam{loc=%s,inc=%d,sort=%d," +
                "bucket=%s,date=%s,empty=%b}", mLocation, mInclusion,
                mSort, mBucketId, mDateTaken,mIsEmptyImageList);
        }

        public static final Parcelable.Creator<ImageListParam> CREATOR
                = new Parcelable.Creator<ImageListParam>() {
            public ImageListParam createFromParcel(Parcel in) {
                return new ImageListParam(in);
            }

            public ImageListParam[] newArray(int size) {
                return new ImageListParam[size];
            }
        };

        public int describeContents() {
            return 0;
        }
    }

    // Location
    public static enum DataLocation { NONE, INTERNAL, EXTERNAL, ALL }

    // Inclusion
    public static final int INCLUDE_IMAGES = (1 << 0);
    public static final int INCLUDE_VIDEOS = (1 << 2);

    // Sort
    public static final int SORT_ASCENDING = 1;
    public static final int SORT_DESCENDING = 2;

    public static final String EXTERNAL_STORAGE_DIRECTORY = getSystemExternalStorageDir();

    private static String CAMERA_IMAGE_BUCKET_NAME = EXTERNAL_STORAGE_DIRECTORY + "/DCIM/Camera";
    //for meizu photo
    public static final String IMAGE_BUCKET_NAME_PHOTO_MEIZU = EXTERNAL_STORAGE_DIRECTORY + "/Photo/";
    //for meizu default camera
    public static final String IMAGE_BUCKET_NAME_CAMERA_MEIZU = EXTERNAL_STORAGE_DIRECTORY + "/Camera";

    public static String CAMERA_IMAGE_BUCKET_ID = getBucketId(CAMERA_IMAGE_BUCKET_NAME);
    public static final String IMAGE_BUCKET_NAME_PHOTO_MEIZU_ID = getBucketId(IMAGE_BUCKET_NAME_PHOTO_MEIZU);
    public static final String IMAGE_BUCKET_NAME_CAMERA_MEIZU_ID = getBucketId(IMAGE_BUCKET_NAME_CAMERA_MEIZU);

    private static String getSystemExternalStorageDir() {
        //SPRD:Fix bug 561234 The edit is invalid.
        String external = StorageUtilProxy.getInternalStoragePath().toString();
//        if(Models.Samsung_GT_I9508.equals(Models.getModel()) || Models.Samsung_GT_I9500.equals(Models.getModel())) {
//            external = "/storage/extSdCard";
//        } else if(Models.HUAWEI_P6.equals(Models.getModel()) || Models.LAJIAO_LA3_W.equals(Models.getModel())) {
//            external = "/storage/sdcard1";
//        }
        return external;
    }
    /**
     * Matches code in MediaProvider.computeBucketValues. Should be a common
     * function.
     */
    public static String getBucketId(String path) {
        return String.valueOf(path.toLowerCase().hashCode());
    }

    /**
     * update bucket name and id
     * @param path : the new bucket name path
     */
    public synchronized static void updateBucketInfo(String path){
        if ((path != null) && path.endsWith("/") && (path.length() > 1)){
            path = path.substring(0,path.length() - 1);
        }
        CAMERA_IMAGE_BUCKET_NAME = path;
        CAMERA_IMAGE_BUCKET_ID = getBucketId(CAMERA_IMAGE_BUCKET_NAME);
    }

    /**
     * update bucket name and id according the sharedpreferences
     * @param context : the context from which we get the preferences
     */
    public synchronized static void updateBucketInfo(Context context){
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
        if (context == null || preferences == null){
            return;
        }

        String defaultPath = getCameraUCamPath();
        ImageManager.updateBucketInfo(
                preferences.getString(Const.KEY_UCAM_SELECT_PATH, defaultPath));
    }

    /**
     * get the bucket name
     */
    public synchronized static String getCameraImageBucketName(){
        return CAMERA_IMAGE_BUCKET_NAME;
    }

    /**
     * get the bucket id
     */
    public synchronized static String getCameraImageBucketId(){
        return CAMERA_IMAGE_BUCKET_ID;
    }

    /**
     * get the subdirectory name
     */
    public static ArrayList<String> getCameraImageSubDirectoryName(){
        ArrayList<String> fileDirList = new ArrayList<String>();
        File[] fileList = new File(CAMERA_IMAGE_BUCKET_NAME).listFiles();
        if(fileList == null || fileList.length == 0) return null;
        for(int i=0;i<fileList.length;i++){
            if(fileList[i].isDirectory()){
                fileDirList.add(fileList[i].toString());
            }
        }
        if(fileDirList == null || fileDirList.size() == 0) return null;
        return fileDirList;
    }
    /**
     * Get UCam photo storage BucketName on M9 platform
     * @return storage BucketName
     */
    public static String getMeizuUCamName() {
        String meizuUCamName = ImageManager.getCameraImageBucketName();
        return meizuUCamName.substring(meizuUCamName.lastIndexOf("/") + 1);
    }

    /**
     * OSX requires plugged-in USB storage to have path /DCIM/NNNAAAAA to be
     * imported. This is a temporary fix for bug#1655552.
     */
    public static void ensureOSXCompatibleFolder() {
        File nnnAAAAA = new File(EXTERNAL_STORAGE_DIRECTORY + "/DCIM/100ANDRO");
        if ((!nnnAAAAA.exists()) && (!nnnAAAAA.mkdir())) {
            Log.e(TAG, "create NNNAAAAA file: " + nnnAAAAA.getPath()
                    + " failed");
        }
    }


    /**
     * Stores a bitmap or a jpeg byte array to a file (using the specified
     * directory and filename). Also add an entry to the media store for
     * this picture. The title, dateTaken, location are attributes for the
     * picture. The degree is a one element array which returns the orientation
     * of the picture.
     * @param cr the ContentResolver object to resolve
     * @param title the picture title name
     * @param dateTaken the data of picture token
     * @param location the location infomation of picture token
     * @param directory the directory of picture saved
     * @param filename the file name of picture
     * @param degree the orientation of picture
     * @param originalUri the Uri of picture
     * @param source the bitmap source to save
     * @param jpegData the jpegData to save
     * @param bSetDegree whether set degree of picture, if false ,will use 0
     * @return the Uri of saved image
     */
    public static Uri addImage(ContentResolver cr, String title, long dateTaken,
            Location location, String directory, String filename,
            Bitmap source, byte[] jpegData, int[] degree, Boolean bSetDegree, Uri originalUri) {

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
                /*
                 * BUG FIX: 1063
                 * FIX COMMENT: if data is null, not insert
                 * DATE: 2012-06-01
                 */
                if ( jpegData == null ) {
                    return null;
                }

                outputStream.write(jpegData);
                if (!bSetDegree){ // not continous mode
                    degree[0] = getExifOrientation(filePath);
                }else {
                    setExifOrientation(filePath, degree[0]);
                }
            }
        } catch (FileNotFoundException ex) {
            Log.w(TAG, ex);
            return null;
        } catch (IOException ex) {
            Log.w(TAG, ex);
            return null;
        } finally {
            Utils.closeSilently(outputStream);
        }

        return insertDB(cr, title, dateTaken, location, directory, filename, degree[0], originalUri);
    }

    /**
     * insert full infomation for voice picture
     * @param cr the ContentResolver object to resolve
     * @param title the picture title name
     * @param dateTaken the data of picture token
     * @param location the location infomation of picture token
     * @param directory the directory of picture saved
     * @param filename the file name of picture
     * @param degree the orientation of picture
     * @param originalUri the Uri of picture
     * @return Uri object of the picture
     */
    public static Uri insertDB(ContentResolver cr, String title, long dateTaken,
            Location location, String directory, String filename, int degree, Uri originalUri) {
        // Read back the compressed file size.
        long size = new File(directory, filename).length();
        String filePath = directory + "/" + filename;

        ContentValues values = new ContentValues();

        values.put(Images.Media.TITLE, title);
        /*
         * That filename is what will be handed to Gmail when a user shares a
         * photo. Gmail gets the name of the picture attachment from the "DISPLAY_NAME" field.
         */
        values.put(Images.Media.DISPLAY_NAME, filename);
        values.put(Images.Media.DATE_TAKEN, dateTaken);
        values.put(Images.Media.MIME_TYPE, "image/jpeg");
        // TODO : add it if needed
        // values.put(ImageColumns.DATE_MODIFIED, dateModifiedSeconds);
        values.put(Images.Media.ORIENTATION, degree);
        values.put(Images.Media.DATA, filePath);
        values.put(Images.Media.SIZE, size);

        if (location != null) {
            values.put(Images.Media.LATITUDE, location.getLatitude());
            values.put(Images.Media.LONGITUDE, location.getLongitude());
        }
        if (CameraUtil.isVoicePhotoEnable()) {
            /* SPRD: Fix bug 605951, lost audio file when edit voice photo @{ */
            InputStream voiceIn = null;
            OutputStream voiceOut = null;
            if (originalUri != null) {
                long start = System.currentTimeMillis();
                Cursor imageCursor = cr.query(originalUri, new String[] {
                    "photo_voice_id"
                }, null, null, null);
                Cursor voiceCursor = null;
                try {
                    if (imageCursor != null && imageCursor.moveToFirst()) {
                        // NOTE: DO NOT use getInt() because it will return 0,
                        // when the value is null
                        String id = imageCursor.getString(0);
                        if (!TextUtils.isEmpty(id)) {
                            voiceCursor = cr.query(MediaStore.Files.getContentUri("external",
                                    Integer.parseInt(id)), new String[] {
                                MediaStore.Audio.Media.DATA
                            }, null, null, null);
                            if (voiceCursor != null && voiceCursor.moveToFirst()) {
                                String path = voiceCursor.getString(0);
                                String voiceDirectory = StorageUtil.getInstance()
                                        .getPhotoVoiceDirectory();
                                String photoVoicePath = voiceDirectory + "/"
                                        + filename.substring(0, filename.lastIndexOf(".")) + ".amr";
                                voiceIn = new FileInputStream(path);
                                voiceOut = new FileOutputStream(photoVoicePath);
                                byte[] buffer = new byte[1024];
                                int length;
                                long begin = System.currentTimeMillis();
                                while ((length = voiceIn.read(buffer)) != -1) {
                                    voiceOut.write(buffer, 0, length);
                                    voiceOut.flush();
                                }
                                long spent = System.currentTimeMillis() - begin;
                                if (spent > 100) {
                                    Log.i(TAG, "Edit voice photo, copy audio file spent:" + spent
                                            + "ms");
                                }
                                ContentValues voiceVal = new ContentValues();
                                voiceVal.put(MediaStore.Audio.Media.DATA, photoVoicePath);
                                Uri voiceUri = cr.insert(
                                        MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, voiceVal);
                                values.put("photo_voice_id", voiceUri.getPathSegments().get(3));
                            }
                        }
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                } finally {
                    if (imageCursor != null) {
                        imageCursor.close();
                    }
                    if (voiceCursor != null) {
                        voiceCursor.close();
                    }
                    Utils.closeSilently(voiceIn);
                    Utils.closeSilently(voiceOut);
                }
                long cost = System.currentTimeMillis() - start;
                if (cost > 100) {
                    Log.i(TAG, "Edit voice photo, copy audio file and insert database cost:" + cost
                            + "ms");
                }
            }
        }
        /* @} */

//        if(isExternalStorage(directory)) {
            return cr.insert(STORAGE_URI, values);
//        }else {
//            return cr.insert(Media.INTERNAL_CONTENT_URI, values);
//        }
    }

    public static boolean isExternalStorage(String dir){
        if(dir.startsWith(EXTERNAL_STORAGE_DIRECTORY)){
            return true;
        }
        if("IS12S".equals(Build.MODEL)) {
            return true;
        }
        return false;
    }

    public static void setExifOrientation(String filepath, int orientation) {
        ExifInterface exif = null;
        //if (orientation != 90 && orientation != 180 && orientation != 270){
        //    return;
        //}
        try {
            exif = new ExifInterface(filepath);
        } catch (IOException ex) {
            Log.e(TAG, "cannot read exif", ex);
        }


        if (exif != null) {
            int degree = ExifInterface.ORIENTATION_NORMAL;
            switch(orientation) {
                case 90:
                    degree = ExifInterface.ORIENTATION_ROTATE_90;
                    break;
                case 180:
                    degree = ExifInterface.ORIENTATION_ROTATE_180;
                    break;
                case 270:
                    degree = ExifInterface.ORIENTATION_ROTATE_270;
                    break;
            }

            exif.setAttribute(
                ExifInterface.TAG_ORIENTATION, Integer.toString(degree));
            try {
                exif.saveAttributes();
            } catch (IOException ex) {
                Log.e(TAG, "cannot save exif", ex);
            }
        }
        return;
    }

    public static int getExifOrientation(String filepath) {
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

    private static boolean checkFsWritable() {
        // Create a temporary file to see whether a volume is really writeable.
        // It's important not to put it in the root directory which may have a
        // limit on the number of files.
        String directoryName = EXTERNAL_STORAGE_DIRECTORY + "/DCIM";
        File directory = new File(directoryName);
        if (!directory.isDirectory()) {
            if (!directory.mkdirs()) {
                Log.e(TAG, "Can not create dir in " + EXTERNAL_STORAGE_DIRECTORY);
                return false;
            }
        }
        boolean canWrite = directory.canWrite();
        if (!canWrite) {
            Log.e(TAG, "Can not write in dir of " + EXTERNAL_STORAGE_DIRECTORY);
        }
        return canWrite;
    }

    public static int deleteUri(ContentResolver cr, Uri url, String where, String[] selectionArgs) {
        if (cr == null || url == null) {
            return -1;
        }

        return cr.delete(url, where, selectionArgs);
    }

    public static Uri registerResultFile(ContentResolver cr, String title, long dateTaken,
            Location location, String directory, String filename, int []degree) {
        String filePath = directory + "/" + filename;
            // Read back the compressed file size.
        long size = new File(directory, filename).length();

        ContentValues values = new ContentValues(9);
        values.put(Images.Media.TITLE, title);
        degree[0] = getExifOrientation(filePath);

        // That filename is what will be handed to Gmail when a user shares a
        // photo. Gmail gets the name of the picture attachment from the
        // "DISPLAY_NAME" field.
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

        return cr.insert(STORAGE_URI, values);
    }

    public static boolean hasStorage() {
        return hasStorage(true);
    }

    public static boolean hasStorage(boolean requireWriteAccess) {
        String state = Environment.getExternalStorageState();

        if (Environment.MEDIA_MOUNTED.equals(state)) {
            if (requireWriteAccess) {
                boolean writable = checkFsWritable();
                return writable;
            } else {
                return true;
            }
        } else if (!requireWriteAccess
                && Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
            return true;
        }
        return false;
    }

    private static Cursor query(ContentResolver resolver, Uri uri,
            String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        try {
            if (resolver == null) {
                return null;
            }
            return resolver.query(uri, projection, selection, selectionArgs, sortOrder);
         } catch (UnsupportedOperationException ex) {
            return null;
        }

    }

    public static boolean isMediaScannerScanning(ContentResolver cr) {
        boolean result = false;
        Cursor cursor = query(cr, MediaStore.getMediaScannerUri(),
                new String [] {MediaStore.MEDIA_SCANNER_VOLUME},
                null, null, null);
        if (cursor != null) {
            if (cursor.getCount() == 1) {
                cursor.moveToFirst();
                result = "external".equals(cursor.getString(0));
            }
            cursor.close();
        }

        return result;
    }

    public static String getLastImageThumbPath() {
        return EXTERNAL_STORAGE_DIRECTORY + "/DCIM/.thumbnails/image_last_thumb";
    }

    public static String getLastVideoThumbPath() {
        return EXTERNAL_STORAGE_DIRECTORY + "/DCIM/.thumbnails/video_last_thumb";
    }

    public static String getTempJpegPath() {
        return EXTERNAL_STORAGE_DIRECTORY + "/DCIM/.tempjpeg";
    }

    /**
     * get UCam root directroy, /mnt/sdcard/UCam
     * @return UCam directory
     */
    public static String getUCamPath() {
        return EXTERNAL_STORAGE_DIRECTORY + "/UCam";
    }

    /**
     * the default keep taking picture path
     * @return target path, eg:/mnt/sdcard/Camera/UCam, /mnt/sdcard/Camera/UCam/UPhoto,
     * /mnt/sdcard/DCIM/UCam, /mnt/sdcard/DCIM/UCam/UPhoto
     */
    public static String getCameraUCamPath() {
        String defaultPath = null;
       if ("X907".equalsIgnoreCase(MODEL_NAME)){
           defaultPath = EXTERNAL_STORAGE_DIRECTORY +"/我的照片/UCam";
        } else if (isMEIZU(MODEL_NAME)) {
            defaultPath = EXTERNAL_STORAGE_DIRECTORY + "/Camera/UCam";
        }else {
            defaultPath = EXTERNAL_STORAGE_DIRECTORY + "/DCIM/";
            android.util.Log.e(TAG, "getCameraUCamPath fail, used the defaultPath= " + defaultPath);
        }
        return defaultPath;
    }

    public static boolean isMEIZU(String model) {
        return Arrays.asList(new String[]{
            "M030","M031","M032","MEIZU_MX","MX", "M9", "M040"
        }).contains(model);
    }
}
