/**
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ugallery.panorama;

import android.graphics.Bitmap;

public class Texture {
    private int    id;
    private int    width;
    private int    height;
    private float normalizedWidth;
    private float normalizedHeight;
    private Bitmap bitmap;
    private float angle;
    private float offsetAngle;

    public int getId() {
        return id;
    }

    public void setId(int id) {
        this.id = id;
    }

    public Bitmap getBitmap() {
        return bitmap;
    }

    public void setBitmap(Bitmap bitmap) {
        this.bitmap = bitmap;
    }

    public int getWidth() {
        return width;
    }

    public void setWidth(int width) {
        this.width = width;
    }

    public int getHeight() {
        return height;
    }

    public void setHeight(int height) {
        this.height = height;
    }

    public float getNormalizedWidth() {
        return normalizedWidth;
    }

    public void setNormalizedWidth(float normalizedWidth) {
        this.normalizedWidth = normalizedWidth;
    }

    public float getNormalizedHeight() {
        return normalizedHeight;
    }

    public void setNormalizedHeight(float normalizedHeight) {
        this.normalizedHeight = normalizedHeight;
    }

    public float getAngle() {
        return angle;
    }

    public void setAngle(float angle) {
        this.angle = angle;
    }

    public float getOffsetAngle() {
        return offsetAngle;
    }

    public void setOffsetAngle(float offsetAngle) {
        this.offsetAngle = offsetAngle;
    }

}
