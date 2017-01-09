/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.Matrix;

public class BubbleVertex {
    private Context mContext;
    private float bitmapWidth;
    private float bitmapHeight;
    private float orgBoxWidth;
    private float orgBoxHeight;
    public Point p1 = new Point(); //Box (left, top)
    public Point p2 = new Point(); //Box (right, top)
    public Point p3 = new Point(); //Box (right, bottom)
    public Point p4 = new Point(); //Box (left, bottom)
    public Point p5 = new Point(); //Arrow vertex 1()
    public Point p6 = new Point(); //Arrow vertex 2(centerX, centerY)
    public Point p7 = new Point(); //Arrow vertex 3(centerX + 40, centerY)
    private boolean hasScale = false;

    private Matrix tempMatrix = new Matrix();

    private static final int OPERATION_BOX_MOVE = 0;
    private static final int OPERATION_BOX_ZOOM = 1;
    private static final int OPERATION_VERTEX_GROW = 2;

    private float mBoxCenterCircleRadious;

    public BubbleVertex(Context context) {
        mContext = context;
        // CID 109172 : SA: Useless self-operation (FB.SA_FIELD_DOUBLE_ASSIGNMENT)
        // mBoxCenterCircleRadious = mContext.getResources().getDimension(R.dimen.box_center_circle_radious);
        mBoxCenterCircleRadious = 10;
    }

    public void updatePoints(Matrix matrix, int type, float attr1, float attr2, float tmpCenterX, float tmpCenterY){
        float[] values = new float[9];
        matrix.getValues(values);
        p1.x = values[2];
        p1.y = values[5];

        switch (type) {
            case OPERATION_BOX_MOVE:
                if(hasScale == false){
                    bitmapWidth = orgBoxWidth *  values[0];
                    bitmapHeight = orgBoxHeight * values[4];
                }

                p2.x = p1.x + bitmapWidth;
                p2.y = p1.y;

                p3.x = p2.x;
                p3.y = p2.y + bitmapHeight;

                p4.x = p1.x;
                p4.y = p1.y + bitmapHeight;
                break;
            case OPERATION_BOX_ZOOM:  //scale
                float[] tempValue = new float[9];
//                tempMatrix.postScale(attr1, attr2, getCenterX(), getCenterY());
                tempMatrix.postScale(attr1, attr2, tmpCenterX, tmpCenterY);
                tempMatrix.getValues(tempValue);
                bitmapWidth = orgBoxWidth *  tempValue[0];
                bitmapHeight = orgBoxHeight * tempValue[4];
                p2.x = p1.x + bitmapWidth;
                p2.y = p1.y;

                p3.x = p2.x;
                p3.y = p2.y + bitmapHeight;

                p4.x = p1.x;
                p4.y = p1.y + bitmapHeight;

                hasScale = true;
                break;
            case OPERATION_VERTEX_GROW:
                /*float[] tempValue2 = new float[9];
                tempMatrix.postScale(attr1, attr2, getCenterX(), getCenterY());
                tempMatrix.getValues(tempValue2);
                bitmapWidth = orgBoxWidth *  tempValue2[0];
                bitmapHeight = orgBoxHeight * tempValue2[4];
                p2.x = p1.x + bitmapWidth;
                p2.y = p1.y;

                p3.x = p2.x;
                p3.y = p2.y + bitmapHeight;

                p4.x = p1.x;
                p4.y = p1.y + bitmapHeight;*/

//                hasScale = true;
                break;
            case 3: //rotate
                if(hasScale == false){
                    bitmapWidth = orgBoxWidth *  values[0];
                    bitmapHeight = orgBoxHeight * values[4];
                }
                p2.x = (float) (p1.x + bitmapWidth * Math.cos(Math.toRadians(attr1)));
                p2.y = (float) (p1.y + bitmapWidth * Math.sin(Math.toRadians(attr1)));

                p3.x = (float) (p2.x - bitmapHeight * Math.sin(Math.toRadians(attr1)));
                p3.y = (float) (p2.y + bitmapHeight * Math.cos(Math.toRadians(attr1)));

                p4.x = (float) (p1.x - bitmapHeight * Math.sin(Math.toRadians(attr1)));
                p4.y = (float) (p1.y + bitmapHeight * Math.cos(Math.toRadians(attr1)));
            default:
                break;
        }
    }

    public void updateDeformPoints(int type, float updateX, float updateY) {
        switch (type) {
            case 1:
                p1.x = updateX;
                p1.y = updateY;
                break;
            case 2:
                p2.x = updateX;
                p2.y = updateY;
                break;
            case 3:
                p3.x = updateX;
                p3.y = updateY;
                break;
            case 4:
                p4.x = updateX;
                p4.y = updateY;
                break;
            default:
                break;
        }
    }

    public void updateArrowPoints(float endX, float endY, int type) {
        if(type == OPERATION_BOX_MOVE) {
            float centerX = getCenterX();
            float centerY = getCenterY();
            float wDistance = centerX - p5.x;
            float hDistance = centerY - p5.y;
            float distance = (float)Math.sqrt(hDistance * hDistance + wDistance * wDistance);
            double rotateDegree = Math.toDegrees(Math.asin(wDistance / distance));

            calculateArrowBasicPointer(rotateDegree, centerX, centerY, centerY > p5.y);
        } else if(type == OPERATION_VERTEX_GROW) {
            float centerX = getCenterX();
            float centerY = getCenterY();
            float wDistance = endX - centerX;
            float hDistance = endY - centerY;
            float distance = (float)Math.sqrt(hDistance * hDistance + wDistance * wDistance);

            double rotateDegree = Math.toDegrees(Math.asin(wDistance / distance));

            p5.x = endX;
            p5.y = endY;
            calculateArrowBasicPointer(rotateDegree, centerX, centerY, endY > centerY);
        }
    }

    public void initBoxPoints(Matrix matrix, int bubbleWidth, int bubbleHeight){
        orgBoxWidth = bubbleWidth;
        orgBoxHeight = bubbleHeight;
        bitmapWidth = bubbleWidth;
        bitmapHeight = bubbleHeight;

        float[] values = new float[9];
        matrix.getValues(values);
        p1.x = values[2];
        p1.y = values[5];

        float width = orgBoxWidth *  values[0];
        float height = orgBoxHeight * values[4];
        p2.x = p1.x + width;
        p2.y = p1.y;

        p3.x = p2.x;
        p3.y = p2.y + height;

        p4.x = p1.x;
        p4.y = p1.y + height;
    }

    public void initArrowPoints() {
        float left = p1.x;
        float top = p1.y;
        float right = p3.x;
        float bottom = p3.y;

        float tempWidth = right - left;
        float tempHeight = bottom - top;
        float centerX = getCenterX();
        float centerY = getCenterY();

        p5.x = tempWidth / 8F + left;
        p5.y = tempHeight / 4F + bottom;

        float wDistance = p5.x - centerX;
        float hDistance = p5.y - centerY;
        float distance = (float)Math.sqrt(hDistance * hDistance + wDistance * wDistance);
        double rotateDegree = Math.toDegrees(Math.asin(wDistance / distance));

        calculateArrowBasicPointer(rotateDegree, centerX, centerY, p5.y > centerY);
    }

    private void calculateArrowBasicPointer(double rotateDegree, float centerX, float centerY, boolean isCenterBelow) {
        if(isCenterBelow) {
            rotateDegree = -rotateDegree;
            p6.x = (float) (centerX + mBoxCenterCircleRadious * Math.cos(Math.toRadians(rotateDegree)));
            p6.y = (float) (centerY + mBoxCenterCircleRadious * Math.sin(Math.toRadians(rotateDegree)));
            p7.x = (float) (centerX - mBoxCenterCircleRadious * Math.cos(Math.toRadians(rotateDegree)));
            p7.y = (float) (centerY - mBoxCenterCircleRadious * Math.sin(Math.toRadians(rotateDegree)));
        } else {
            p6.x = (float) (centerX - mBoxCenterCircleRadious * Math.cos(Math.toRadians(rotateDegree)));
            p6.y = (float) (centerY - mBoxCenterCircleRadious * Math.sin(Math.toRadians(rotateDegree)));
            p7.x = (float) (centerX + mBoxCenterCircleRadious * Math.cos(Math.toRadians(rotateDegree)));
            p7.y = (float) (centerY + mBoxCenterCircleRadious * Math.sin(Math.toRadians(rotateDegree)));
        }
    }

    public float getCenterX(){
        return (p1.x + p3.x) / 2;
    }

    public float getCenterY(){
        return (p1.y + p3.y) / 2;
    }

    public float getScaleX() {
        return bitmapWidth / orgBoxWidth;
    }

    public float getScaleY(){
        return bitmapHeight / orgBoxHeight;
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

        if(vector1 >= 0  && vector2 >= 0 && vector3 >= 0 && vector4 >= 0){
            flag = true;
        }
        if(vector1 <= 0  && vector2 <= 0 && vector3 <= 0 && vector4 <= 0){
            flag = true;
        }
        return flag;
    }

    //The funtion of this method can fixed the bug30256
    public boolean isContainsArrow(float x, float y) {
        boolean flag = false;
        float vectorX1 = p7.x - p6.x;
        float vectorY1 = p7.y - p6.y;
        float vectorX2 = x - p6.x;
        float vectorY2 = y - p6.y;
        float vector1 = vectorX1 * vectorY2 - vectorX2 * vectorY1;

        float vectorX3 = p5.x - p7.x;
        float vectorY3 = p5.y - p7.y;
        float vectorX4 = x - p7.x;
        float vectorY4 = y - p7.y;
        float vector2 = vectorX3 * vectorY4 - vectorX4 * vectorY3;

        float vectorX5 = p6.x - p5.x;
        float vectorY5 = p6.y - p5.y;
        float vectorX6 = x - p5.x;
        float vectorY6 = y - p5.y;
        float vector3 = vectorX5 * vectorY6 - vectorX6 * vectorY5;

        if(vector1 >= 0  && vector2 >= 0 && vector3 >= 0){
            flag = true;
        }
        if(vector1 <= 0  && vector2 <= 0 && vector3 <= 0){
            flag = true;
        }

        return flag;
    }

    public void reset(){
        bitmapWidth = 0;
        bitmapHeight = 0;
        orgBoxWidth = 0;
        orgBoxHeight = 0;
        p1.x = 0;
        p1.y = 0;
        p2.x = 0;
        p2.y = 0;
        p3.x = 0;
        p3.y = 0;
        p4.x = 0;
        p4.y = 0;
        p5.x = 0;
        p5.y = 0;
        p6.x = 0;
        p6.y = 0;
        p7.x = 0;
        p7.y = 0;
        tempMatrix.reset();
        /*mScale = 1.0f;
        mScaleH = 1.0f;
        mScaleW = 1.0f;*/
        hasScale = false;
    }

    class Point{
        public float x;
        public float y;
    }

}
