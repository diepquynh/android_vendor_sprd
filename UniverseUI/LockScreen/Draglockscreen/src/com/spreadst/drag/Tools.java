/** Create by Spreadst */

package com.spreadst.drag;

import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.Formatter;
import java.util.Locale;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.util.Log;

public class Tools {

    protected static final String PACKAGE_NAME = "com.spreadst.drag";

    private static Paint mPaint = new Paint();

    private static Bitmap mbackgroundBitmap;

    private static Bitmap mDefaultBackgroundBitmap;

    private static Bitmap mNeedRecycleBitmapOfInTime;

    public static int getLayoutId(Resources res, String id_name) {
        if (res == null) {
            return 0;
        }
        return res.getIdentifier(PACKAGE_NAME + ":layout/" + id_name, null,
                null);
    }

    public static int getInnerId(Resources res, String id_name) {
        if (res == null) {
            return 0;
        }
        return res.getIdentifier(PACKAGE_NAME + ":id/" + id_name, null, null);
    }

    public static int getDrawableId(Resources res, String id_name) {
        if (res == null) {
            return 0;
        }
        return res.getIdentifier(PACKAGE_NAME + ":drawable/" + id_name, null,
                null);
    }

    public static int getDimenId(Resources res, String id_name) {
        if (res == null) {
            return 0;
        }
        return res
                .getIdentifier(PACKAGE_NAME + ":dimen/" + id_name, null, null);
    }

    public static int getStringId(Resources res, String id_name) {

        if (res == null) {
            return 0;
        }
        return res.getIdentifier(PACKAGE_NAME + ":string/" + id_name, null,
                null);

    }

    /* Modify 20130813 Spreadst of paly music do not show hour start */
    private static StringBuilder sFormatBuilder = new StringBuilder();
    private static Formatter sFormatter = new Formatter(sFormatBuilder, Locale.getDefault());
    private static final Object[] sTimeArgs = new Object[5];

    public static String getTimeString(Resources mRes, long secs) {
        String durationformat = mRes.getString(
                secs < 3600 ? Tools.getStringId(mRes, "durationformatshort") : Tools.getStringId(
                        mRes, "durationformatlong"));
        sFormatBuilder.setLength(0);

        final Object[] timeArgs = sTimeArgs;
        timeArgs[0] = secs / 3600;
        timeArgs[1] = secs / 60;
        timeArgs[2] = (secs / 60) % 60;
        timeArgs[3] = secs;
        timeArgs[4] = secs % 60;
        String hms = sFormatter.format(durationformat, timeArgs).toString();
        if (hms == null) {
            return "00:00";
        }
        return hms;
    }

    public static String makeTimeString4MillSec(Resources mRes, long millSecs) {
        int secs = Math.round((float) millSecs / 1000);
        return getTimeString(mRes, secs == 0 ? 1 : secs);
    }

    /* Modify 20130813 Spreadst of paly music do not show hour end */

    public static Bitmap getArtwork(Context context, long song_id,
            long album_id, boolean allowdefault) {

        if (album_id < 0) {
            // This is something that is not in the database, so get the album
            // art directly
            // from the file.
            if (song_id >= 0) {
                Bitmap bm = getArtworkFromFile(context, song_id, -1);
                if (bm != null) {
                    return bm;
                }
            }
            if (allowdefault) {
                return getDefaultArtwork(context);
            }
            return getDefaultArtwork(context);
        }

        ContentResolver res = context.getContentResolver();
        Uri uri = ContentUris.withAppendedId(sArtworkUri, album_id);
        if (uri != null) {
            InputStream in = null;
            try {
                in = res.openInputStream(uri);
                return BitmapFactory.decodeStream(in, null, sBitmapOptions);
            } catch (FileNotFoundException ex) {
                // The album art thumbnail does not actually exist. Maybe the
                // user deleted it, or
                // maybe it never existed to begin with.
                Bitmap bm = getArtworkFromFile(context, song_id, album_id);
                if (bm != null) {
                    if (bm.getConfig() == null) {
                        bm = bm.copy(Bitmap.Config.RGB_565, false);
                        if (bm == null && allowdefault) {
                            return getDefaultArtwork(context);
                        }
                    }
                } else if (allowdefault) {
                    bm = getDefaultArtwork(context);
                }
                return bm;
            } finally {
                try {
                    if (in != null) {
                        in.close();
                    }
                } catch (IOException ex) {
                }
            }
        }

        return getDefaultArtwork(context);
    }

    private static final Uri sArtworkUri = Uri
            .parse("content://media/external/audio/albumart");
    private static Bitmap mCachedBit = null;
    private static final BitmapFactory.Options sBitmapOptions = new BitmapFactory.Options();
    static {
        sBitmapOptions.inPreferredConfig = Bitmap.Config.RGB_565;
        sBitmapOptions.inDither = false;
    }

    private static Bitmap getArtworkFromFile(Context context, long songid,
            long albumid) {
        Bitmap bm = null;
        byte[] art = null;
        String path = null;
        ParcelFileDescriptor pfd = null;

        if (albumid < 0 && songid < 0) {
            throw new IllegalArgumentException(
                    "Must specify an album or a song id");
        }

        try {
            if (albumid < 0) {
                Uri uri = Uri.parse("content://media/external/audio/media/"
                        + songid + "/albumart");
                pfd = context.getContentResolver().openFileDescriptor(uri, "r");
                if (pfd != null) {
                    FileDescriptor fd = pfd.getFileDescriptor();
                    bm = BitmapFactory.decodeFileDescriptor(fd);
                }
            } else {
                Uri uri = ContentUris.withAppendedId(sArtworkUri, albumid);
                pfd = context.getContentResolver().openFileDescriptor(uri, "r");
                if (pfd != null) {
                    FileDescriptor fd = pfd.getFileDescriptor();
                    bm = BitmapFactory.decodeFileDescriptor(fd);
                }
            }
        } catch (IllegalStateException ex) {
        } catch (FileNotFoundException ex) {
        } finally {
            try {
                if (pfd != null)
                    pfd.close();
            } catch (IOException e) {
            }
        }
        if (bm != null) {
            mCachedBit = bm;
        }
        return bm;
    }

    public static Bitmap getDefaultArtwork(Context context) {
        Log.d(PACKAGE_NAME, "getDefaultArtwork....");
        if (mDefaultBackgroundBitmap == null) {
            Resources res = null;
            try {
                res = context.getPackageManager().getResourcesForApplication(
                        PACKAGE_NAME);
            } catch (NameNotFoundException e) {
                e.printStackTrace();
            }
            if (res != null) {
                mDefaultBackgroundBitmap = BitmapFactory.decodeResource(res,
                        getDrawableId(res, "music_default_sprd"));
            }
        }
        Log.d(PACKAGE_NAME, "mDefaultBackgroundBitmap:  "
                + mDefaultBackgroundBitmap);
        return mDefaultBackgroundBitmap;
    }

    public static Bitmap getBackgroundBitmap(Bitmap srcBitmap, int backWidth,
            int backHeight) {
        mPaint.setAntiAlias(true);
        mPaint.setColor(Color.WHITE);
        mNeedRecycleBitmapOfInTime = mbackgroundBitmap;
        mbackgroundBitmap = Bitmap.createBitmap(backWidth, backHeight,
                Bitmap.Config.ARGB_8888);
        Canvas cv = new Canvas(mbackgroundBitmap);
        Bitmap squareBitmap = getSquareBitmap(srcBitmap, backWidth);
        if (srcBitmap != mDefaultBackgroundBitmap) {
            srcBitmap.recycle();
        }
        cv.drawBitmap(squareBitmap, 0, 0, mPaint);
        Bitmap reversalBitmap = getReversalBitmap(squareBitmap);
        cv.drawBitmap(reversalBitmap, 0, squareBitmap.getHeight(), mPaint);
        squareBitmap.recycle();
        reversalBitmap.recycle();
        return mbackgroundBitmap;
    }

    private static Bitmap getSquareBitmap(Bitmap srcBitmap, int side) {
        int width = srcBitmap.getWidth();
        int height = srcBitmap.getHeight();
        Matrix matrix = new Matrix();
        float sw = ((float) side) / width;
        float sh = ((float) side) / height;
        matrix.postScale(sw, sh);
        Bitmap squareBitmap = Bitmap.createBitmap(srcBitmap, 0, 0, width,
                height, matrix, true);
        return squareBitmap;
    }

    private static Bitmap getReversalBitmap(Bitmap squareBitmap) {
        Matrix matrix = new Matrix();
        matrix.preScale(-1, 1);
        matrix.postRotate(180);
        Bitmap ReversalBitmap = Bitmap
                .createBitmap(squareBitmap, 0, 0, squareBitmap.getWidth(),
                        squareBitmap.getHeight(), matrix, true);
        return ReversalBitmap;
    }

    public static void recycleLastBackgroundBitmap() {
        Log.d(PACKAGE_NAME,
                "recycle mNeedRecycleBitmapOfInTime: " + mNeedRecycleBitmapOfInTime != null ? "recycle!"
                        : "null");
        if (mNeedRecycleBitmapOfInTime != null) {
            mNeedRecycleBitmapOfInTime.recycle();
            mNeedRecycleBitmapOfInTime = null;
        }
    }

    public static void recycleAllBitmap() {
        Log.d(PACKAGE_NAME,
                "recycle recycleAllBitmap");
        recycleLastBackgroundBitmap();
        if (mbackgroundBitmap != null) {
            mbackgroundBitmap.recycle();
            mbackgroundBitmap = null;
        }
        if (mDefaultBackgroundBitmap != null) {
            mDefaultBackgroundBitmap.recycle();
            mDefaultBackgroundBitmap = null;
        }
    }
}
