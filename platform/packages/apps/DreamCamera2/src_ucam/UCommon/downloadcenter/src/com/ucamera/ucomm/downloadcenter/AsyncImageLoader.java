/*
 * Copyright (C) 2010,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.downloadcenter;

import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.ref.SoftReference;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.HashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * asyn load image
 */
public class AsyncImageLoader {
    //
    private HashMap<String, SoftReference<Drawable>> imageCache;

    private ExecutorService mService;
    /**
     * construction method
     */
    public AsyncImageLoader() {
        imageCache = new HashMap<String, SoftReference<Drawable>>();
    }

    public void shutdown() {
        synchronized (this) {
            if (mService != null) {
                mService.shutdown();
                mService = null;
            }
            imageCache.clear();
        }
    }

    /**
     * load drawable to display in the gridview
     * @param imageUrl image url contains http/www and /mnt/sdcard/ etc
     * @param imageCallback call back interface
     * @return the drawable that provided to diaplsy item
     */
    public Drawable loadDrawable(final String imageUrl, final ImageCallback imageCallback) {
        /* FIX BUG: 1440
         * BUG CAUSE: supported null pointer
         * Date: 2012-08-13
         */
        if(imageCache == null){
            imageCache = new HashMap<String, SoftReference<Drawable>>();
        }
        if (imageCache.containsKey(imageUrl)) {
            SoftReference<Drawable> softReference = imageCache.get(imageUrl);
            Drawable drawable = null;
            if(softReference != null){
                drawable = softReference.get();
            }
            if (drawable != null) {
                return drawable;
            }
        }

        final Handler handler = new Handler() {
            public void handleMessage(Message message) {
                imageCallback.imageLoaded((Drawable) message.obj, imageUrl);
            }
        };

        if (mService == null) {
            synchronized (this) {
                mService = Executors.newFixedThreadPool(3);
            }
        }

        mService.execute(new Runnable() {
            public void run() {
                Drawable drawable;
                try {
                    drawable = loadImageFromUrl(imageUrl);
                    imageCache.put(imageUrl, new SoftReference<Drawable>(drawable));
                    Message message = handler.obtainMessage(0, drawable);
                    handler.sendMessage(message);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        });
        return null;
    }

    /**
     * load drawable to display in the gridview
     * @param path image path
     * @return the drawable that provided to diaplsy item
     * @throws IOException
     */
    public static synchronized Drawable loadImageFromUrl(String path) throws Exception {
        Drawable d = null;
        if(path != null) {
            if (path.startsWith("http://") || path.startsWith("www.")) {
                String cachedPath = generateCachedPath(path);
                if (new File(cachedPath).exists()) {
                    d = Drawable.createFromPath(cachedPath);
                } else {
                    d = loadAndSaveFromRemote(path, cachedPath);
                }
            } else {
                d = Drawable.createFromPath(path);
            }
        }
        return d;
    }

    private static Drawable loadAndSaveFromRemote(String path, String cachedPath)
            throws Exception {
        Drawable d;
        HttpURLConnection conn = null;
        InputStream is = null;
        OutputStream os = null;
        try {
            if (cachedPath == null || cachedPath.trim().length() == 0) {
                cachedPath = generateCachedPath(path);
            }
            /*
             * FIX BUG: 1225
             * BUG CAUSE: No judgment of thumbnail direction is exist or not
             * FIX COMMENT: add judgment,create thumbnail direction if not exist
             * DATE: 2012-07-06
             */
            if(!new File(Constants.THUMBNAILS_DIR).exists()){
                /* SPRD: CID 109237 : RV: Bad use of return value (FB.RV_RETURN_VALUE_IGNORED_BAD_PRACTICE) @{ */
                if(!(new File(Constants.THUMBNAILS_DIR).mkdirs())){
                    throw new Exception();
                }
                // new File(Constants.THUMBNAILS_DIR).mkdirs();
                /* @} */
            }
            URL url = new URL(path);
            conn= (HttpURLConnection) url.openConnection();
            conn.setConnectTimeout(5 * 1000);
            conn.setRequestMethod("GET");
            conn.setUseCaches(false);
            if(conn.getResponseCode() == HttpURLConnection.HTTP_OK){
                is = conn.getInputStream();
                os = new FileOutputStream(cachedPath);

                byte[] buf = new byte[1024];
                int r = -1;
                while ((r = is.read(buf)) != -1) {
                    os.write(buf,0,r);
                }
                os.flush();
            }
            d = Drawable.createFromPath(cachedPath);
        }finally {
            DownLoadUtil.closeSilently(os);
            DownLoadUtil.closeSilently(is);
            try {
                if (conn != null ) {
                    conn.disconnect();
                }
            } catch (Exception e) {
                // IGNORE THIS
            }
        }
        return d;
    }

    private static String generateCachedPath(String url) {
        String filename;
        if (url == null || url.trim().length() == 0) {
            filename = Constants.THUMBNAILS_DIR + "c.u";
        } else {
            filename = "c"+url.hashCode() + ".u";
        }
        return Constants.THUMBNAILS_DIR + filename;
    }
    public interface ImageCallback {
        public void imageLoaded(Drawable imageDrawable, String imageUrl);
    }
}
