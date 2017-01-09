/**
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ugallery.panorama;

import java.io.InputStream;
import java.util.ArrayList;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.util.Log;

public class PanoramaTexture {

    public static final String TAG = "PanoramaTexture";

    public static final int WIDTH_MAX = 1024;

    private ArrayList<Texture> textureList = new ArrayList<Texture>();

    public PanoramaTexture(Context context, Bitmap bitmap, float angle) throws OutOfMemoryError{
//        Bitmap bitmap = loadBitmap(context);
        if (bitmap == null) {
            return ;
        }

        if (bitmap != null) {
            if (bitmap.getWidth() >= WIDTH_MAX) {
                addTextures(bitmap);
            } else {
                addTexture(bitmap);
            }
        }
        if (angle < 0 || angle > 360) {
            angle = 360;
        }

        computerTextureAngle(bitmap.getWidth(),angle);
    }

//    private Bitmap loadBitmap(Context context) {
//        BitmapFactory.Options opt = new BitmapFactory.Options();
//        opt.inPreferredConfig = Bitmap.Config.RGB_565;
//        opt.inPurgeable = true;
//        opt.inInputShareable = true;
//        InputStream is = context.getResources().openRawResource(R.drawable.test6);
//        Bitmap bitmap = BitmapFactory.decodeStream(is, null, opt);
//        return bitmap;
//    }

    private void addTextures(Bitmap bitmap) throws OutOfMemoryError{
        int width = bitmap.getWidth();
        int height = bitmap.getHeight();
        int count = width / WIDTH_MAX;
        int lastWidth = width % WIDTH_MAX;
        try {
            for (int i = 0; i < count; i++) {
                Bitmap b = Bitmap.createBitmap(bitmap, i * WIDTH_MAX, 0, WIDTH_MAX, height);
                addTexture(b);
                //recycle(b);
            }
            if(lastWidth > 0){
                Bitmap last = Bitmap.createBitmap(bitmap, width - lastWidth, 0, lastWidth, height);
                addTexture(last);
                //recycle(last);
            }
        }
        catch (OutOfMemoryError e) {
            throw e;
        }
        finally {
            recycle(bitmap);
        }
    }

    private void addTexture(Bitmap bitmap) throws OutOfMemoryError{
        try {
            int width = bitmap.getWidth();
            int height = bitmap.getHeight();
            float normalizedWidth = 1.0f;
            float normalizedHeight = 1.0f;
            if (!isPowerOf2(width) || !isPowerOf2(height)) {
                int paddedWidth = nextPowerOf2(width);
                int paddedHeight = nextPowerOf2(height);
                Bitmap.Config config = bitmap.getConfig();
                if (config == null) config = Bitmap.Config.RGB_565;
                if (width * height >= 512 * 512) config = Bitmap.Config.RGB_565;
                Bitmap padded = Bitmap.createBitmap(paddedWidth, paddedHeight, config);
                Canvas canvas = new Canvas(padded);
                canvas.drawBitmap(bitmap, 0, 0, null);
                bitmap.recycle();
                bitmap = padded;
                normalizedWidth = (float) width / (float) paddedWidth;
                normalizedHeight = (float) height / (float) paddedHeight;
            }
            Texture texture = new Texture();
            texture.setBitmap(bitmap);
            texture.setWidth(width);
            texture.setHeight(height);
            texture.setNormalizedWidth(normalizedWidth);
            texture.setNormalizedHeight(normalizedHeight);
            textureList.add(texture);
        } catch (Exception e) {
            Log.i(TAG, "Throw Exceptin");
        } catch (OutOfMemoryError eMem) {
            Log.i(TAG, "Handling low memory condition");
            throw eMem;
        }
    }

    private boolean isPowerOf2(int n) {
        return (n & -n) == n;
    }

    private int nextPowerOf2(int n) {
        n -= 1;
        n |= n >>> 16;
        n |= n >>> 8;
        n |= n >>> 4;
        n |= n >>> 2;
        n |= n >>> 1;
        return n + 1;
    }

    public ArrayList<Texture> getTextureList() {
        return textureList;
    }

    public Texture getTexture(int i){
        return textureList.get(i);
    }

    private void computerTextureAngle(int width, float angle) {
        int n = width / WIDTH_MAX;
        int d = width % WIDTH_MAX;
        float offset = 0;
        for (int i = 0; i < n; i++) {
            Texture t = textureList.get(i);
            t.setAngle(WIDTH_MAX / (float)width * angle);
            offset -= t.getAngle();
            t.setOffsetAngle(offset);
        }
        if (d > 0) {
            Texture t = textureList.get(n);
            t.setAngle(d / (float)width * angle);
            offset -= t.getAngle();
            t.setOffsetAngle(offset);
        }
    }

    public void destroy() {
        if (textureList != null) {
            for (int i = 0; i < textureList.size(); i++) {
                Texture texture = textureList.get(i);
                Bitmap b = texture.getBitmap();
                if (b != null && !b.isRecycled()) {
                    recycle(b);
                }
            }
            textureList.clear();
        }
    }

    private void recycle(Bitmap b) {
        if (b != null && !b.isRecycled()) {
            b.recycle();
        }
        b = null;
    }

}
