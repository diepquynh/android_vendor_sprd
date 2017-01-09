package com.ucamera.uphoto.gpuprocess;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.text.TextUtils;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

/**
 * Copyright (C) 2013 Thundersoft Corporation
 * All rights Reserved
 */
public class GpuProcess {
//    private final String SHADER_PATH = "magiclens";
    /* SPRD: CID 109196 (#1 of 1): SS: Unread field should be static (FB.SS_SHOULD_BE_STATIC) @{
    private final String SHADER_PATH = "effects";
    */
    private static final String SHADER_PATH = "effects";
    /* @} */
    private String mCurrentType = "";
    private AssetManager mAssetManager = null;
    private EglWorker mWorker = new EglWorker();

    public GpuProcess(Context context) {
        mAssetManager = context.getAssets();
    }

    // type: effect id, like infared/sketch etc.
    public void setShaderType(String type) {
        if (TextUtils.equals(type, mCurrentType)) {
            return ;
        }

        mCurrentType = type;
        mWorker.setCurrentType(type, getShaderContent(mAssetManager, type));
    }

    public boolean process(Bitmap srcBitmap, Bitmap outBitmap, Bitmap[] texture) {
        if (srcBitmap == null || outBitmap == null) {
            return false;
        }

        int ret = mWorker.process(srcBitmap, outBitmap, texture);
        if (ret < 0) {
            return false;
        }

        return true;
    }

    // WARNING: this must be invoked before release the GpuProccess object
    // for this will release the opengl context.
    public void finish() {
        mWorker.finish();
    }

    private String getShaderContent(AssetManager assetManager, String fileName) {
        InputStream inputStream = null;
        /* SPRD: CID 109004 (#1 of 1): Resource leak (RESOURCE_LEAK) @{ */
        String bufferLine;
        StringBuffer tempText = new StringBuffer();
        /*
        try {
//            inputStream = assetManager.open(SHADER_PATH + "/" + type + ".fs");
            inputStream = assetManager.open(SHADER_PATH + "/" + fileName);
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        }

        */
        try{
        /* @} */
            inputStream = assetManager.open(SHADER_PATH + "/" + fileName);
        /* SPRD: CID 109118 (#1 of 1): Dm: Dubious method used (FB.DM_DEFAULT_ENCODING) @{
        BufferedReader bufferreader = new BufferedReader(new InputStreamReader(
                inputStream));
        */
            BufferedReader bufferreader = new BufferedReader(new InputStreamReader(inputStream, "utf-8"));

        /* @} */

        /* SPRD: CID 109273 (#1 of 1): SBSC: String concatenation in loop using + operator (FB.SBSC_USE_STRINGBUFFER_CONCATENATION) @{
        String bufferLine, tempText = "";
        */

        /*
        try {
            while ((bufferLine = bufferreader.readLine()) != null) {
                tempText = tempText + bufferLine;
            }
        } catch (IOException e) {
        }
        return tempText;
        */
            while ((bufferLine = bufferreader.readLine()) != null) {
                tempText.append(bufferLine);
            }
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        } finally {
            try {
                if (inputStream != null) {
                    inputStream.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        String result = tempText.toString();
        return result;
        /* @} */
    }

}
