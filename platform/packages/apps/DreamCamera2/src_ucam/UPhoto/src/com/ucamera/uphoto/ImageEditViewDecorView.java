/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Handler.Callback;
import android.os.Message;
import android.util.Log;
import android.view.MotionEvent;
import android.widget.ImageView;

public class ImageEditViewDecorView extends ImageView{
    private final static String TAG = "ImageEditViewDecorView";
    private Bitmap mBitmap;
    private RectF decorMaxRectF;
    private DecorPathFVertex drawPathVertex = new DecorPathFVertex();
    private Paint mPaint = new Paint();
    private ModifyMode mMode = ModifyMode.None;
    private Drawable growHandle;
    private Rect growHandleRect = new Rect();
    private Matrix mMatrix = new Matrix();
    private boolean isTouchable = false;
    private Context mContext;
    private final static float BANLANCE = 6.0f;
    private float mTheta = 0.0f;
    private float mScale = 1.0f;
    private Drawable centerFinding;

    public ImageEditViewDecorView(Context context, RectF decorMaxRectF) {
        super(context);
        this.decorMaxRectF = decorMaxRectF;
        Resources resources = context.getResources();
        growHandle = resources.getDrawable(R.drawable.edit_decor_handle);
        centerFinding = resources.getDrawable(R.drawable.edit_decor_center_position);
        mContext = context;
        mPaint.setAntiAlias(true);
    }

    public void setImageBitmap(Bitmap bitmap){
        drawPathVertex.reset();  //fix the bug29077
        mBitmap = bitmap;
        setDecorRectF(bitmap);
        invalidate();
    }

    public void setDecorMaxRectF(RectF decorMaxRectF){
        this.decorMaxRectF = decorMaxRectF;
        setDecorRectF(mBitmap);
    }

    public Bitmap getBitmap(){
        return mBitmap;
    }

    /*
     * FIX BUG: 1666 451
     * BUG CAUSE: This method is ImageView's member method, when sdk version is more than 11, this method would do something.
     * FIX COMMENT: Do not need to override this method.
     * DATE: 2012-10-14
     */
    /*public Matrix getMatrix(){
        return mMatrix;
    }*/

    private void setDecorRectF(Bitmap bitmap) {
        final int decorWidth = bitmap.getWidth();
        final int decorHeight = bitmap.getHeight();
        final float windowCenterX = decorMaxRectF.centerX();
        final float windowCenterY = decorMaxRectF.centerY();
        final float left = windowCenterX - decorWidth / 2;
        final float top = windowCenterY - decorHeight / 2;
        Log.d(TAG, "setDecorRectF(): left = " + left + ", top = " + top);
        mMatrix.setTranslate(left, top);

        drawPathVertex.initPoints(mMatrix, decorHeight, decorWidth);
        setGrowHandlePosition(windowCenterX, windowCenterY);
    }

    private void setGrowHandlePosition(final float windowCenterX,final float windowCenterY) {
        float x1 = drawPathVertex.p1.x;
        float y1 = drawPathVertex.p1.y;
        float x2 = drawPathVertex.p2.x;
        float y2 = drawPathVertex.p2.y;

        float x3 = drawPathVertex.p3.x;
        float y3 = drawPathVertex.p3.y;
        float x4 = drawPathVertex.p4.x;
        float y4 = drawPathVertex.p4.y;

        double distance1 = Math.sqrt((x1 - windowCenterX) * (x1 - windowCenterX)
                                        + (y1 - windowCenterY) * (y1 - windowCenterY));
        double distance2 = Math.sqrt((x2 - windowCenterX) * (x2 - windowCenterX)
                                    + (y2 - windowCenterY) * (y2 - windowCenterY));
        double distance3 = Math.sqrt((x3 - windowCenterX) * (x3 - windowCenterX)
                                    + (y3 - windowCenterY) * (y3 - windowCenterY));
        double distance4 = Math.sqrt((x4 - windowCenterX) * (x4 - windowCenterX)
                                    + (y4 - windowCenterY) * (y4 - windowCenterY));

        float decorCenterX = (x1+x2+x3+x4)/4;
        float decorCenterY = (y1+y2+y3+y4)/4;
        double handlerCenterX =0;
        double handlerCenterY =0;
        double halfDecorDiagonal = Math.sqrt((decorCenterX - x1)*(decorCenterX - x1) +
                                                (decorCenterY - y1)*(decorCenterY - y1));
        double distance = Math.min(Math.min(distance1, distance2), Math.min(distance3, distance4));
        /**
         * FIX BUG: 36
         * BUG CAUSE: when the handle rotate ,get wrong coordinate.
         * FIX COMMENT: according the decor center and vertex coordinate to get the handle coordinate.
         *              the decor center ,vertex and the handle center is on a line.
         * Date: 2011-12-31
         */
        if(distance == distance1){
            handlerCenterX = getHandlePoint(decorCenterX,x1,halfDecorDiagonal);
            handlerCenterY = getHandlePoint(decorCenterY,y1,halfDecorDiagonal);
        }else if(distance == distance2){
            handlerCenterX = getHandlePoint(decorCenterX,x2,halfDecorDiagonal);
            handlerCenterY = getHandlePoint(decorCenterY,y2,halfDecorDiagonal);
        }else if(distance == distance3){
            handlerCenterX = getHandlePoint(decorCenterX,x3,halfDecorDiagonal);
            handlerCenterY = getHandlePoint(decorCenterY,y3,halfDecorDiagonal);
        }else{
            handlerCenterX = getHandlePoint(decorCenterX,x4,halfDecorDiagonal);
            handlerCenterY = getHandlePoint(decorCenterY,y4,halfDecorDiagonal);
        }
        growHandleRect.set((int)handlerCenterX - growHandle.getIntrinsicHeight()/2,
                (int)handlerCenterY - growHandle.getIntrinsicHeight()/2,
                (int)handlerCenterX + growHandle.getIntrinsicHeight()/2,
                (int)handlerCenterY + growHandle.getIntrinsicHeight()/2);
        growHandle.setBounds(growHandleRect);
    }

    /**
     * @param decorCenterPoint the center coordinate for the decor.
     * @param decorVertex the vertex coordinate for the decor.
     * @param halfDecorDiagonal half of the decor diagonal length.
     * @return the center coordinate of handle.
     */
    private double getHandlePoint(float decorCenterPoint, double decorVertex ,double halfDecorDiagonal){
        double handlerCenterPoint;
        if (decorVertex < decorCenterPoint) {
            handlerCenterPoint = decorCenterPoint - (1+growHandle.getIntrinsicHeight()/2/halfDecorDiagonal)*
            Math.abs(decorVertex-decorCenterPoint);
        }else {
            handlerCenterPoint = decorCenterPoint + (1+growHandle.getIntrinsicHeight()/2/halfDecorDiagonal)*
            Math.abs(decorVertex-decorCenterPoint);
        }
        return handlerCenterPoint;
    }

    private boolean overBounds = false;
    private boolean isShowHandle = false;
    @Override
    protected void onDraw(Canvas canvas) {
        canvas.drawBitmap(mBitmap, mMatrix, mPaint);
        if(overBounds){
            Paint p = new Paint();
            p.setColor(android.graphics.Color.RED);
            p.setAlpha(40);
            Path path = new Path();
            path.moveTo(drawPathVertex.p1.x, drawPathVertex.p1.y);
            path.lineTo(drawPathVertex.p2.x, drawPathVertex.p2.y);
            path.lineTo(drawPathVertex.p3.x, drawPathVertex.p3.y);
            path.lineTo(drawPathVertex.p4.x, drawPathVertex.p4.y);
            canvas.drawPath(path, p);

            p.setStrokeWidth(9F);
            p.setColor(android.graphics.Color.GRAY);
            canvas.drawPoint(drawPathVertex.getCenterX(), drawPathVertex.getCenterY(), p);
        }

        if(isShowHandle){
            growHandle.draw(canvas);
        }

        if(mMode == ModifyMode.Grow){
            centerFinding.draw(canvas);
        }

        if(performRefresh){
            invalidate();
        }
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        return super.dispatchTouchEvent(event);
    }

    public boolean isTouchable() {
        return isTouchable;
    }

    public void setTouchable(boolean isTouchable) {
        this.isTouchable = isTouchable;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        final float touchPosX = event.getX();
        final float touchPosY = event.getY();
        final int actionCode = event.getAction();
        switch (actionCode) {
            case MotionEvent.ACTION_UP:
                touchEnded(touchPosX, touchPosY);
                break;
            case MotionEvent.ACTION_DOWN:
                touchBegin(touchPosX, touchPosY);
                break;
            case MotionEvent.ACTION_MOVE:
                touchMoved(touchPosX, touchPosY);
                break;
        }
        return isTouchable;
    }

    private void touchMoved(float touchPosX, float touchPosY) {
        if(mMode == ModifyMode.Move){
            if(Math.abs(touchPosX - beginPosX) <= 2 || Math.abs(touchPosY - beginPosY) <= 2){
                return;
            }
            float distanceX = touchPosX - beginPosX;
            float distanceY = touchPosY - beginPosY;

            drawPathVertex.p1.x += distanceX;
            drawPathVertex.p1.y += distanceY;
            drawPathVertex.p2.x += distanceX;
            drawPathVertex.p2.y += distanceY;
            drawPathVertex.p3.x += distanceX;
            drawPathVertex.p3.y += distanceY;
            drawPathVertex.p4.x += distanceX;
            drawPathVertex.p4.y += distanceY;

            mMatrix.postTranslate(distanceX, distanceY);
            drawPathVertex.updatePoints(mMatrix, 0, mScale);

            drawPathVertex.updatePoints(mMatrix, 2, mTheta);

            beginPosX = touchPosX;
            beginPosY = touchPosY;
        }else if(mMode == ModifyMode.Grow){
            //if the touch cooridinate is in the area of minScaleRectF, we quit the code below to avoid
            //zooming the bitmap in too much and not touchable
            if(minScaleRectF.contains(touchPosX, touchPosY)){
                return;
            }
            float preTheta = 0;
            float postTheta = 0;
            float centerX = beginTouchCenterX;
            float centerY = beginTouchCenterY;
            final float preDistanceX = beginPosX - centerX;
            final float preDistanceY = beginPosY - centerY;
            final float preDistance = (float) Math.sqrt(preDistanceX * preDistanceX + preDistanceY * preDistanceY);
            final float postDistanceX = touchPosX - centerX;
            final float postDistanceY = touchPosY - centerY;
            final float postDistance = (float) Math.sqrt(postDistanceX * postDistanceX + postDistanceY * postDistanceY);

            final float gapDistance = (float) Math.sqrt((touchPosX - beginPosX) * (touchPosX - beginPosX)
                                                    + (touchPosY - beginPosY) * (touchPosY - beginPosY));
            if(gapDistance <= BANLANCE){
                return;
            }
            if((touchPosX < centerX && touchPosY < centerY)
                    && (touchPosX > centerX && touchPosY > centerY)){
                preTheta = 90 * (preDistanceX / preDistance);
                postTheta = 90 * (postDistanceX / postDistance);
            }else{
                preTheta = 90 * (preDistanceY / preDistance);
                postTheta = 90 * (postDistanceY / postDistance);
            }
            final float updateTheta = postTheta - preTheta;

            if(touchPosX > centerX){
                mTheta += updateTheta;
                mMatrix.postRotate(updateTheta, centerX, centerY);
            }else{
                mTheta -= updateTheta;
                mMatrix.postRotate(-updateTheta, centerX, centerY);
            }
//            rotateMatrix.setRotate(mTheta, centerDelta[0], centerDelta[1]);
//            mMatrix.postRotate(mTheta, centerDelta[0], centerDelta[1]);
//            drawPathVertex.updatePoints(mMatrix, 2, mTheta);
            float scale = 1.0f;
            if(preDistance != 0){
                scale = scale + (postDistance - preDistance) / preDistance;
            }

            mScale = scale;
            mMatrix.postScale(mScale, mScale, centerX, centerY);
            drawPathVertex.updatePoints(mMatrix, 1, mScale);
            beginPosX = touchPosX;
            beginPosY = touchPosY;

        }else if(mMode == ModifyMode.None){

        }
        isShowHandle = false;
    }

    private float beginPosX = 0;
    private float beginPosY = 0;
    private float beginTouchCenterX = 0;
    private float beginTouchCenterY = 0;
    private RectF minScaleRectF = new RectF();
    // CID 109275 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // float[] matrixValue = new float[9];
    private void touchBegin(float touchPosX, float touchPosY) {
        beginPosX = touchPosX;
        beginPosY = touchPosY;
        beginTouchCenterX = drawPathVertex.getCenterX();
        beginTouchCenterY = drawPathVertex.getCenterY();
        // fix the bug29183
        minScaleRectF.set(beginTouchCenterX - growHandleRect.width()/2, beginTouchCenterY - growHandleRect.height()/2,
                beginTouchCenterX + growHandleRect.width()/2, beginTouchCenterY + growHandleRect.height()/2);
        if(isShowHandle == true && growHandleRect.contains((int)beginPosX, (int)beginPosY)){
            mMode = ModifyMode.Grow;
            isTouchable = true;
            centerFinding.setBounds((int)(beginTouchCenterX - centerFinding.getIntrinsicWidth() / 2), (int)(beginTouchCenterY - centerFinding.getIntrinsicHeight() / 2)
                                    , (int)(beginTouchCenterX + centerFinding.getIntrinsicWidth() / 2), (int)(beginTouchCenterY + centerFinding.getIntrinsicHeight() / 2));
        }else if(drawPathVertex.contains(beginPosX, beginPosY) && !growHandleRect.contains((int)beginPosX, (int)beginPosY)){
            isShowHandle = true;
            isTouchable = true;
            mMode = ModifyMode.Move;
        }else{
            mMode = ModifyMode.None;
            isShowHandle = false;
            isTouchable = false;
        }
        performRefresh = true;
        invalidate();
    }

    private void touchEnded(float touchPosX, float touchPosY) {
        if(mMode == ModifyMode.Grow){
            isShowHandle = true;
            drawPathVertex.updatePoints(mMatrix, 2, mTheta);
        }else if(mMode == ModifyMode.Move){
            float distanceX = touchPosX - beginPosX;
            float distanceY = touchPosY - beginPosY;

            drawPathVertex.p1.x += distanceX;
            drawPathVertex.p1.y += distanceY;
            drawPathVertex.p2.x += distanceX;
            drawPathVertex.p2.y += distanceY;
            drawPathVertex.p3.x += distanceX;
            drawPathVertex.p3.y += distanceY;
            drawPathVertex.p4.x += distanceX;
            drawPathVertex.p4.y += distanceY;
            isShowHandle = true;
            drawPathVertex.updatePoints(mMatrix, 0, mTheta);
            drawPathVertex.updatePoints(mMatrix, 2, mTheta);
        }

        if(isShowHandle){
            final float windowCenterX = decorMaxRectF.centerX();
            final float windowCenterY = decorMaxRectF.centerY();
            setGrowHandlePosition(windowCenterX, windowCenterY);
        }

        if(overBounds){
            Intent intent = new Intent();
            intent.setAction(ImageEditConstants.ACTION_DECORVIEW_DELETED_RECEIVERD);
            intent.putExtra("index", index);
            mContext.sendBroadcast(intent);
        }
        beginPosX = touchPosX;
        beginPosY = touchPosY;

        mMode = ModifyMode.None;
        performRefresh = false;
    }

    public float getRoateDegree(){
        return mTheta;
    }

    enum ModifyMode { None, Move, Grow }

    protected void lastInvalidateView(long updateMillis) {
        performRefresh = true;
        invalidate();
        if(!handler.hasMessages(ACTION_INVALIDATE_CANCEL)){
            handler.sendEmptyMessageDelayed(ACTION_INVALIDATE_CANCEL, (long) (updateMillis * 1.5));
        }
    }

    private final static int ACTION_INVALIDATE_CANCEL = -1;
    private boolean performRefresh;
    private Handler handler = new Handler(new Callback() {

        public boolean handleMessage(Message msg) {
            switch (msg.what) {
            case ACTION_INVALIDATE_CANCEL:
                performRefresh = false;
                handler.removeMessages(ACTION_INVALIDATE_CANCEL);
                break;
            default:
                break;
            }
            return true;
        }
    });

    public float getScaleX(){
        return drawPathVertex.getScaleX();
    }

    public float getScaleY(){
        return drawPathVertex.getScaleY();
    }

    public float getOffsetX(){
        return drawPathVertex.p1.x;
    }

    public float getOffsetY(){
        return drawPathVertex.p1.y;
    }

    class DecorPathFVertex{
        private float bitmapWidth;
        private float bitmapHeight;
        private float orgWidth;
        private float orgHeight;
        public Point p1 = new Point();
        public Point p2 = new Point();
        public Point p3 = new Point();
        public Point p4 = new Point();
        private Matrix tempMatrix = new Matrix();
        private boolean hasScale;

        public void updatePoints(Matrix matrix, int type, float degree){
            float[] values = new float[9];
            matrix.getValues(values);
            p1.x = values[2];
            p1.y = values[5];
            switch (type) {
            case 0:  // translate
                if(hasScale == false){
                    bitmapWidth = orgWidth * values[0];
                    bitmapHeight = orgHeight * values[4];
                }
                p2.x = p1.x + bitmapWidth;
                p2.y = p1.y;

                p3.x = p2.x;
                p3.y = p2.y + bitmapHeight;

                p4.x = p1.x;
                p4.y = p1.y + bitmapHeight;

                break;
            case 1:  //scale
//                tempMatrix.setScale(degree, degree);
                float[] tempValue = new float[9];
                tempMatrix.postScale(degree, degree, getCenterX(), getCenterY());
                tempMatrix.getValues(tempValue);
                bitmapWidth = orgWidth * tempValue[0];
                bitmapHeight = orgHeight * tempValue[4];
                p2.x = p1.x + bitmapWidth;
                p2.y = p1.y;

                p3.x = p2.x;
                p3.y = p2.y + bitmapHeight;

                p4.x = p1.x;
                p4.y = p1.y + bitmapHeight;

                hasScale = true;
                break;

            case 2:  // rotate
                if(hasScale == false){
                    bitmapWidth = orgWidth * values[0];
                    bitmapHeight = orgHeight * values[4];
                }
                p2.x = (float) (p1.x + bitmapWidth * Math.cos(Math.toRadians(degree)));
                p2.y = (float) (p1.y + bitmapWidth * Math.sin(Math.toRadians(degree)));

                p3.x = (float) (p2.x - bitmapHeight * Math.sin(Math.toRadians(degree)));
                p3.y = (float) (p2.y + bitmapHeight * Math.cos(Math.toRadians(degree)));

                p4.x = (float) (p1.x - bitmapHeight * Math.sin(Math.toRadians(degree)));
                p4.y = (float) (p1.y + bitmapHeight * Math.cos(Math.toRadians(degree)));
                break;
            default:
                break;
            }
        }

        public float getScaleX() {
            return bitmapWidth / orgWidth;
        }

        public float getScaleY(){
            return bitmapHeight / orgHeight;
        }

        public void reset(){
            bitmapWidth = 0;
            bitmapHeight = 0;
            orgWidth = 0;
            orgHeight = 0;
            p1.x = 0;
            p1.y = 0;
            p2.x = 0;
            p2.y = 0;
            p3.x = 0;
            p3.y = 0;
            p4.x = 0;
            p4.y = 0;
            tempMatrix.reset();
            hasScale = false;
            mTheta = 0.0f;
            mScale = 1.0f;
        }

        public void initPoints(Matrix matrix, int decorHeight, int decorWidth) {
            orgHeight = decorHeight;
            orgWidth = decorWidth;
            bitmapWidth = decorWidth;
            bitmapHeight = decorHeight;

            float[] values = new float[9];
            matrix.getValues(values);
            p1.x = values[2];
            p1.y = values[5];

            float width = orgWidth * values[0];
            float height = orgHeight * values[4];
            p2.x = p1.x + width;
            p2.y = p1.y;

            p3.x = p2.x;
            p3.y = p2.y + height;

            p4.x = p1.x;
            p4.y = p1.y + height;
        }

        public void setBitmapHeight(int decorHeight) {
            bitmapHeight = decorHeight;
        }

        public void setBitmapWidth(int decorWidth) {
            bitmapWidth = decorWidth;
        }

        public float getCenterX(){
            return (p1.x + p3.x) / 2;
        }

        public float getCenterY(){
            return (p1.y + p3.y) / 2;
        }

        public boolean contains(float x, float y){
            boolean flag = false;

            float vectorX1 = p2.x - p1.x;
            float vectorY1 = p2.y - p1.y;
            float vectorX2 = x - p1.x;
            float vectorY2 = y - p1.y;
            float vector1 = vectorX1 * vectorY2 - vectorX2 * vectorY1;

            float vectorX3 = p3.x - p2.x;
            float vectorY3 = p3.y - p2.y;
            float vectorX4 = x - p2.x;
            float vectorY4 = y - p2.y;
            float vector2 = vectorX3 * vectorY4 - vectorX4 * vectorY3;

            float vectorX5 = p4.x - p3.x;
            float vectorY5 = p4.y - p3.y;
            float vectorX6 = x - p3.x;
            float vectorY6 = y - p3.y;
            float vector3 = vectorX5 * vectorY6 - vectorX6 * vectorY5;

            float vectorX7 = p1.x - p4.x;
            float vectorY7 = p1.y - p4.y;
            float vectorX8 = x - p4.x;
            float vectorY8 = y - p4.y;
            float vector4 = vectorX7 * vectorY8 - vectorX8 * vectorY7;

            if(vector1 >= 0 && vector2 >= 0 && vector3 >= 0 && vector4 >= 0){
                flag = true;
            }
            if(vector1 <= 0 && vector2 <= 0 && vector3 <= 0 && vector4 <= 0){
                flag = true;
            }
            return flag;
        }

        class Point{
            public float x;
            public float y;
        }
    }

    private int index;
    public void setIndexInParent(int i) {
        index = i;
    }
}
