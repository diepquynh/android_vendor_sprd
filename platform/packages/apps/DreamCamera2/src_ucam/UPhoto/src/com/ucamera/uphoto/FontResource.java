/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.Typeface;
import android.os.Environment;
import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.util.WeakHashMap;

public class FontResource {
    private static final String   TAG                = "FontResource";
    private static final String   DOWNLOAD_FILE_EXT  = ".ucam";
    private static final String   DOWNLOAD_FILE_CN_EXT = ".CN.ucam";
    private static final String   FONT_ASSETS_PATH   = "fonts";
    private static final String[] EMTPY_STRING_ARRAY = new String[0];
    private static final String   FONT_DOWNLOAD_PATH;
    static {
        final String sdcard = Environment.getExternalStorageDirectory().getAbsolutePath();
        FONT_DOWNLOAD_PATH = String.format("%s/UCam/download/%s/%s", sdcard,
                Const.EXTRA_FONT_VALUE, Const.EXTRA_FONT_VALUE);
    };

    public static String[] listFonts(Context context, String language) {
        String[] assetFonts = listAssetFonts(context);
        String[] downloadFonts = listDownloadFonts(context, language);
        String[] result = new String[assetFonts.length + downloadFonts.length];
        int coped = 0;
        if(language.equals("zh")) {
            System.arraycopy(downloadFonts, 0, result, coped, downloadFonts.length);
            coped += downloadFonts.length;
            System.arraycopy(assetFonts, 0, result, coped, assetFonts.length);
        } else {
            System.arraycopy(assetFonts, 0, result, coped, assetFonts.length);
            coped += assetFonts.length;
            System.arraycopy(downloadFonts, 0, result, coped, downloadFonts.length);
        }

        return result;
    }

    public static String[] listAssetFonts(Context context) {
        try {
            String[] assetFonts = context.getAssets().list(FONT_ASSETS_PATH);
            if (assetFonts != null) {
                return assetFonts;
            }
        } catch (IOException e) {
            Log.e(TAG, "Fail load asset fonts", e);
        }
        return EMTPY_STRING_ARRAY;
    }

    public static String[] listDownloadFonts(Context context, String language) {
        String[] downloadENFonts = new File(FONT_DOWNLOAD_PATH).list(new FileFilter(
                DOWNLOAD_FILE_EXT));
        String[] downloadCNFonts = new File(FONT_DOWNLOAD_PATH).list(new FileAccept(
                DOWNLOAD_FILE_CN_EXT));
        if(downloadENFonts == null && downloadCNFonts == null) {
            return EMTPY_STRING_ARRAY;
        }

        /**
         * FIX BUG: 829
         * BUG CAUSE: new demand
         * FIX COMMENT: Distinguish between Chinese and English font.
         * DATE: 2012-04-20
         */
        /* SPRD: CID 108940 : Dereference after null check (FORWARD_NULL) @{ */
        int length = 0;
        if(downloadENFonts != null)
            length += downloadENFonts.length;
        if(downloadCNFonts != null)
            length += downloadCNFonts.length;
        String[] result = new String[length];
        // String[] result = new String[downloadENFonts.length + downloadCNFonts.length];
        /* @} */
        int coped = 0;
        if(language.equals("zh")) {
            System.arraycopy(downloadCNFonts, 0, result, coped, downloadCNFonts.length);
            coped += downloadCNFonts.length;
            System.arraycopy(downloadENFonts, 0, result, coped, downloadENFonts.length);
        } else {
            System.arraycopy(downloadENFonts, 0, result, coped, downloadENFonts.length);
            coped += downloadENFonts.length;
            System.arraycopy(downloadCNFonts, 0, result, coped, downloadCNFonts.length);
        }

        return result;
    }

    public static Typeface createTypeface(Context context, String filename) {
        if (filename == null)
            return Typeface.DEFAULT;

        Typeface typeface = sIntalledTypefaces.get(filename);
        if (typeface != null) {
            return typeface;
        }

        try {
            typeface= (isDownloadResource(filename))?
                  Typeface.createFromFile(FONT_DOWNLOAD_PATH + "/" + filename)
                : Typeface.createFromAsset(context.getAssets(), FONT_ASSETS_PATH + "/" + filename);
        }catch (Exception e) {
            Log.e(TAG,"Fail create type face by: " + filename,e);
        }

        if (typeface != null) {
            sIntalledTypefaces.put(filename, typeface);
        }

        return typeface;
    }

    private static boolean isDownloadResource(String filename) {
        return filename != null && filename.endsWith(DOWNLOAD_FILE_EXT);
    }


    private static WeakHashMap<String, Typeface> sIntalledTypefaces
            = new WeakHashMap<String, Typeface>();
}
