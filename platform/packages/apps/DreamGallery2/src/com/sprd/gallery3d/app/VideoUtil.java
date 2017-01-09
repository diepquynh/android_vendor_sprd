/**
 * Created By Spreadst
 * */

package com.sprd.gallery3d.app;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;

import android.text.format.Formatter;
import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteException;
import android.net.Uri;
import android.provider.MediaStore;
import android.provider.MediaStore.Video.VideoColumns;
import android.provider.SyncStateContract.Columns;
import android.util.Log;
import android.app.Activity;

import com.android.gallery3d.R;

public class VideoUtil {
    private static final String TAG = "VideoUtil";
    private static final String DATABASE_TABLE = "content://com.sprd.gallery3d.app.VideoBookmarkProvider/bookmarks";
    private static long mLastClickTime;
    /**
     * get all videos in mobile phone
     * @return ArrayList of the videos in mobile phone
     * */
    public static ArrayList<VideoItems> getVideoList(Context context) {

        ArrayList<VideoItems> videoList = new ArrayList<VideoItems>();
        final String[] PROJECTION = new String[] {
                VideoColumns._ID,
                VideoColumns.TITLE,
                VideoColumns.MIME_TYPE,
                VideoColumns.LATITUDE,
                VideoColumns.LONGITUDE,
                VideoColumns.DATE_TAKEN,
                VideoColumns.DATE_ADDED,
                VideoColumns.DATE_MODIFIED,
                VideoColumns.DATA,
                VideoColumns.DURATION,
                VideoColumns.BUCKET_ID,
                VideoColumns.SIZE,
                VideoColumns.RESOLUTION,
                VideoColumns.DISPLAY_NAME
        };
        ContentResolver resolver = context.getContentResolver();
        Cursor cursor = null;
        try {
            cursor = resolver.query(MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
                    PROJECTION, null, null, null);
            for (int i = 0; i < cursor.getCount(); i++) {
                cursor.moveToNext();
                int id = cursor.getInt(0);
                String duration = cursor.getString(9);
                String url = cursor.getString(8);
                String displayName;
                displayName = cursor.getString(13);
                /*SPRD:Bug611400 The video type of "vob" can't show title@{*/
                if(displayName == null || "".equals(displayName)) {
                    if (url == null || "".equals(url)) {
                        displayName = "";
                    } else {

                        Uri uri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI.buildUpon().
                                appendPath(String.valueOf(id)).build().parse(url);
                        displayName = uri.getLastPathSegment();
                    }
                }
                /*Bug611400@}*/
                long width = cursor.getLong(4);
                long height = cursor.getLong(3);
                long size = cursor.getLong(11);
                String date_modified = cursor.getString(7);
                VideoItems videoItems = new VideoItems(id,
                        displayName,
                        duration,
                        date_modified,
                        width,
                        height, size, url);
                videoList.add(videoItems);
            }
        } catch (SQLiteException e) {
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
                cursor = null;
            }
        }
        return videoList;
    }

    /**
     * get all videos filmed by the camera (which is stored in file:DCIM)
     * @return ArrayList of the videos in the file:DCIM
     * */
    public static ArrayList<VideoItems> getFilmedVideos(
            ArrayList<VideoItems> videoList) {
        ArrayList<VideoItems> mVideoList = new ArrayList<VideoItems>();
        for (int i = 0; i < videoList.size(); i++) {
            Log.d(TAG, "videoList.get(i).getUrl()" + videoList.get(i).getUrl());
            if (videoList.get(i).getUrl().contains("DCIM")) {
                mVideoList.add(videoList.get(i));
            }
        };
        return mVideoList;
    }

    /**
     * get all the videos except the filmed videos
     * @return ArrayList of the videos except filmed videos
     * */
    public static ArrayList<VideoItems> getLocalVideos(ArrayList<VideoItems> videoList) {
        ArrayList<VideoItems> mVideoList = new ArrayList<VideoItems>();
        for (int i = 0; i < videoList.size(); i++) {
            Log.d(TAG, "videoList.get(i).getUrl()=" + videoList.get(i).getUrl());
            if (!videoList.get(i).getUrl().contains("DCIM")) {
                mVideoList.add(videoList.get(i));
            }
        }
        return mVideoList;
    }
    /**
     * get the OTG videos
     * @return ArrayList of the OTG videos
     * */
    public static ArrayList<VideoItems> getOtgVideos(ArrayList<VideoItems> videoList,String ogtdevicetitle) {
        if (ogtdevicetitle == null || videoList ==null )return null;
        ArrayList<VideoItems> mVideoList = new ArrayList<VideoItems>();
        for (int i = 0; i < videoList.size(); i++) {
            Log.d(TAG, "videoList.get(i).getUrl()=" + videoList.get(i).getUrl());
            if (videoList.get(i).getUrl().contains(ogtdevicetitle)) {
                mVideoList.add(videoList.get(i));
            }
        }
        return mVideoList;
    }
    /**
     * get the video playing records and divide them into groups by date
     * */
    public static void getRecentVideos(Context context, ArrayList<RecentVideoInfo> todayVideoList,
            ArrayList<RecentVideoInfo> yesterdayVideoList,
            ArrayList<RecentVideoInfo> furtherVideoList) {
        ContentResolver cr = context.getContentResolver();
        Cursor cursor = null;
        try {
            cursor = cr.query(
                    Uri.parse(DATABASE_TABLE),
                    null, null, null, Columns._ID);
            cursor.moveToLast();
            Date currentDate = new Date();
            long currentDayStart = startCurrentDate(currentDate, 0).getTime();
            for (int i = cursor.getCount() - 1; i >= 0; i--) {
                cursor.moveToPosition(i);
                int id = cursor.getInt(0);
                String title = cursor.getString(1);
                String bookmark = cursor.getString(2);
                String uri = cursor.getString(3);
                String duration = cursor.getString(4);
                String date = cursor.getString(5);
                RecentVideoInfo recentVideoInfo = new RecentVideoInfo(id, title, uri, bookmark,
                        duration, date);

                if (Date.parse(date) >= currentDayStart) {
                    todayVideoList.add(recentVideoInfo);
                } else if (currentDayStart - Date.parse(date) < 24 * 60 * 60 * 1000) {
                    yesterdayVideoList.add(recentVideoInfo);
                } else {
                    furtherVideoList.add(recentVideoInfo);
                }
            }
        } catch (SQLiteException e) {
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
                cursor = null;
            }
        }
    }

    /**
     * get all video playing records in the database created by ourselves
     * @return ArrayList of all video playing records
     */
    public static ArrayList<RecentVideoInfo> getRecentVideos(Context context) {
        ContentResolver cr = context.getContentResolver();
        Cursor cursor = null;
        ArrayList<RecentVideoInfo> videoList = new ArrayList<RecentVideoInfo>();
        try {
            cursor = cr.query(
                    Uri.parse(DATABASE_TABLE),
                    null, null, null, Columns._ID);
            cursor.moveToLast();
            for (int i = cursor.getCount() - 1; i >= 0; i--) {
                cursor.moveToPosition(i);
                int id = cursor.getInt(0);
                String title = cursor.getString(1);
                String bookmark = cursor.getString(2);
                String uri = cursor.getString(3);
                String duration = cursor.getString(4);
                String date = cursor.getString(5);
                RecentVideoInfo recentVideoInfo = new RecentVideoInfo(id, title, uri, bookmark,
                        duration, date);
                videoList.add(recentVideoInfo);
            }
        } catch (SQLiteException e) {
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
                cursor = null;
            }
        }
        return videoList;
    }

    /**
     * get the video id from its path
     * @return the real id of the video in com.providers.media
     */
    public static int getIdFromPath(String path, Context context) {
        ContentResolver cr = context.getContentResolver();
        Cursor cursor = null;
        int id = -1;
        try {
            cursor = cr.query(
                    MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
                    null, null, null, Columns._ID);
            for (int i = 0; i < cursor.getCount(); i++) {
                cursor.moveToNext();
                if (cursor.getString(1).equals(path)) {
                    id = cursor.getInt(0);
                }
            }
        } catch (SQLiteException e) {
            e.printStackTrace();
        } catch (Exception e) {
            // TODO: handle exception
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return id;
    }

    /**
     * get the time length of the videos
     * @return the length of the videos in special format:HH:mm:ss
     * */
    public static String calculatTime(int milliSecondTime) {
        int hour = milliSecondTime / (60 * 60 * 1000);
        int minute = (milliSecondTime - hour * 60 * 60 * 1000) / (60 * 1000);
        int seconds = (milliSecondTime - hour * 60 * 60 * 1000 - minute * 60 * 1000) / 1000;
        if (seconds >= 60) {
            seconds = seconds % 60;
            minute += seconds / 60;
        }
        if (minute >= 60) {
            minute = minute % 60;
            hour += minute / 60;
        }
        String sh = "";
        String sm = "";
        String ss = "";
        if (hour < 10) {
            sh = "0" + String.valueOf(hour);
        } else {
            sh = String.valueOf(hour);
        }
        if (minute < 10) {
            sm = "0" + String.valueOf(minute);
        } else {
            sm = String.valueOf(minute);
        }
        if (seconds < 10) {
            ss = "0" + String.valueOf(seconds);
        } else {
            ss = String.valueOf(seconds);
        }
        if (milliSecondTime < 60 * 60 * 1000) {
            return sm + ":" + ss;
        } else {
            return sh + ":" + sm + ":" + ss;
        }
    }

    /**
     * get the time start of the day
     * @param date
     * @flag 0  return yyyy-MM-dd 00:00:00
     *               1  return yyyy-MM-dd 23:59:59
     * @return
     */
    public static Date startCurrentDate(Date date, int flag) {
        Calendar cal = Calendar.getInstance();
        cal.setTime(date);
        int hour = cal.get(Calendar.HOUR_OF_DAY);
        int minute = cal.get(Calendar.MINUTE);
        int second = cal.get(Calendar.SECOND);
        long millisecond = hour*60*60*1000 + minute*60*1000 + second*1000;
        //00:00:00 am
        cal.setTimeInMillis(cal.getTimeInMillis()-millisecond);

        if (flag == 0) {
            return cal.getTime();
        } else if (flag == 1) {
            //23:59:59 pm
            cal.setTimeInMillis(cal.getTimeInMillis()+23*60*60*1000 + 59*60*1000 + 59*1000);
        }
        return cal.getTime();
    }

    /**
     * @param context and video size
     * @return string time
     */
    public static String getStringVideoSize(Context context, long size) {
        String stringSize = Formatter.formatFileSize(context, size);
        return stringSize;
    }

    public static void updateStatusBarColor(Activity activity, boolean actionMode) {
        if (actionMode) {
            int cabStatusBarColor = activity.getResources().getColor(
                    R.color.statusbar_color);
            activity.getWindow().setStatusBarColor(cabStatusBarColor);
        } else {
            int cabStatusBarColor = activity.getResources().getColor(
                    R.color.actionmode_color);
            activity.getWindow().setStatusBarColor(cabStatusBarColor);
        }
    }

    public static String getIdFromUri(Uri uri) {
        return uri.toString().substring(
                uri.toString().lastIndexOf("/") + 1,
                uri.toString().length());
    }

    /* SPRD:Add for bug598043 When playing videos from geak camera,the gallery3d will crash @{ */
    public static Uri getContentUri(int id) {
        Uri baseUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
        return baseUri.buildUpon().appendPath(String.valueOf(id)).build();
    }
    /* Bug598043 end @}*/

    /* SPRD:Add for bug597680 When click the sharemenu several times,there will be many share dialogs @{ */
    public synchronized static boolean isFastClick() {
        long time = System.currentTimeMillis();
        if ( time - mLastClickTime < 1000) {
            return true;
        }
        mLastClickTime = time;
        return false;
    }
    /* Bug597680 end@}　*/

    /* SPRD:Add for bug605100 judge if the video has been played @{ */
    public static boolean isUriExistedInDB(Uri uri,Context context) {
        ContentResolver cr = context.getContentResolver();
        Cursor cursor = null;
        boolean isExistedInDB = false;
        try {
            cursor = cr.query(
                    Uri.parse(DATABASE_TABLE),
                    null, null, null, Columns._ID);
            cursor.moveToLast();
            for (int i = cursor.getCount() - 1; i >= 0; i--) {
                cursor.moveToPosition(i);
                String myUri = cursor.getString(3);
                if (myUri.equals(uri.toString())) {
                    isExistedInDB = true;
                    break;
                }
            }
        } catch (SQLiteException e) {
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
                cursor = null;
            }
        }
        return isExistedInDB;
    }
    /* Bug605100 end @} */
}
