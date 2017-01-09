
package com.sprd.validationtools.itemstest;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.R;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.view.SurfaceHolder.Callback;
import android.widget.TextView;
import android.widget.Toast;
import android.util.DisplayMetrics;
import android.util.Log;
import android.os.Debug;

import java.util.ArrayList;
import java.util.List;

/**
 * @author guoss
 */
public class SingleTouchPointTest extends BaseActivity {
    private SurfaceView mSurfaceView = null;
    private SurfaceHolder mHolder = null;

    private static final int RADIUS = 30;

    protected float mScreenWidth = 0;

    protected float mScreenHeight = 0;

    private static final float FIXED_TEXT = 50;

    private final List<float[]> mPointList = new ArrayList<float[]>();

    boolean mIsInCircle = false;

    private int mCirclePosition = -1;

    private float[] mMatchCircleXY = null;

    private static final int CIRCLE_X_1 = 0;

    private static final int CIRCLE_Y_1 = 1;

    private static final int CIRCLE_X_2 = 2;

    private static final int CIRCLE_Y_2 = 3;

    private List<List<float[]>> mCirclesList = null;

    private float mTouchUpX = 0;

    private float mTouchUpY = 0;

    private static final int DISTANCE = 90;

    private DisplayMetrics mDisplayMetrics;

    private static final int STOKEN_WIDTH = 10;

    private boolean mIsUpcanvasUnlocked = false;

    protected List<List<float[]>> getCirclesList() {
        float horizontalXY1[] = {
                DISTANCE, DISTANCE, mScreenWidth - DISTANCE, mScreenHeight - DISTANCE
        };
        List<float[]> horizontalList = new ArrayList<float[]>();
        horizontalList.add(horizontalXY1);

        float verticalXY1[] = {
                mScreenWidth - DISTANCE, DISTANCE, DISTANCE, mScreenHeight - DISTANCE
        };
        List<float[]> verticalList = new ArrayList<float[]>();
        verticalList.add(verticalXY1);

        List<List<float[]>> centrePointList = new ArrayList<List<float[]>>();
        centrePointList.add(horizontalList);
        centrePointList.add(verticalList);
        return centrePointList;
    }

    protected boolean isShowCoordinate() {
        return true;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.single_touch_test);
        TextView text = (TextView) findViewById(R.id.title_text);
        text.setText(getString(R.string.touchpoint_info));
        this.mDisplayMetrics = new DisplayMetrics();
        mScreenWidth = mDisplayMetrics.widthPixels;
        getWindowManager().getDefaultDisplay().getMetrics(this.mDisplayMetrics);
        mScreenWidth = getWindowManager().getDefaultDisplay().getWidth();
        mScreenHeight = getWindowManager().getDefaultDisplay().getHeight();
        mSurfaceView = (SurfaceView) findViewById(R.id.surface_point);
        mHolder = mSurfaceView.getHolder();
        mHolder.addCallback(new Callback() {
            public void surfaceDestroyed(SurfaceHolder holder) {
            }

            public void surfaceCreated(SurfaceHolder holder) {
                Canvas canvas = mHolder.lockCanvas();
                canvas.drawColor(Color.WHITE);
                startDraw(canvas);
                showCoordinate(canvas, 0, 0);
                mHolder.unlockCanvasAndPost(canvas);
            }

            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            }
        });
        mCirclesList = getCirclesList();
        //super.onCreate(savedInstanceState);
        super.removeButton();
    }

    /**
     * @return get radius of px
     */
    private float getRadiusPx() {
        float radius = -1;
        radius = RADIUS * mDisplayMetrics.density;
        //if (Debug.isDebug()) {
        //    Log.v("VTools", "density = " + mDisplayMetrics.density);
        //}
        return radius;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (mCirclesList == null || mCirclesList.isEmpty()) {
            return false;
        }
        List<float[]> circles = mCirclesList.get(0);
        if (circles == null || circles.isEmpty()) {
            return false;
        }
        int pointCount = event.getPointerCount();
        if (pointCount == 1) {
            int action = event.getAction();
            float touchX = event.getX();
            float touchY = event.getY();
            switch (action) {
                case MotionEvent.ACTION_DOWN:
                    mMatchCircleXY = getMatchCircle(circles, touchX, touchY,
                            getRadiusPx());
                    if (mMatchCircleXY == null) {
                        return false;
                    }
                    mIsInCircle = true;
                    setXY(touchX, touchY);
                    break;
                case MotionEvent.ACTION_UP:
                    if (mHolder == null || mPointList == null) {
                        return false;
                    }
                    mPointList.clear();
                    /*SPRD: fix bug370169 avoid nullpointer @{*/
                    if(mHolder.getSurface().isValid()){
                    /* @}*/
                        Canvas canvasUp = mHolder.lockCanvas();
                        mIsUpcanvasUnlocked = false;
                        if (canvasUp != null && this.hasWindowFocus()) {
                            canvasUp.drawColor(Color.WHITE);
                        }
                        if (mIsInCircle) {
                            if (isInCircle(touchX, touchY, mMatchCircleXY[0],
                                    mMatchCircleXY[1], getRadiusPx())) {
                                if (!circles.isEmpty()) {
                                    if (mCirclePosition >= 0) {
                                        circles.remove(mCirclePosition);
                                    }
                                }
                                if (circles.isEmpty()) {
                                    mCirclesList.remove(0);
                                }
                                if (mCirclesList.isEmpty()) {
                                    Toast.makeText(this, R.string.text_pass, Toast.LENGTH_SHORT).show();
                                    storeRusult(true);
                                    if(!mIsUpcanvasUnlocked) {
                                          mHolder.unlockCanvasAndPost(canvasUp);
                                    }
                                    this.finish();
                                    return false;
                                }
                            } else {
                                storeRusult(false);
                            }
                            mTouchUpX = touchX;
                            mTouchUpY = touchY;
                        }
                        mIsInCircle = false;
                        mCirclePosition = -1;
                        mMatchCircleXY = null;
                        mPointList.clear();
                        startDraw(canvasUp);
                        showCoordinate(canvasUp, mTouchUpX, mTouchUpY);
                        mHolder.unlockCanvasAndPost(canvasUp);
                        mIsUpcanvasUnlocked = true;
                    }
                    break;
                case MotionEvent.ACTION_MOVE:
                    if (!mIsInCircle || mHolder == null || mPointList == null) {
                        return false;
                    }
                    /*SPRD: fix bug370169 avoid nullpointer @{*/
                    if(mHolder.getSurface().isValid()){
                    /* @}*/
                        Canvas canvasMove = mHolder.lockCanvas();
                        if (canvasMove != null && this.hasWindowFocus()) {
                            canvasMove.drawColor(Color.WHITE);
                        }
                        float[] match = circles.get(mCirclePosition);
                        if (isInner(touchX, touchY, match[CIRCLE_X_1],
                                match[CIRCLE_Y_1], match[CIRCLE_X_2],
                                match[CIRCLE_Y_2], getRadiusPx())) {
                            setXY(touchX, touchY);
                            showCoordinate(canvasMove, touchX, touchY);
                        } else {
                            storeRusult(false);
                            mIsInCircle = false;
                            mCirclePosition = -1;
                            mMatchCircleXY = null;
                            mPointList.clear();
                        }
                        startDraw(canvasMove);
                        mHolder.unlockCanvasAndPost(canvasMove);
                    }
                    break;
                default:
                    break;
            }
        }else{
            if(mPointList != null){
                 mPointList.clear();
            }
            mIsInCircle = false;
        }
        return super.onTouchEvent(event);
    }

    /**
     * @param x point x of touching screen
     * @param y point y of touching screen
     * @param list circle array
     * @param radius circle radius
     * @param position circle position
     * @return (x,y) for match circle
     */
    private float[] getMatchCircle(List<float[]> list, float x, float y, double raidus) {
        float[] matchXY = new float[2];
        if (list == null || list.isEmpty()) {
            return null;
        }
        for (int i = 0; i < list.size(); i++) {
            float[] circles = list.get(i);
            float x1 = circles[CIRCLE_X_1];
            float y1 = circles[CIRCLE_Y_1];
            float x2 = circles[CIRCLE_X_2];
            float y2 = circles[CIRCLE_Y_2];
            if (isInCircle(x, y, x1, y1, raidus)) {
                matchXY[0] = x2;
                matchXY[1] = y2;
                mCirclePosition = i;
                return matchXY;
            } else if (isInCircle(x, y, x2, y2, raidus)) {
                matchXY[0] = x1;
                matchXY[1] = y1;
                mCirclePosition = i;
                return matchXY;
            }
        }
        return null;
    }

    // start draw
    private void startDraw(Canvas canvas) {
        if (mCirclesList == null || mCirclesList.isEmpty()) {
            return;
        }
        int iTag = 0;
        for (List<float[]> circles : mCirclesList) {
            for (int i = 0; i < circles.size(); i++) {
                drawShapes(circles.get(i)[CIRCLE_X_1], circles.get(i)[CIRCLE_Y_1],
                        circles.get(i)[CIRCLE_X_2], circles.get(i)[CIRCLE_Y_2], canvas,
                        getRadiusPx(), iTag);
            }
            Paint paint = new Paint();
            paint.setFlags(Paint.ANTI_ALIAS_FLAG);
            paint.setStrokeWidth(STOKEN_WIDTH);
            paint.setColor(Color.RED);
            for (int i = 1; i < mPointList.size(); i++) {
                canvas.drawLine(mPointList.get(i - 1)[0], mPointList.get(i - 1)[1],
                        mPointList.get(i)[0],
                        mPointList.get(i)[1], paint);
            }
            iTag++;
        }
    }

    /**
     * show Coordinate
     */
    private void showCoordinate(Canvas canvas, float touchX, float touchY) {
        Paint paint = new Paint();
        paint.setFlags(Paint.ANTI_ALIAS_FLAG);
        paint.setColor(Color.RED);
        if (isShowCoordinate()) {
            int x = 0;
            int y = 0;
            if (touchX != 0 && touchY != 0) {
                x = (int) touchX;
                y = (int) touchY;
            }
            canvas.drawText("(" + x + "," + y + ")", (mScreenWidth / 2), FIXED_TEXT, paint);
        }
    }

    /**
     * @param x x
     * @param y y save touch screen (x,y)
     */
    private void setXY(float x, float y) {
        float xy[] = {
                x, y
        };
        mPointList.add(xy);
    }

    /**
     * @param x0 point x of touching screen
     * @param y0 point y of touching screen
     * @param x1 centre x of circle1
     * @param y1 centre y of circle1
     * @param x2 centre x of circle2
     * @param y2 centre y of circle2
     * @param radius
     */
    private boolean isInner(float x0, float y0, float x1, float y1, float x2, float y2, float radius) {
        double height = getHeight(x0, y0, x1, y1, x2, y2);
        if (height < radius) {
            return true;
        } else if (height < 0) {
            return false;
        }
        return false;
    }

    /*
     * @param x1 point x1
     * @param y1 point y1
     * @param x2 point x2
     * @param y2 point y2
     * @return two points distance
     */
    private double getTwoPointDistance(double x1, double y1, double x2, double y2) {
        double distance = -1;
        distance = Math.sqrt(Math.abs(x1 - x2) * Math.abs(x1 - x2) + Math.abs(y1 - y2)
                * Math.abs(y1 - y2));
        return distance;
    }

    /**
     * @param x0 point x0
     * @param y0 point y0
     * @param x1 point x1
     * @param y1 point y1
     * @param x2 point x2
     * @param y2 point y2
     * @return length of (x0,y0) to line
     */
    private double getHeight(double x0, double y0, double x1, double y1, double x2, double y2) {
        double height = -1;
        double half = 2;
        double a, b, c;
        a = getTwoPointDistance(x1, y1, x2, y2);
        b = getTwoPointDistance(x1, y1, x0, y0);
        c = getTwoPointDistance(x0, y0, x2, y2);
        double p = (a + b + c) / half;
        double area = Math.sqrt(p * (p - a) * (p - b) * (p - c));
        height = half * area / a;
        return height;
    }

    /**
     * @param x0 point x0
     * @param y0 point y0
     * @param x1 point x1
     * @param y1 point y1
     * @param radius radius
     * @return true if (x0,y0) is in circle ,else false
     */
    private boolean isInCircle(double x0, double y0, double x1, double y1, double radius) {
        double distance = 0;
        distance = Math.sqrt(Math.abs(x0 - x1) * Math.abs(x0 - x1) + Math.abs(y0 - y1)
                * Math.abs(y0 - y1));
        if (distance <= radius) {
            return true;
        }
        return false;
    }

    /**
     * @param x1 pointX1
     * @param x2 pointX2
     * @param y1 pointY1
     * @param y2 pointY2
     * @param canvas canvas
     * @param radius radius
     */
    private boolean drawShapes(float x1, float y1, float x2, float y2, Canvas canvas, float radius,
            int iTag) {
        if (x1 == 0 || y2 == 0 || x2 == 0 || y1 == 0) {
            return false;
        }
        Paint paint = new Paint();
        if (iTag > 0) {
            paint.setColor(Color.GRAY);
        } else {
            paint.setColor(Color.BLUE);
        }
        paint.setFlags(Paint.ANTI_ALIAS_FLAG);
        canvas.drawCircle(x1, y1, radius, paint);
        canvas.drawCircle(x2, y2, radius, paint);
        float slope = (y1 - y2) / (x1 - x2);
        double distanceY = Math.sqrt(radius * radius + radius * radius * slope * slope);
        if (x1 == x2) {
            canvas.drawLine(x1 - radius, 0, x1 - radius, mScreenHeight, paint);
            canvas.drawLine(x1 + radius, 0, x1 + radius, mScreenHeight, paint);
            return true;
        } else if (y1 == y2) {
            canvas.drawLine(0, y1 - radius, mScreenWidth, y1 - radius, paint);
            canvas.drawLine(0, y1 + radius, mScreenWidth, y1 + radius, paint);
            return true;
        } else if (slope > 0) {
            float lineUpY1 = (float) (y1 - x1 * slope - distanceY);
            float lineDownY1 = (float) (y1 - x1 * slope + distanceY);
            float lineUpX2 = (mScreenHeight - lineUpY1) / slope;
            float lineDownX2 = (mScreenHeight - lineDownY1) / slope;
            canvas.drawLine(0, lineUpY1, lineUpX2, mScreenHeight, paint);
            canvas.drawLine(0, lineDownY1, lineDownX2, mScreenHeight, paint);
        } else if (slope < 0) {
            float lineUpY1 = (float) (y1 - x1 * slope - distanceY);
            float lineDownY1 = (float) (y1 - x1 * slope + distanceY);
            float lineUpX2 = (0 - lineUpY1) / slope;
            float lineDownX2 = (0 - lineDownY1) / slope;
            canvas.drawLine(0, lineUpY1, lineUpX2, 0, paint);
            canvas.drawLine(0, lineDownY1, lineDownX2, 0, paint);
        } else {
            return false;
        }
        return true;
    }
}
