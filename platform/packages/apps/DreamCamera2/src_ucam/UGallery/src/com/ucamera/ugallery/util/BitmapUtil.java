/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery.util;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Matrix.ScaleToFit;
import android.graphics.Rect;
import android.graphics.RectF;
import android.media.ExifInterface;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.MediaStore;
import android.util.Log;
import android.util.TypedValue;

public class BitmapUtil {

    private static final String TAG = "BitmapUtils";
    private static final int DEFAULT_COMPRESS_QUALITY = 90;

    private static Bitmap createBitmap(Bitmap source, Matrix m) {
        return Bitmap.createBitmap(source, 0, 0, source.getWidth(), source.getHeight(), m, true);
    }

    private static Bitmap createBitmap(Bitmap source, int maxWidth, int maxHeight) {
    	Bitmap bitmap = source;
        if (bitmap != null) {
            // Scale down the sampled bitmap if it's still larger than the desired dimension.
            float scale = Math.min((float) maxWidth / bitmap.getWidth(),
                    (float) maxHeight / bitmap.getHeight());
            scale = Math.max(scale, Math.min((float) maxHeight / bitmap.getWidth(),
                    (float) maxWidth / bitmap.getHeight()));
            if (scale < 1) {
                Matrix m = new Matrix();
                m.setScale(scale, scale);
                Bitmap transformed = createBitmap(bitmap, m);
                bitmap.recycle();
                return transformed;
            }
        }
        return bitmap;
    }
    
    private static Rect getBitmapBounds(Uri uri, Context context) {
        Rect bounds = new Rect();
        InputStream is = null;

        try {
            is = context.getContentResolver().openInputStream(uri);
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inJustDecodeBounds = true;
            BitmapFactory.decodeStream(is, null, options);

            bounds.right = options.outWidth;
            bounds.bottom = options.outHeight;
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } finally {
        	Util.closeSilently(is);
        }

        return bounds;
    }
    
    private static Rect getBitmapBounds(byte[] jpeg) {
    	Rect bounds = new Rect();
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inJustDecodeBounds = true;
        BitmapFactory.decodeByteArray(jpeg, 0, jpeg.length, options);

        bounds.right = options.outWidth;
        bounds.bottom = options.outHeight;
    	
    	return bounds;
    }
    
    public static Rect getBitmapBounds(String path) {
    	Rect bounds = new Rect();
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(path, options);

        bounds.right = options.outWidth;
        bounds.bottom = options.outHeight;
    	
    	return bounds;
    }

    /**
     * Decodes bitmap that keeps aspect-ratio and spans most within the bounds.
     */
    private static Bitmap decodeBitmap(Uri uri, Context context, int width, int height) {
        InputStream is = null;
        Bitmap bitmap = null;

        try {
            // TODO: Take max pixels allowed into account for calculation to avoid possible OOM.
            Rect bounds = getBitmapBounds(uri, context);
            int sampleSize = Math.max(bounds.width() / width, bounds.height() / height);
            sampleSize = Math.min(sampleSize,
                    Math.max(bounds.width() / height, bounds.height() / width));

            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inSampleSize = Math.max(sampleSize, 1);
            options.inPreferredConfig = Bitmap.Config.ARGB_8888;

            is = context.getContentResolver().openInputStream(uri);
            bitmap = BitmapFactory.decodeStream(is, null, options);
        } catch (FileNotFoundException e) {
            Log.e(TAG, "FileNotFoundException: " + uri);
        } finally {
        	Util.closeSilently(is);
        }

        // Ensure bitmap in 8888 format, good for editing as well as GL compatible.
        if ((bitmap != null) && (bitmap.getConfig() != Bitmap.Config.ARGB_8888)) {
            Bitmap copy = bitmap.copy(Bitmap.Config.ARGB_8888, true);
            bitmap.recycle();
            bitmap = copy;
        }

        return createBitmap(bitmap, width, height);
    }

    private static Bitmap decodeBitmap(byte[] jepg, int width, int height) {
        Bitmap bitmap = null;

		Rect bounds = getBitmapBounds(jepg);
		
		int sampleSize = Math.max(bounds.width() / width, bounds.height()
				/ height);
		sampleSize = Math.min(sampleSize,
				Math.max(bounds.width() / height, bounds.height() / width));

		BitmapFactory.Options options = new BitmapFactory.Options();
		options.inSampleSize = Math.max(sampleSize, 1);
		options.inPreferredConfig = Bitmap.Config.ARGB_8888;

		bitmap = BitmapFactory.decodeByteArray(jepg, 0, jepg.length, options);


        // Ensure bitmap in 8888 format, good for editing as well as GL compatible.
        if ((bitmap != null) && (bitmap.getConfig() != Bitmap.Config.ARGB_8888)) {
            Bitmap copy = bitmap.copy(Bitmap.Config.ARGB_8888, true);
            bitmap.recycle();
            bitmap = copy;
        }

        return createBitmap(bitmap, width, height);
    }
    
    private static Bitmap decodeBitmap(String path, int width, int height) 
    {
        Bitmap bitmap = null;
		Rect bounds = getBitmapBounds(path);
		int sampleSize = Math.max(bounds.width() / width, bounds.height()
				/ height);
		sampleSize = Math.min(sampleSize,
				Math.max(bounds.width() / height, bounds.height() / width));

		BitmapFactory.Options options = new BitmapFactory.Options();
		options.inSampleSize = Math.max(sampleSize, 1);
		options.inPreferredConfig = Bitmap.Config.ARGB_8888;

		bitmap = BitmapFactory.decodeFile(path, options);

		bitmap = createBitmap(bitmap, width, height);
		
        // Ensure bitmap in 8888 format, good for editing as well as GL compatible.
        if ((bitmap != null) && (bitmap.getConfig() != Bitmap.Config.ARGB_8888)) {
            Bitmap copy = bitmap.copy(Bitmap.Config.ARGB_8888, true);
            bitmap.recycle();
            bitmap = copy;
        }
        return bitmap;
    }
    
    /**
     * Gets decoded bitmap (maybe immutable) that keeps orientation as well.
     */
    public static Bitmap getBitmap(Uri uri, Context context, int width, int height) {
        Bitmap bitmap = decodeBitmap(uri, context, width, height);

        // Rotate the decoded bitmap according to its orientation if it's necessary.
        if (bitmap != null) {
            int orientation = ExifUtil.getOrientation(uri, context);

            bitmap =  rotate(bitmap, orientation);
        }

        // Ensure bitmap in 8888 format, good for editing as well as GL compatible.
        if ((bitmap != null) && (bitmap.getConfig() != Bitmap.Config.ARGB_8888)) {
            Bitmap copy = bitmap.copy(Bitmap.Config.ARGB_8888, true);
            bitmap.recycle();
            bitmap = copy;
        }
        return bitmap;
    }
    
    public static Bitmap getBitmap(byte[] jpeg, int width, int height) {
        Bitmap bitmap = decodeBitmap(jpeg, width, height);

        // Rotate the decoded bitmap according to its orientation if it's necessary.
        if (bitmap != null) {
            int orientation = ExifUtil.getOrientation(jpeg);

            bitmap =  rotate(bitmap, orientation);
        }

        // Ensure bitmap in 8888 format, good for editing as well as GL compatible.
        if ((bitmap != null) && (bitmap.getConfig() != Bitmap.Config.ARGB_8888)) {
            Bitmap copy = bitmap.copy(Bitmap.Config.ARGB_8888, true);
            bitmap.recycle();
            bitmap = copy;
        }
        return bitmap;
    }
    
    public static Bitmap getBitmap(String path, int width, int height) {
        Bitmap bitmap = decodeBitmap(path, width, height);

        // Rotate the decoded bitmap according to its orientation if it's necessary.
        if (bitmap != null) {
            int orientation = ExifUtil.getOrientation(path);
            bitmap =  rotate(bitmap, orientation);
        }

        // Ensure bitmap in 8888 format, good for editing as well as GL compatible.
        if ((bitmap != null) && (bitmap.getConfig() != Bitmap.Config.ARGB_8888)) {
            Bitmap copy = bitmap.copy(Bitmap.Config.ARGB_8888, true);
            bitmap.recycle();
            bitmap = copy;
        }
        return bitmap;
    }
    
	public static RectF caculateFitinSize(int dispWidth, int dispHeight,
			int imageWidth, int imageHeight) {
		RectF rect = new RectF();
		if(dispWidth*imageHeight>imageWidth*dispHeight) {
			rect.top = 0;
			rect.bottom = dispHeight;
			int width = dispHeight*imageWidth/imageHeight;
			rect.left = (dispWidth-width)/2;
			rect.right = rect.left + width;
		} else {
			rect.left = 0;
			rect.right = dispWidth;
			int height = dispWidth*imageHeight/imageWidth;
			rect.top = (dispHeight-height)/2;
			rect.bottom = rect.top+height;
		}
		return rect;
	}
	
	public static Bitmap createFitinBitmap(Bitmap src, int maxWidth, int maxHeight) {
		int srcWidth = src.getWidth();
		int srcHeight = src.getHeight();
		int dstWidth = srcWidth;
		int dstHeight = srcHeight;
		if(maxWidth<srcWidth || maxHeight<srcHeight) {
			RectF srcRect = new RectF(0,0,srcWidth,srcHeight);
			RectF dstRect = new RectF(0,0,maxWidth,maxHeight);
			Matrix m = new Matrix();
			m.setRectToRect(srcRect, dstRect, ScaleToFit.CENTER);
			m.mapRect(dstRect,srcRect);
			dstWidth = (int)dstRect.width();
			dstHeight = (int)dstRect.height();
		}
		return Bitmap.createScaledBitmap(src, dstWidth, dstHeight, false);
	}
	
	
	public static Bitmap MirrorConvert(Bitmap srcBitmap, int degrees, boolean bFreesrcBitmap )
	{
		int w = srcBitmap.getWidth();
		// CID 109162 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
		// int h = srcBitmap.getHeight();

		Bitmap newb ;

		Matrix m = new Matrix();
		m.postRotate(degrees);
		m.postScale(-1, 1); //����ˮƽ��ת
		m.postTranslate(w, 0);

		newb 
			= Bitmap.createBitmap(srcBitmap, 0, 0, srcBitmap.getWidth(), srcBitmap.getHeight(), m, false );
	 
		if(bFreesrcBitmap == true && newb.equals(srcBitmap) == false)
			srcBitmap.recycle();
		return newb;
	}
	
	public static byte[] SaveBitmapToMemory(Bitmap srcBitmap, CompressFormat format)
	{
		byte[] data = null;
		
		try {
			ByteArrayOutputStream baos = new ByteArrayOutputStream();
			srcBitmap.compress(format, 100, baos);
			data = baos.toByteArray();
		}  
		  catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		return data; 
	}
	
	
	public static boolean saveBmp(Bitmap bitmap, String name) {
		File pf = new File(name);
		FileOutputStream stream;
		try {
			stream = new FileOutputStream(pf);
			bitmap.compress(Bitmap.CompressFormat.JPEG, 90, stream);
			stream.flush();
			stream.close();
			return true;
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		return false;

	}
	
    // Whether we should recycle the input (unless the output is the input).
    public static final boolean RECYCLE_INPUT = true;
    public static final boolean NO_RECYCLE_INPUT = false;

    public static Bitmap transform(Matrix scaler,
                                   Bitmap source,
                                   int targetWidth,
                                   int targetHeight,
                                   boolean scaleUp,
                                   boolean recycle) {
        int deltaX = source.getWidth() - targetWidth;
        int deltaY = source.getHeight() - targetHeight;
        if (!scaleUp && (deltaX < 0 || deltaY < 0)) {
            /*
             * In this case the bitmap is smaller, at least in one dimension,
             * than the target.  Transform it by placing as much of the image
             * as possible into the target and leaving the top/bottom or
             * left/right (or both) black.
             */
            Bitmap b2 = Bitmap.createBitmap(targetWidth, targetHeight,
                    Bitmap.Config.ARGB_8888);
            Canvas c = new Canvas(b2);

            int deltaXHalf = Math.max(0, deltaX / 2);
            int deltaYHalf = Math.max(0, deltaY / 2);
            Rect src = new Rect(
                    deltaXHalf,
                    deltaYHalf,
                    deltaXHalf + Math.min(targetWidth, source.getWidth()),
                    deltaYHalf + Math.min(targetHeight, source.getHeight()));
            int dstX = (targetWidth  - src.width())  / 2;
            int dstY = (targetHeight - src.height()) / 2;
            Rect dst = new Rect(
                    dstX,
                    dstY,
                    targetWidth - dstX,
                    targetHeight - dstY);
            c.drawBitmap(source, src, dst, null);
            if (recycle) {
                source.recycle();
            }
            return b2;
        }
        float bitmapWidthF = source.getWidth();
        float bitmapHeightF = source.getHeight();

        float bitmapAspect = bitmapWidthF / bitmapHeightF;
        float viewAspect   = (float) targetWidth / targetHeight;

        if (bitmapAspect > viewAspect) {
            float scale = targetHeight / bitmapHeightF;
            if (scale < .9F || scale > 1F) {
                scaler.setScale(scale, scale);
            } else {
                scaler = null;
            }
        } else {
            float scale = targetWidth / bitmapWidthF;
            if (scale < .9F || scale > 1F) {
                scaler.setScale(scale, scale);
            } else {
                scaler = null;
            }
        }

        Bitmap b1;
        if (scaler != null) {
            // this is used for minithumb and crop, so we want to filter here.
            b1 = Bitmap.createBitmap(source, 0, 0,
                    source.getWidth(), source.getHeight(), scaler, true);
        } else {
            b1 = source;
        }

        if (recycle && b1 != source) {
            source.recycle();
        }

        int dx1 = Math.max(0, b1.getWidth() - targetWidth);
        int dy1 = Math.max(0, b1.getHeight() - targetHeight);

        Bitmap b2 = Bitmap.createBitmap(
                b1,
                dx1 / 2,
                dy1 / 2,
                targetWidth,
                targetHeight);

        if (b2 != b1) {
            if (recycle || b1 != source) {
                b1.recycle();
            }
        }

        return b2;
    }
	
    // Rotates the bitmap by the specified degree.
    // If a new bitmap is created, the original bitmap is recycled.
    public static Bitmap rotate(Bitmap b, int degrees) {
        if (degrees != 0 && b != null) {
            Matrix m = new Matrix();
            m.setRotate(degrees,
                    (float) b.getWidth() / 2, (float) b.getHeight() / 2);
            try {
                Bitmap b2 = Bitmap.createBitmap(
                        b, 0, 0, b.getWidth(), b.getHeight(), m, true);
                if (b != b2) {
                    b.recycle();
                    b = b2;
                }
            } catch (OutOfMemoryError ex) {
                // We have no memory to rotate. Return the original bitmap.
            }
        }
        return b;
    }
    
    /**
     * Make a bitmap from a given Uri.
     *
     * @param uri
     */
    public static Bitmap makeBitmap(int minSideLength, int maxNumOfPixels,
            Uri uri, ContentResolver cr, boolean useNative) {
        ParcelFileDescriptor input = null;
        try {
            input = cr.openFileDescriptor(uri, "r");
            BitmapFactory.Options options = null;
            if (useNative) {
                options = createNativeAllocOptions();
            }
            return makeBitmap(minSideLength, maxNumOfPixels, uri, cr, input,
                    options);
        } catch (IOException ex) {
            return null;
        } finally {
            Util.closeSilently(input);
        }
    }

    public static Bitmap makeBitmap(int minSideLength, int maxNumOfPixels,
            ParcelFileDescriptor pfd, boolean useNative) {
        BitmapFactory.Options options = null;
        if (useNative) {
            options = createNativeAllocOptions();
        }
        return makeBitmap(minSideLength, maxNumOfPixels, null, null, pfd,
                options);
    }

    public static Bitmap makeBitmap(int minSideLength, int maxNumOfPixels,
            Uri uri, ContentResolver cr, ParcelFileDescriptor pfd,
            BitmapFactory.Options options) {
        try {
            if (pfd == null) pfd = makeInputStream(uri, cr);
            if (pfd == null) return null;
            if (options == null) options = new BitmapFactory.Options();

            FileDescriptor fd = pfd.getFileDescriptor();
            options.inJustDecodeBounds = true;
            BitmapManager.instance().decodeFileDescriptor(fd, options);
            if (options.mCancel || options.outWidth == -1
                    || options.outHeight == -1) {
                return null;
            }
            options.inSampleSize = computeSampleSize(
                    options, minSideLength, maxNumOfPixels);
            options.inJustDecodeBounds = false;

            options.inDither = false;
            options.inPreferredConfig = Bitmap.Config.ARGB_8888;
            return BitmapManager.instance().decodeFileDescriptor(fd, options);
        } catch (OutOfMemoryError ex) {
            Log.e(TAG, "Got oom exception ", ex);
            return null;
        } finally {
            Util.closeSilently(pfd);
        }
    }
    
    // Returns Options that set the puregeable flag for Bitmap decode.
    private static BitmapFactory.Options createNativeAllocOptions() {
        return new BitmapFactory.Options();
    }
    
    private static int computeSampleSize(BitmapFactory.Options options,
            int minSideLength, int maxNumOfPixels) {
        int initialSize = computeInitialSampleSize(options, minSideLength,
                maxNumOfPixels);

        int roundedSize;
        if (initialSize <= 8) {
            roundedSize = 1;
            while (roundedSize < initialSize) {
                roundedSize <<= 1;
            }
        } else {
            roundedSize = (initialSize + 7) / 8 * 8;
        }

        return roundedSize;
    }
    
    private static int computeInitialSampleSize(BitmapFactory.Options options,
            int minSideLength, int maxNumOfPixels) {
        double w = options.outWidth;
        double h = options.outHeight;

        int lowerBound = (maxNumOfPixels == -1) ? 1 :
                (int) Math.ceil(Math.sqrt(w * h / maxNumOfPixels));
        int upperBound = (minSideLength == -1) ? 128 :
                (int) Math.min(Math.floor(w / minSideLength),
                Math.floor(h / minSideLength));

        if (upperBound < lowerBound) {
            // return the larger one when there is no overlapping zone.
            return lowerBound;
        }

        if ((maxNumOfPixels == -1) &&
                (minSideLength == -1)) {
            return 1;
        } else if (minSideLength == -1) {
            return lowerBound;
        } else {
            return upperBound;
        }
    }
    
    private static ParcelFileDescriptor makeInputStream(
            Uri uri, ContentResolver cr) {
        try {
            return cr.openFileDescriptor(uri, "r");
        } catch (IOException ex) {
            return null;
        }
    }
	
	//If bFitIn is true, will return a smaller bitmap for fit in bitmap. 
	//Otherwise, will return a bigger bitmap for center crop. 
	private static Bitmap decodeBitmapFromFile(String fileName, int minWidth, int minHeight, boolean bFitIn){
		BitmapFactory.Options opts = new BitmapFactory.Options();
       opts.inJustDecodeBounds = true;	            
       BitmapFactory.decodeFile(fileName,opts);
       setSampleSize(opts, minWidth, minHeight, bFitIn);
		opts.inJustDecodeBounds = false;
		opts.inPreferredConfig = Bitmap.Config.RGB_565;
		Bitmap bmp = BitmapFactory.decodeFile(fileName,opts);
		return bmp;
	}
    
	private static Bitmap decodeBitmapFromByteArray(byte[] bytes, int minWidth, int minHeight, boolean bFitIn) {
		BitmapFactory.Options opts = new BitmapFactory.Options();
	       opts.inJustDecodeBounds = true;	            
	       BitmapFactory.decodeByteArray(bytes, 0, bytes.length,opts);
	       setSampleSize(opts, minWidth, minHeight, bFitIn);
			opts.inJustDecodeBounds = false;
			opts.inPreferredConfig = Bitmap.Config.RGB_565;
			Bitmap bmp = BitmapFactory.decodeByteArray(bytes, 0, bytes.length,opts);
			return bmp;
	}
	
	//If bFitIn is true, will return a sample size for fit in bitmap. (Bigger sample size, smaller bitmap)
	//Otherwise, will return a sample size for center crop. (Smaller sample size, bigger bitmap)
	private static void setSampleSize(BitmapFactory.Options opts,int width, int height, boolean bFitin) {
	      float x = (float) Math.max(opts.outWidth,opts.outHeight) / Math.max(width,height);
	        float y = (float) Math.min(opts.outWidth,opts.outHeight) / Math.min(width,height);
	        if(x <= 1 || y <= 1)
	        	opts.inSampleSize = 1;
	        else {
	        	if(bFitin) {
	        		opts.inSampleSize = x > y ? Math.round(x) : Math.round(y);
	        	} else {
	        		opts.inSampleSize = x < y ? Math.round(x) : Math.round(y);
	        	}
	        }
	}
	
	public static Bitmap getLatestImage(ContentResolver cr, String dir) {
		Cursor c = cr.query(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, 
				new String[]{MediaStore.Images.Media.DATA},
				MediaStore.Images.Media.BUCKET_ID+"=?", 
				new String[]{String.valueOf(dir.toLowerCase().hashCode())}, 
				MediaStore.Images.Media._ID+" DESC");
		if(c==null) {
			return null;
		}
		String path = null;
		if(c.moveToFirst()) {
			path = c.getString(0);
		}
		c.close();
		return getThumbnail(path, 120, 120, false);
	}
	
	/**
	 * Create a thumbnail bitmap for specified path.
	 * @param bFitIn If bFitIn is true, will return a smaller bitmap for fit in bitmap. 
				Otherwise, will return a bigger bitmap for center crop. 
	 * @return
	 */
	public static Bitmap getThumbnail(String path, int width, int height, boolean bFitIn) {
		Bitmap thumb = null;
		if(path!=null) {
			ExifInterface exif = null;
			byte[] byThumb = null;
			try {
				exif = new ExifInterface(path);
			} catch (IOException e) {
				e.printStackTrace();
			}
			if(exif!=null) byThumb = exif.getThumbnail();
			if(byThumb==null) {
				thumb = BitmapUtil.decodeBitmapFromFile(path, width, height, bFitIn);
			} else {
				thumb = BitmapUtil.decodeBitmapFromByteArray(byThumb, width, height, bFitIn);
			}
			int degrees = ExifUtil.getOrientation(exif);
			return BitmapUtil.rotate(thumb, degrees);
		}
		return null;
	}
	
	
	public static Bitmap decodeResourceUse565(Resources resources, int id) {
		TypedValue value = new TypedValue();
		resources.openRawResource(id, value);
		BitmapFactory.Options opts = new BitmapFactory.Options();
		opts.inTargetDensity = value.density;
		opts.inPreferredConfig = Bitmap.Config.RGB_565;
		return BitmapFactory.decodeResource(resources, id, opts);
	}
	
	
    public static native void nv21DownSample(byte[] in, int inWidth, int inHeight, byte[] out, int sampleSize, int rotation);
    
}
