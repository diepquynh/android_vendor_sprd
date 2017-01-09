/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
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

package com.ucamera.ugallery.util;

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
import android.os.Environment;
import android.os.Parcel;
import android.os.Parcelable;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Images.ImageColumns;
import android.util.Log;
import android.view.OrientationEventListener;

import com.sprd.camera.storagepath.StorageUtil;
import com.ucamera.ugallery.gallery.BaseImageList;
import com.ucamera.ugallery.gallery.IImage;
import com.ucamera.ugallery.gallery.IImageList;
import com.ucamera.ugallery.gallery.ImageList;
import com.ucamera.ugallery.gallery.ImageListUber;
import com.ucamera.ugallery.gallery.SingleImageList;
import com.ucamera.ugallery.gallery.VideoList;
import com.ucamera.ugallery.gallery.VideoObject;
import com.ucamera.ugallery.gallery.privateimage.util.Constants;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

/**
 * {@code ImageManager} is used to retrieve and store images
 * in the media content provider.
 */
public class ImageManager {
    private static final String TAG = "ImageManager";

    private static final Uri STORAGE_URI = Images.Media.EXTERNAL_CONTENT_URI;
    private static final Uri VIDEO_STORAGE_URI =
            MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
            //fix bug468584
            //Uri.parse("content://media/external/video/media");
    private static final Uri VIDEO_INTERNAL_STORAGE_URI =
            MediaStore.Video.Media.INTERNAL_CONTENT_URI;
            //fix bug468584
            //Uri.parse("content://media/internal/video/media");
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

        // This is only used if we are creating a single image list.
        public Uri mSingleImageUri;

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
            out.writeParcelable(mSingleImageUri, flags);
            out.writeInt(mIsEmptyImageList ? 1 : 0);
        }

        private ImageListParam(Parcel in) {
            mLocation = DataLocation.values()[in.readInt()];
            mInclusion = in.readInt();
            mSort = in.readInt();
            mBucketId = in.readString();
            mDateTaken = in.readString();
            mSingleImageUri = in.readParcelable(null);
            mIsEmptyImageList = (in.readInt() != 0);
        }

        @Override
        public String toString() {
            return String.format("ImageListParam{loc=%s,inc=%d,sort=%d," +
                "bucket=%s,date=%s,empty=%b}", mLocation, mInclusion,
                mSort, mBucketId, mDateTaken,mIsEmptyImageList, mSingleImageUri);
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

    public static final String EXTERNAL_STORAGE_DIRECTORY = Environment.getExternalStorageDirectory().toString();

    public static String CAMERA_IMAGE_BUCKET_NAME = EXTERNAL_STORAGE_DIRECTORY + "/DCIM/Camera";
    //for meizu photo
    public static final String IMAGE_BUCKET_NAME_PHOTO_MEIZU = EXTERNAL_STORAGE_DIRECTORY + "/Photo/";
    //for meizu default camera
    public static final String IMAGE_BUCKET_NAME_CAMERA_MEIZU = EXTERNAL_STORAGE_DIRECTORY + "/Camera";

    public static String CAMERA_IMAGE_BUCKET_ID = getBucketId(CAMERA_IMAGE_BUCKET_NAME);
    public static String CAMERA_IMAGE_BACKUP_BUCKET_ID = getBucketId(Constants.STORE_DIR_LOCKED.concat(CAMERA_IMAGE_BUCKET_NAME));
    public static String ALL_VIDEO_LOCKED_BUCKET_ID = getBucketId(Constants.STORE_DIR_LOCKED);
    public static final String IMAGE_BUCKET_NAME_PHOTO_MEIZU_ID = getBucketId(IMAGE_BUCKET_NAME_PHOTO_MEIZU);
    public static final String IMAGE_BUCKET_NAME_CAMERA_MEIZU_ID = getBucketId(IMAGE_BUCKET_NAME_CAMERA_MEIZU);

    //for meizu Ucam camera
    public static final String MEIZU_UCAM_BUCKET_ID = getBucketId(EXTERNAL_STORAGE_DIRECTORY +"/Camera/UCam");
    public static final String MEIZU_UCAM_BACKUP_BUCKET_ID = getBucketId(Constants.STORE_DIR_LOCKED.concat(EXTERNAL_STORAGE_DIRECTORY +"/Camera/UCam"));
    public static final String UCAM_BUCKET_ID = getBucketId(EXTERNAL_STORAGE_DIRECTORY +"/DCIM/UCam");
    public static final String UCAM_BACKUP_BUCKET_ID = getBucketId(Constants.STORE_DIR_LOCKED.concat(EXTERNAL_STORAGE_DIRECTORY +"/DCIM/UCam"));
    public static final String UPHOTO_BUCKET_ID = getBucketId(EXTERNAL_STORAGE_DIRECTORY +"/DCIM/UCam/UPhoto");
    public static final String UPHOTO_BACKUP_BUCKET_ID = getBucketId(Constants.STORE_DIR_LOCKED.concat(EXTERNAL_STORAGE_DIRECTORY +"/DCIM/UCam/UPhoto"));

    //for some SN devices
    public static final String SN_PHOTOGRAPHY_BUCKET_ID = StorageUtils.generateBucketId(EXTERNAL_STORAGE_DIRECTORY +"/DCIM");
    /**
     * Matches code in MediaProvider.computeBucketValues. Should be a common
     * function.
     */
    public static String getBucketId(String path) {
        return String.valueOf(path.toLowerCase().hashCode());
    }
    public static boolean isVideoBucketId(String bucketId) {
        return Constants.ALL_VIDEOS_BUCKET_ID.equals(bucketId) || ImageManager.ALL_VIDEO_LOCKED_BUCKET_ID.equals(bucketId);
    }
    public static String getBucketIdForImage(String imagePath) {
        if (imagePath != null) {
            int i = imagePath.lastIndexOf("/");
            if (i != -1) {
                imagePath = imagePath.substring(0,i);
            }
            return getBucketId(imagePath);
        }
        return null;
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
        /* SPRD: CID 109026 : Dereference before null check (REVERSE_INULL) @{ */
        SharedPreferences preferences = null;
        if(context != null) {
            preferences = PreferenceManager.getDefaultSharedPreferences(context);
        }
        // SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
        /* @} */
        if (context == null || preferences == null){
            return;
        }

        /*String defaultPath = getCameraUCamPath("");
        ImageManager.updateBucketInfo(
                preferences.getString(CameraSettings.KEY_UCAM_SELECT_PATH, defaultPath));*/
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
     * @return true if the mimetype is an image mimetype.
     */
    public static boolean isImageMimeType(String mimeType) {
        return mimeType.startsWith("image/");
    }

    /**
     * @return true if the image is an image.
     */
    public static boolean isImage(IImage image) {
        /*
         * FIX BUG: 5965
         * FIX COMMENT: avoid null pointer exception
         * DATE: 2014-02-24
         */
        if(image.getMimeType() != null) {
            return isImageMimeType(image.getMimeType());
        }
        return false;
    }

    // return the rounded orientation from degree of OrientaionListener
    // return -1 when the orientation has no obvious movement
    // return 0 when orientation < 30 or orientation > 360-30
    // return 90 when 90 - 30 < orientation < 90 + 30
    // return 180 when 180 - 30 < orientation < 180 + 30
    // return 270 when 270 - 30 < orientation < 270 + 30
    public static int roundOrientation(int orientationInput) {
        int orientation = orientationInput;

        if (orientation == OrientationEventListener.ORIENTATION_UNKNOWN) {
            // do not update the orientation
            return -1;
        }

        /*if(com.ucamera.ucam.Compatible.isHoneyCombo()) {
            //3.0/3.1/3.2 sdk version is 11/12/13
            orientation = (orientation - 90) % 360;
        } else {
            orientation = orientation % 360;
        }*/
        int retVal = -1;
        if ( (4 * 90) - 30 < orientation || orientation < (0 * 90) + 30 ) {
            retVal = 0;
        } else if ( (1 * 90) -30 < orientation && orientation < (1 * 90) + 30) {
            retVal = 90;
        } else if ((2 * 90) -30 < orientation && orientation < (2 * 90) + 30) {
            retVal = 180;
        } else if ((3 * 90) -30 < orientation && orientation < (3 * 90) + 30) {
            retVal = 270;
        }
        return retVal;
    }

    //
    // Stores a bitmap or a jpeg byte array to a file (using the specified
    // directory and filename). Also add an entry to the media store for
    // this picture. The title, dateTaken, location are attributes for the
    // picture. The degree is a one element array which returns the orientation
    // of the picture.
    //
    public static Uri addImage(ContentResolver cr, String title, long dateTaken,
            Location location, String directory, String filename,
            Bitmap source, byte[] jpegData, int[] degree, Boolean bSetDegree) {

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
            Util.closeSilently(outputStream);
        }

        return insertDB(cr, title, dateTaken, location, directory, filename, degree[0]);
    }

    public static Uri insertDB(ContentResolver cr, String title, long dateTaken,
            Location location, String directory, String filename, int degree)
    {
        // Read back the compressed file size.
        long size = new File(directory, filename).length();
        String filePath = directory + "/" + filename;

        ContentValues values = new ContentValues(9);
        values.put(Images.Media.TITLE, title);

        // That filename is what will be handed to Gmail when a user shares a
        // photo. Gmail gets the name of the picture attachment from the
        // "DISPLAY_NAME" field.
        values.put(Images.Media.DISPLAY_NAME, filename);
        values.put(Images.Media.DATE_TAKEN, dateTaken);
        values.put(Images.Media.MIME_TYPE, "image/jpeg");
        values.put(Images.Media.ORIENTATION, degree);
        values.put(Images.Media.DATA, filePath);
        values.put(Images.Media.SIZE, size);

        if (location != null) {
            values.put(Images.Media.LATITUDE, location.getLatitude());
            values.put(Images.Media.LONGITUDE, location.getLongitude());
        }

        return cr.insert(STORAGE_URI, values);
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
        if(filepath == null) {
            return 0;
        }
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

    public static IImageList makeEmptyImageList() {
        return makeImageList(null, getEmptyImageListParam());
    }

    // This is the factory function to create an image list.
    public static IImageList makeImageList(ContentResolver cr,
            ImageListParam param) {
        DataLocation location = param.mLocation;
        int inclusion = param.mInclusion;
        int sort = param.mSort;
        String bucketId = param.mBucketId;
//        String dateTaken = param.mDateTaken;
        Uri singleImageUri = param.mSingleImageUri;
        boolean isEmptyImageList = param.mIsEmptyImageList;
        if (isEmptyImageList || cr == null) {
            return new EmptyImageList();
        }

        if (singleImageUri != null) {
            return new SingleImageList(cr, singleImageUri);
        }

        // false ==> don't require write access
        boolean haveSdCard = hasStorage(false);

        // use this code to merge videos and stills into the same list
        ArrayList<BaseImageList> l = new ArrayList<BaseImageList>();

        if (haveSdCard && location != DataLocation.INTERNAL) {
            if ((inclusion & INCLUDE_IMAGES) != 0) {
                l.add(new ImageList(cr, STORAGE_URI, sort, bucketId));
            }
            if ((inclusion & INCLUDE_VIDEOS) != 0) {
                l.add(new VideoList(cr, VIDEO_STORAGE_URI, sort, bucketId));
            }
        }
        if (location == DataLocation.INTERNAL || location == DataLocation.ALL) {
            if ((inclusion & INCLUDE_IMAGES) != 0) {
                l.add(new ImageList(cr,
                        Images.Media.INTERNAL_CONTENT_URI, sort, bucketId));
            }
//            if(Util.getInternalStorageDir() != null) {
                if ((inclusion & INCLUDE_VIDEOS) != 0) {
                    if(!Models.isLaJiaoPepper())
                        l.add(new VideoList(cr, VIDEO_INTERNAL_STORAGE_URI, sort, bucketId));
                }
//            }
        }

        // Optimization: If some of the lists are empty, remove them.
        // If there is only one remaining list, return it directly.
        Iterator<BaseImageList> iter = l.iterator();
        while (iter.hasNext()) {
            BaseImageList sublist = iter.next();
            if (sublist.isEmpty()) {
                sublist.close();
                iter.remove();
            }
        }

        if (l.size() == 1) {
            BaseImageList list = l.get(0);
            return list;
        }

        ImageListUber uber = new ImageListUber(
                l.toArray(new IImageList[l.size()]), sort);
        return uber;
    }

    // This is a convenience function to create an image list from a Uri.
    public static IImageList makeImageList(ContentResolver cr, Uri uri, int sort) {
        String uriString = (uri != null) ? uri.toString() : "";
        String bucketId = (uri != null) ?uri.getQueryParameter("bucketId"):null;

        /*if (uriString.startsWith("content://drm")) {
            return makeImageList(cr, DataLocation.ALL, INCLUDE_DRM_IMAGES, sort, null);
        } else */if (uriString.startsWith("content://media/external/video")) {
            return makeImageList(cr, DataLocation.EXTERNAL, INCLUDE_VIDEOS, sort, null);
        } else if (isSingleImageMode(uriString) || bucketId == null ) {
            return makeSingleImageList(cr, uri);
        } else {
            return makeImageList(cr, DataLocation.ALL, INCLUDE_IMAGES, sort, bucketId);
        }
    }

    public static IImageList makeSingleImageList(ContentResolver cr, Uri uri) {
        return makeImageList(cr, getSingleImageListParam(uri));
    }

    static boolean isSingleImageMode(String uriString) {
        return !uriString.startsWith(MediaStore.Images.Media.EXTERNAL_CONTENT_URI.toString())
                && !uriString.startsWith(MediaStore.Images.Media.INTERNAL_CONTENT_URI.toString());
    }

    public static ImageListParam getSingleImageListParam(Uri uri) {
        ImageListParam param = new ImageListParam();
        param.mSingleImageUri = uri;
        return param;
    }

    private static class EmptyImageList implements IImageList {
        public void close() {
        }

        public int getCount() {
            return 0;
        }

        public IImage getImageAt(int i) {
            return null;
        }

        @Override
        public HashMap<String, String> getBucketIds() {
            return null;
        }

        public HashMap<String, String> getBaseUri(){
            return null;
        };
        @Override
        public boolean isEmpty() {
            return false;
        }

        @Override
        public IImage getImageForUri(Uri uri) {
            return null;
        }

        @Override
        public boolean removeImage(IImage image) {
            return false;
        }

        @Override
        public boolean removeImageAt(int i) {
            return false;
        }

        @Override
        public int getImageIndex(IImage image) {
            return 0;
        }
    }

    public static ImageListParam getEmptyImageListParam() {
        ImageListParam param = new ImageListParam();
        param.mIsEmptyImageList = true;
        return param;
    }

    public static ImageListParam getImageListParam(DataLocation location, int inclusion, int sort,
            String bucketId) {
        ImageListParam param = new ImageListParam();
        param.mLocation = location;
        param.mInclusion = inclusion;
        param.mSort = sort;
        param.mBucketId = bucketId;
        return param;
    }

    public static ImageListParam getImageListParam(DataLocation location,
         int inclusion, int sort, String bucketId,String dateTaken) {
         ImageListParam param = new ImageListParam();
         param.mLocation = location;
         param.mInclusion = inclusion;
         param.mSort = sort;
         param.mBucketId = bucketId;
         param.mDateTaken = dateTaken;
         return param;
    }

    public static ImageManager.ImageListParam getImageListParam(String date, String bucketId) {
        if(Util.getInternalStorageDir() != null){
            return ImageManager.getImageListParam(DataLocation.ALL, ImageManager.INCLUDE_IMAGES,
                    ImageManager.SORT_DESCENDING, bucketId, date);
        }else{
            return ImageManager.getImageListParam(DataLocation.EXTERNAL, ImageManager.INCLUDE_IMAGES,
                    ImageManager.SORT_DESCENDING, bucketId, date);
        }

    }

    public static IImageList makeImageList(ContentResolver cr,
            DataLocation location, int inclusion, int sort, String bucketId,String dateTaken) {
        ImageListParam param = getImageListParam(location, inclusion, sort,
                bucketId,dateTaken);
        return makeImageList(cr, param);
    }

    public static IImageList makeImageList(ContentResolver cr, DataLocation location,
            int inclusion, int sort, String bucketId) {
        ImageListParam param = getImageListParam(location, inclusion, sort, bucketId);
        return makeImageList(cr, param);
    }

    public static ImageListParam getAllImages(boolean storageAvailable, String bucketId) {
        if (!storageAvailable) {
            return getEmptyImageListParam();
        } else {
             Uri uri = Images.Media.INTERNAL_CONTENT_URI.buildUpon().appendQueryParameter("bucketId", bucketId).build();
             return getImageListParam(
                    DataLocation.EXTERNAL,
                    INCLUDE_IMAGES,
                    SORT_DESCENDING,
                    (uri != null) ? uri.getQueryParameter("bucketId") : null);
        }
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
        //SPRD:fix bug537451 pull sd card, edit and puzzle can not work.
        //String state = Environment.getExternalStorageState();
        String state = StorageUtil.getInstance().getStorageState();
//        if(Models.Samsung_GT_I9508.equals(Models.getModel()) || Models.Samsung_GT_I9500.equals(Models.getModel()) ||
//                com.ucamera.ugallery.util.Models.HUAWEI_P6.equals(com.ucamera.ugallery.util.Models.getModel())) {
//            String external = Util.getExternalStorageDir();
//            File file = new File(external);
//            if(file.exists()) {
//                if(requireWriteAccess) {
//                    return checkFsWritable();
//                } else {
//                    return file.canRead();
//                }
//            } else {
//                return false;
//            }
//        }
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
        Log.d(TAG, "media scanning: " + result);
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
     * @param uphotoPath uphoto directory
     * @return target path, eg:/mnt/sdcard/Camera/UCam, /mnt/sdcard/Camera/UCam/UPhoto,
     * /mnt/sdcard/DCIM/UCam, /mnt/sdcard/DCIM/UCam/UPhoto
     */
    public static String getCameraUCamPath(String uphotoPath) {
        String defaultPath = null;
//        if (Compatible.instance().mIsMeizuManufacturer){
//            /*
//             * FIX BUG: 320
//             * BUG CAUSE: m9 need a directory with parent of /sdcard/camera
//             * FIX COMMENT: change default dir to /sdcard/camera/UCam for m9
//             * DATE: 2011-12-29
//             */
//            defaultPath = EXTERNAL_STORAGE_DIRECTORY + "/Camera/UCam" + uphotoPath;
//        } else if(Compatible.instance().mIsN06C) {
//            /**
//             * FIX BUG: 1346
//             * BUG CAUSE: When the storage path is /mnt/sdcard/DCIM or subdirectories,
//             *            photographs are stored to Photography automatically.
//             * FIX COMMENT: Set the default storage path is /mnt/sdcard/UCam. but if user
//             *            choose /mnt/sdcard/DCIM/ or subdirectories to set storage path, this bug will reproduce.
//             * DATE: 2012-07-27
//             */
//            defaultPath = EXTERNAL_STORAGE_DIRECTORY + "/UCam/" + uphotoPath;
//        } else{
//            /*
//             * FIX BUG: 325
//             * BUG CAUSE: should use /mnt/sdcard to replace the /sdcard
//             * FIX COMMENT: use EXTERNAL_STORAGE_DIRECTORY
//             * DATE: 2011-12-29
//             */
//            defaultPath = EXTERNAL_STORAGE_DIRECTORY + "/DCIM/UCam" + uphotoPath;
//        }
        return defaultPath;
    }
    /**
     * @return true if the image is a video.
     */
    public static boolean isVideo(IImage image) {
        // This is the right implementation, but we use instanceof for speed.
        //return isVideoMimeType(image.getMimeType());
        return (image instanceof VideoObject);
    }

    public static boolean updateRenamedImage(ContentResolver resolver, String newPath, String title, String suffix, IImage image, String currentStoragePos) {
        // Save the image.
        // Insert into MediaStore.
        ContentValues values = new ContentValues();
        values.put(ImageColumns.TITLE, title);
        values.put(ImageColumns.DISPLAY_NAME, title.concat(suffix));
        values.put(ImageColumns.DATA, newPath);
        try {
            int row = -1;
            Uri uri = null;
            if ("internal".equals(currentStoragePos)|| image.fullSizeImageUri().toString().contains(Images.Media.INTERNAL_CONTENT_URI
                                            .toString())) {
                if (ImageManager.isImage(image)) {
                    uri = Images.Media.INTERNAL_CONTENT_URI;
                } else {
                    uri = MediaStore.Video.Media.INTERNAL_CONTENT_URI;
                  //uri = Uri.parse("content://media/internal/video/media");
                }
            } else {
                if (ImageManager.isImage(image)) {
                    uri = Images.Media.EXTERNAL_CONTENT_URI;
                } else {
                    uri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
                  //uri = Uri.parse("content://media/external/video/media");
                }
            }
            row = resolver.update(uri, values,
                    Images.Media._ID + "=?",
                    new String[] { String.valueOf(image.getImageId())});
            return row == 1;
        } catch (Throwable th) {
            Log.d(TAG, "Failed to update image" + th);
            return false;
        }
    }
}
