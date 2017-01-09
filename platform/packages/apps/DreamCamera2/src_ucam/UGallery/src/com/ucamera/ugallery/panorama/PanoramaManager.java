/**
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ugallery.panorama;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;
public class PanoramaManager {

    private FloatBuffer mVertexsBuffer;
    private ShortBuffer mFaceOrderBuffer;
    private FloatBuffer mTexcoordsBuffer;
    private Texture mTexture;


    public PanoramaManager(float radius, float length, int precision, Texture texture) {
        super();
        //this.rotation = 0;
        this.mTexture = texture;
        getVers(precision, radius, length, texture.getOffsetAngle(), texture.getAngle());
        getOrder(precision);
        getTeccoords(precision);
    }

    /**
     * @see calculate the vertex coordinates
     * @see Assuming the center of the cylinder(0,0,0)
     * @param r cylinder of radius
     * @param l cylinder of lengths
     * @param n cylinder of accuracy
     */
    private void getVers(int n, float r, float l , float offsetAngle, float angle) {
        float[] vertexsArray = new float[n * 3 * 2 + 6];
        float radian  = (float) ((1.0f / (n - 1))* angle * Math.PI / 180.0f);
        float offset = (float)( offsetAngle * Math.PI/ 180f );

        for (int i = 0; i < n; i++) {
            vertexsArray[i * 3] = (float) Math.sin(offset + radian * i) * r;
            vertexsArray[i * 3 + 1] = l;
            vertexsArray[i * 3 + 2] = (float) Math.cos(offset + radian * i) * r;

            vertexsArray[i * 3 + 3 * n] = (float) Math.sin(offset + radian * i) * r;
            vertexsArray[i * 3 + 1 + 3 * n] = -l;
            vertexsArray[i * 3 + 2 + 3 * n] = (float) Math.cos(offset + radian * i) * r;
        }

        // On the top center 2n+1
        vertexsArray[2 * n * 3] = 0;
        vertexsArray[2 * n * 3 + 1] = l;
        vertexsArray[2 * n * 3 + 2] = 0;
        // Under the bottom center 2n+2
        vertexsArray[2 * n * 3 + 3] = 0;
        vertexsArray[2 * n * 3 + 4] = -l;
        vertexsArray[2 * n * 3 + 5] = 0;

        mVertexsBuffer = createBuffer(vertexsArray);
    }

    private FloatBuffer createBuffer(float[] src) {
        ByteBuffer bb = ByteBuffer.allocateDirect(src.length * 4);
        bb.order(ByteOrder.nativeOrder());
        FloatBuffer buffer = bb.asFloatBuffer();
        buffer.put(src);
        buffer.position(0);
        return buffer;
    }
    /**
     * @see To get the drawing order of the vertices
     * @param n
     */
    private void getOrder(int n) {
        short[] stripArray = new short[n * 4];
        // cylindrical surface of the drawing order of the vertices generate
        for (short i = 0; i < n; i++) {
            stripArray[i * 4] = i;
            stripArray[i * 4 + 1] = (short) (i + n);
            if (i == n - 1) {
                stripArray[i * 4 + 2] = (short) (i + 1 - n);
                stripArray[i * 4 + 3] = (short) (i + 1);
            } else {
                stripArray[i * 4 + 2] = (short) (i + 1);
                stripArray[i * 4 + 3] = (short) (i + n + 1);
            }
        }

        mFaceOrderBuffer = ShortBuffer.wrap(stripArray);
    }

    public void getTeccoords(int n) {
        float width = mTexture.getNormalizedWidth();
        float height = mTexture.getNormalizedHeight();
        float[] texcoords = new float[n * 2 * 2];
        float temp = width / (n - 1);
        for (int i = 0; i < n; i++) {
            texcoords[i * 2] = width - i * temp;
            texcoords[i * 2 + 1] = 0;

            texcoords[i * 2 + n * 2] = width - i * temp;
            texcoords[i * 2 + n * 2 + 1] = height;
        }
        mTexcoordsBuffer = createBuffer(texcoords);
    }

    /**
     * @see draw cylinder
     * @param gl
     */
    public void doDraw(GL10 gl1) {
        GL11 gl = (GL11) gl1;
        gl.glEnable(GL11.GL_TEXTURE_2D);
        gl.glBindTexture(GL11.GL_TEXTURE_2D, mTexture.getId());
        gl.glEnableClientState(GL11.GL_VERTEX_ARRAY);
        gl.glEnableClientState(GL11.GL_TEXTURE_COORD_ARRAY);

        gl.glVertexPointer(3, GL11.GL_FLOAT, 0, mVertexsBuffer);
        gl.glTexCoordPointer(2, GL11.GL_FLOAT, 0, mTexcoordsBuffer);

        gl.glDrawElements(GL11.GL_TRIANGLE_STRIP,mFaceOrderBuffer.array().length - 2, GL11.GL_UNSIGNED_SHORT,mFaceOrderBuffer);

        gl.glFinish();
        gl.glDisableClientState(GL11.GL_TEXTURE_COORD_ARRAY);
        gl.glDisableClientState(GL11.GL_VERTEX_ARRAY);
        gl.glDisable(GL11.GL_TEXTURE_2D);

    }

}
