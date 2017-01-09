/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.Point;
import android.graphics.Paint.Style;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Toast;
import com.ucamera.uphoto.R;
public class MakeupFaceView extends View {

    private static final String TAG = "MakeupFaceView";
    // CID 109207 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private Path mPath;
    private Rect mPoint1Rect, mPoint2Rect, mPoint3Rect;
    private Paint mPaint;
    private Point mDownPoint,mMovePoint, mUpPoint, mPoint1, mPoint2, mPoint3;
    public int mDownState;
    public int mMoveState;
//    private DrawTriangleCallBack mDrawTriangleCallBack;
/*    public interface DrawTriangleCallBack {
        public void changeButtonStatus();
    }*/

    public MakeupFaceView(Context context) {
        super(context);
        mPaint = new Paint(Paint.DITHER_FLAG);

        mPaint.setStyle(Style.STROKE);
        mPaint.setStrokeWidth(1);
        mPaint.setColor(Color.parseColor("#55ffffff"));
        mPaint.setAntiAlias(true);

        mDownPoint = new Point();
        mMovePoint = new Point();
        mUpPoint = new Point();
        mPoint1 = null;
        mPoint2 = null;
        mPoint3 = null;
        mPoint1Rect = new Rect();
        mPoint2Rect = new Rect();
        mPoint3Rect = new Rect();
        // CID 109207 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mPath = new Path();
        mEyesLeft = context.getResources().getDrawable(R.drawable.eyes_left);
        mEyesRight = context.getResources().getDrawable(R.drawable.eyes_right);
        mMouth = context.getResources().getDrawable(R.drawable.mouth);
    }


/*    public void setDrawTriangleCallBack(DrawTriangleCallBack drawTriangleCallBack) {
        mDrawTriangleCallBack = drawTriangleCallBack;
    }
*/
    public void setFacePosition(Point lefteye, Point righteye, Point mouth) {
        if (lefteye != null ) {
            mPoint1 = lefteye;
            mPoint1Rect = new Rect(mPoint1.x - 50, mPoint1.y - 50,
                    mPoint1.x + 50, mPoint1.y + 50);
        }
        if (righteye != null) {
            mPoint2 = righteye;
            mPoint2Rect = new Rect(mPoint2.x - 50, mPoint2.y - 50,
                    mPoint2.x + 50, mPoint2.y + 50);
        }
        if (mouth != null) {
            mPoint3 = mouth;
            mPoint3Rect = new Rect(mPoint3.x - 50, mPoint3.y - 50,
                    mPoint3.x + 50, mPoint3.y + 50);
        }
    }

    public void setFaceBound(int offsetX, int offsetY, int bitmapWidth, int bitmapHeight) {
       mLeftBound = offsetX;
       mTopBound = offsetY;
       mRightBound = offsetX + bitmapWidth;
       mButtomBound = offsetY + bitmapHeight;
    }
    @Override
    protected void onDraw(Canvas canvas) {
        /*
         * FIX BUG:5205
         * BUG COMMENT: Avoid null pointer exception
         * DATE: 2013-11-11
         */
        if (mPoint1Rect != null && mPoint1 != null && mEyesLeft != null) {
            canvas.drawPoint(mPoint1.x, mPoint1.y, mPaint);
            mEyesLeft.setBounds(mPoint1.x - mEyesLeft.getIntrinsicWidth()/2, mPoint1.y - mEyesLeft.getIntrinsicHeight()/2, mPoint1.x + mEyesLeft.getIntrinsicWidth()/2, mPoint1.y + mEyesLeft.getIntrinsicHeight()/2);
            mEyesLeft.draw(canvas);
        }
        if (mPoint2Rect != null && mPoint2 != null && mPoint1 != null && mEyesRight != null) {
            canvas.drawPoint(mPoint2.x, mPoint2.y, mPaint);
            canvas.drawLine(mPoint1.x, mPoint1.y, mPoint2.x, mPoint2.y, mPaint);
            mEyesRight.setBounds(mPoint2.x - mEyesRight.getIntrinsicWidth()/2, mPoint2.y - mEyesLeft.getIntrinsicHeight()/2, mPoint2.x + mEyesRight.getIntrinsicWidth()/2, mPoint2.y + mEyesRight.getIntrinsicHeight()/2);
            mEyesRight.draw(canvas);
        }
        if (mPoint3Rect != null && mPoint1 != null && mPoint2 != null && mPoint3 != null && mMouth != null) {
            canvas.drawPoint(mPoint3.x, mPoint3.y, mPaint);
            canvas.drawLine(mPoint2.x, mPoint2.y, mPoint3.x, mPoint3.y, mPaint);
            canvas.drawLine(mPoint3.x, mPoint3.y, mPoint1.x, mPoint1.y, mPaint);
            mMouth.setBounds(mPoint3.x - mMouth.getIntrinsicWidth()/2, mPoint3.y - mMouth.getIntrinsicHeight()/2, mPoint3.x + mMouth.getIntrinsicWidth()/2, mPoint3.y + mMouth.getIntrinsicHeight()/2);
            mMouth.draw(canvas);
        }
    }

    public Rect getRect() {
        int left = Math.min(mPoint1.x ,Math.min(mPoint2.x,mPoint3.x))- Math.abs(mPoint2.x - mPoint1.x)/2;
        int top = Math.min(mPoint1.y ,Math.min(mPoint2.y,mPoint3.y)) - Math.abs(mPoint2.x - mPoint1.x)/2;
        int right = Math.max(mPoint1.x ,Math.max(mPoint2.x,mPoint3.x)) + Math.abs(mPoint2.x - mPoint1.x)/2;
        int bottom = Math.max(mPoint1.y ,Math.max(mPoint2.y,mPoint3.y)) + Math.abs(mPoint2.x - mPoint1.x)/2;
        if(left < right && top < bottom) {
            /*return new Rect(Math.min(mPoint1.x ,Math.min(mPoint2.x,mPoint3.x))- Math.abs(mPoint2.x - mPoint1.x)/2,  Math.min(mPoint1.y ,Math.min(mPoint2.y,mPoint3.y)) - Math.abs(mPoint2.x - mPoint1.x)/2,
                    Math.max(mPoint1.x ,Math.max(mPoint2.x,mPoint3.x)) + Math.abs(mPoint2.x - mPoint1.x)/2, Math.max(mPoint1.y ,Math.max(mPoint2.y,mPoint3.y)) + Math.abs(mPoint2.x - mPoint1.x)/2);*/
            return new Rect(left, top, right, bottom);
        } else {
            return new Rect(mPoint1.x-20 ,mPoint1.y-20,mPoint1.x+20,mPoint1.y+20);
        }
    }
    public Point getPoint1(){
        return mPoint1;
    }
    public Point getPoint2(){
        return mPoint2;
    }
    public Point getPoint3(){
        return mPoint3;
    }
    private Point mLastMove = new Point();
    private Drawable mEyesLeft;
    private Drawable mEyesRight;
    private Drawable mMouth;
    private int mLeftBound;
    private int mTopBound;
    private int mRightBound;
    private int mButtomBound;
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        switch (event.getAction()) {
        case MotionEvent.ACTION_DOWN:
            mDownPoint.set((int) event.getX(), (int) event.getY());
            mLastMove.set(mDownPoint.x, mDownPoint.y);
                if (mPoint1Rect.contains(mDownPoint.x, mDownPoint.y)) {
                    mDownState = 1;
                } else if (mPoint2Rect.contains(mDownPoint.x, mDownPoint.y)) {
                    mDownState = 2;
                } else if (mPoint3Rect.contains(mDownPoint.x, mDownPoint.y)) {
                    mDownState = 3;
                } else if (isInsideTriangle(mPoint1, mPoint2, mPoint3, mDownPoint)) {
                    mDownState = 4;
                } else {
                    mDownState = 0;
                }
            break;

        case MotionEvent.ACTION_MOVE:
            mMovePoint.set((int) event.getX(), (int) event.getY());
            if (mMovePoint.x < mLeftBound || mMovePoint.x > mRightBound
                    || mMovePoint.y < mTopBound
                    || mMovePoint.y > mButtomBound)
                return true;
            switch (mDownState) {
            case 1:
                /*
                 * FIX BUG:5798
                 * BUG COMMENT: make the left eye in the left of right eye;
                 * DATE: 2014-01-15
                 */
                if(mPoint2.x - mMovePoint.x >50) {
                    mPoint1.set(mMovePoint.x, mMovePoint.y);
                    postInvalidate();
                    mMoveState = 1;
                }
                break;
            case 2:
                if(mMovePoint.x - mPoint1.x >50) {
                    mPoint2.set(mMovePoint.x, mMovePoint.y);
                    postInvalidate();
                    mMoveState = 2;
                }
                break;
            case 3:
                mPoint3.set(mMovePoint.x, mMovePoint.y);
                postInvalidate();
                mMoveState = 3;
                break;
            case 4:
                int movedX = mMovePoint.x - mLastMove.x;
                int movedY = mMovePoint.y - mLastMove.y;
                if( mPoint1.x + movedX > mLeftBound && mPoint1.x + movedX < mRightBound
                        && mPoint1.y + movedY > mTopBound
                        && mPoint1.y + movedY < mButtomBound
                        && mPoint2.x + movedX > mLeftBound && mPoint2.x + movedX < mRightBound
                        && mPoint2.y + movedY > mTopBound
                        && mPoint2.y + movedY < mButtomBound
                        && mPoint3.x + movedX > mLeftBound && mPoint3.x + movedX < mRightBound
                        && mPoint3.y + movedY > mTopBound
                        && mPoint3.y + movedY < mButtomBound
                        ) {
                    mLastMove.set(mMovePoint.x, mMovePoint.y);
                    mPoint1.set(mPoint1.x + movedX, mPoint1.y + movedY);
                    mPoint2.set(mPoint2.x + movedX, mPoint2.y + movedY);
                    mPoint3.set(mPoint3.x + movedX, mPoint3.y + movedY);
                }
                postInvalidate();
                mMoveState = 4;
                break;
            case 0:
                mMoveState = 0;
                return super.onTouchEvent(event);
            default:
                break;
            }
            break;

        case MotionEvent.ACTION_UP:
            mUpPoint.set((int) event.getX(), (int) event.getY());
            if (mUpPoint.x < mLeftBound || mUpPoint.x > mRightBound
                    || mUpPoint.y < mTopBound
                    || mUpPoint.y > mButtomBound)
                return true;
            switch (mMoveState) {
            case 1:
                if(mPoint2.x - mUpPoint.x >50) {
                    mPoint1.set(mUpPoint.x, mUpPoint.y);
                    mPoint1Rect = new Rect(mPoint1.x - 50, mPoint1.y - 50,
                            mPoint1.x + 50, mPoint1.y + 50);
                }
                break;
            case 2:
                if(mMovePoint.x - mPoint1.x >50) {
                    mPoint2.set(mUpPoint.x, mUpPoint.y);
                    mPoint2Rect = new Rect(mPoint2.x - 50, mPoint2.y - 50,
                            mPoint2.x + 50, mPoint2.y + 50);
                }
                break;
            case 3:
                mPoint3.set(mUpPoint.x, mUpPoint.y);
                mPoint3Rect = new Rect(mPoint3.x - 50, mPoint3.y - 50,
                        mPoint3.x + 50, mPoint3.y + 50);
                break;
            case 4:
                mPoint1Rect = new Rect(mPoint1.x - 50, mPoint1.y - 50,
                        mPoint1.x + 50, mPoint1.y + 50);
                mPoint2Rect = new Rect(mPoint2.x - 50, mPoint2.y - 50,
                        mPoint2.x + 50, mPoint2.y + 50);
                mPoint3Rect = new Rect(mPoint3.x - 50, mPoint3.y - 50,
                        mPoint3.x + 50, mPoint3.y + 50);
                break;
            default:
                break;
            }
            break;

        default:
            break;
        }
        return true;
    }

    public static boolean isInsideTriangle(Point a, Point b, Point c, Point p) {

        double abc = triangleArea(a, b, c);
        double abp = triangleArea(a, b, p);
        double acp = triangleArea(a, c, p);
        double bcp = triangleArea(b, c, p);
        if (abc == abp + acp + bcp) {
            return true;
        } else {
            return false;
        }
    }

    private static double triangleArea(Point a, Point b, Point c) {
        double result = Math.abs((a.x * b.y + b.x * c.y + c.x * a.y - b.x * a.y
                - c.x * b.y - a.x * c.y) / 2.0D);
        return result;
    }
}
