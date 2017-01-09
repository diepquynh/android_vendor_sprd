package com.sprd.ucam.vgesutre;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.NinePatch;
import android.graphics.Rect;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.view.View;

import com.android.camera2.R;
import com.sprd.hz.selfportrait.MSG;
import com.sprd.hz.selfportrait.detect.DetectEngine;
import com.sprd.hz.selfportrait.detect.OnDetectListener;

public class DetectView extends View
    implements OnDetectListener{
    private DetectEngine mEngine = null;
    private Handler mHandler = null;
    private Rect[] mRectFaces = null;
    private Bitmap mDrawFace = null;
    private NinePatch mNpFace = null;

    public DetectView(Context context, AttributeSet attrs) {
        super(context,attrs);
        initialize();
    }
    public void setEngine(DetectEngine e) {
        mEngine = e;
        mEngine.addOnDetectListener(this);
    }
    public void setHandler(Handler h) {
        mHandler = h;
    }
    private void initialize()
    {
        mDrawFace = BitmapFactory.decodeResource(getResources(),
                R.drawable.camera_face_rect_normal);
        mNpFace = new NinePatch(mDrawFace, mDrawFace.getNinePatchChunk(),"");
    }
    public void setFaceRectResource(int resid) {
        mDrawFace = BitmapFactory.decodeResource(getResources(), resid);
        mNpFace = new NinePatch(mDrawFace, mDrawFace.getNinePatchChunk(),"");
    }
    public void setFaces(Rect[] faces) {
        mRectFaces = faces;
        postInvalidate();
    }
    public void setGestures(Rect[] gestures) {
//        postInvalidate();
    }
    @Override
    protected void onDraw(Canvas c) {
        super.onDraw(c);
        if(mRectFaces!=null) {
            for(Rect rect: mRectFaces) {
                mNpFace.draw(c, rect);
            }
        }
    }

    public void onDetectFace(Rect[] faces) {
        setFaces(faces);
    }
    private static final int CAPTURE_THRESHOLD = 1;
    private int mDetectLast = 0;
    @Override
    public void onDetectGesture(Rect[] gestures) {
        setGestures(gestures);
        if(gestures!=null && gestures.length>0) {
            mDetectLast ++;
            if(mDetectLast>=CAPTURE_THRESHOLD) {
                Message.obtain(mHandler, MSG.CAMERA_SHUTTER, DetectEngine.DETECT_TYPE_GESTURE, 0)
                    .sendToTarget();
                mDetectLast = 0;
            }
        } else {
            mDetectLast = 0;
        }
    }

    /* SPRD: Add for bug 569343 (487754 in 5.1) @{ */
    public void clear() {
        mRectFaces = null;
        invalidate();
    }
    /* @} */
}
