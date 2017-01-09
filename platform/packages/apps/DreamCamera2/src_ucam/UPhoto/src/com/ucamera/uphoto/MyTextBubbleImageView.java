/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PaintFlagsDrawFilter;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Bitmap.Config;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.RelativeLayout;
import android.widget.Toast;

public class MyTextBubbleImageView extends ImageEditViewPreview {
    private static final String TAG = "MyTextBubbleImageView";
    private Context mContext;
    private RectF mVisibilityRectF;
    private ImageEditDesc mImageEditDesc;
    private GestureDetector mGestureDetector;
    private TextBubbleBox mTextBubbleBox;
    private Drawable mVertexHandle;
    private Drawable mBoxZoomHandle;
    private Drawable mLabelRotateHandle;
    private Drawable mDeformHandle1;
    private Drawable mDeformHandle2;
    private Drawable mDeformHandle3;
    private Drawable mDeformHandle4;
    private Rect mDeformHandleRect1;
    private Rect mDeformHandleRect2;
    private Rect mDeformHandleRect3;
    private Rect mDeformHandleRect4;
    private Rect mVertexHandleRect;
    private Rect mBoxZoomHandleRect;
    private Rect mLabelRotateHandleRect;
    private BubbleVertex mBubbleVertex;
    private Matrix mBoxMatrix;
    private int mDegree;
//    private static final int ARROW_BOX_RADIUS = 15;
    // CID 109256 (#1 of 3): UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private float mBoxCenterCircleRadious;
//    private float min_bubble_width_height;
    // CID 109256 (#2 of 3): UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private float mMinBubbleWidth;
    private float mMinBubbleHeight;
    private ViewGroup mViewGroup;
    private ImageEditViewPreview mPreview;
//    private String mInitText;
    private RelativeLayout mRelativeLayout = null;
    private EditText mEditText;
    // CID 109256 (#3 of 3): UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private MyTextBubbleImageView myTextBubbleImageView;
//    private TextBubbleBoxAttribute mTextBubbleBoxAttribute;
    private int handleId = 0; //0: zoom; 1: deform; 2: rotate+scale

    private Canvas mCanvas = null;

    private float mLastStartX;
    private float mLastStartY;
    private boolean mIsEditting = false;

    private float mScale = 1.0f;
    private float mScaleW = 1.0f;
    private float mScaleH = 1.0f;

    private float mBeginTouchX = 0;
    private float mBeginTouchY = 0;
    private float mBeginTouchCenterX = 0;
    private float mBeginTouchCenterY = 0;
    private boolean mIsShowHandle = false;
    private int mMode;
    private static final int OPERATION_BOX_NONE = -1;
    private static final int OPERATION_BOX_MOVE = 0;
    private static final int OPERATION_BOX_ZOOM = 1;
    private static final int OPERATION_VERTEX_GROW = 2;
    private static final int OPERATION_ARROW_STATUS = 3;
    private static final int OPERATION_LABEL_ROTATE = 4;
    private static final int OPERATION_LABEL_DEFORM = 5;
    private PointF startPointF = new PointF();
    private Handler mMainHandler;
    private int mLabelShape = 0;
    private ViewAttributes mUpdateViewAttrs;

    private Paint p1 = new Paint();
    private static final int A = 5;

    public MyTextBubbleImageView(Context context, Handler mainHandler) {
        super(context);
        mContext = context;
        mMainHandler = mainHandler;
        mImageEditDesc = ImageEditDesc.getInstance();
        mGestureDetector = new GestureDetector(context, new MyTextBubbleGestureDetector());
        mVertexHandle = context.getResources().getDrawable(R.drawable.edit_move_icon);
        mBoxZoomHandle = context.getResources().getDrawable(R.drawable.edit_zoom_icon);
        mLabelRotateHandle = context.getResources().getDrawable(R.drawable.edit_decor_handle);
        mDeformHandle1 = context.getResources().getDrawable(R.drawable.edit_label_balloon_size_reduce_pressed);
        mDeformHandle2 = context.getResources().getDrawable(R.drawable.edit_label_balloon_size_reduce_pressed);
        mDeformHandle3 = context.getResources().getDrawable(R.drawable.edit_label_balloon_size_reduce_pressed);
        mDeformHandle4 = context.getResources().getDrawable(R.drawable.edit_label_balloon_size_reduce_pressed);
        mVertexHandleRect = new Rect();
        mLabelRotateHandleRect = new Rect();
        mBoxZoomHandleRect = new Rect();
        mDeformHandleRect1 = new Rect();
        mDeformHandleRect2 = new Rect();
        mDeformHandleRect3 = new Rect();
        mDeformHandleRect4 = new Rect();
        mBubbleVertex = new BubbleVertex(mContext);
        mBoxMatrix = new Matrix();
        // CID 109256 (#3 of 3): UrF: Unread field (FB.URF_UNREAD_FIELD)
        // myTextBubbleImageView = this;
        // CID 109256 (#2 of 3): UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mMinBubbleWidth = mContext.getResources().getDimension(R.dimen.min_bubble_width);
        mMinBubbleHeight = mContext.getResources().getDimension(R.dimen.min_bubble_height);
        // CID 109256 (#1 of 3): UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mBoxCenterCircleRadious = mContext.getResources().getDimension(R.dimen.box_center_circle_radious);
        p1.setColor(Color.RED);
    }

    public void setVisibilityRect(RectF visibilityRectF) {
        mVisibilityRectF = visibilityRectF;
    }

    public void setGroundLayer(ViewGroup viewGroup, ImageEditViewPreview preview) {
        mViewGroup = viewGroup;
        mPreview = preview;
    }

    public Bitmap composeTextBubbleBitmap(){
        if(mTextBubbleBox instanceof TitleBox) {
//            mTextBubbleBox.draw(mCanvas, mBubbleVertex, mBoxMatrix);
            ((TitleBox) mTextBubbleBox).drawTitle(mCanvas, mBubbleVertex, mBoxMatrix);
        } else if(mTextBubbleBox instanceof LabelBox) {
//            mTextBubbleBox.draw(mCanvas, mBubbleVertex, mBoxMatrix);
            ((LabelBox)mTextBubbleBox).drawLabel(mCanvas, mBubbleVertex, mBoxMatrix);
        } else {
            writeTextToBubble();
            myEditHandler.sendEmptyMessage(0);
            if(mTextBubbleBox != null) {
                mTextBubbleBox.draw(mCanvas, mBoxMatrix, mBubbleVertex);
            }
        }
        /*
         * FIX BUG : 5404
         * BUG COMMENT: Avoid null pointer exception
         * DATE: 2013-12-11
         */
        if(mPreview == null) return null;
        Bitmap bottomLayer = mPreview.getBitmap();
        if(bottomLayer == null) return null;

        Bitmap bitmap = null;
        do {
            try {
                //fixed the bug5941
                Config config = bottomLayer.getConfig();
                config = (config == null)? Bitmap.Config.ARGB_8888 : config;
                bitmap = Bitmap.createBitmap(bottomLayer.getWidth(), bottomLayer.getHeight(), config);
            } catch(OutOfMemoryError oom) {
                Log.w(TAG, "composeTextBubbleBitmap(): code has a memory leak is detected....");
            }
            if(bitmap == null) {
                //index is zero, means that any operation can not be Completed
                if(mImageEditDesc.reorganizeQueue() < 2) {
                    ImageEditOperationUtil.showHandlerToast(getContext(), R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                    return null;
                }
            }
        } while(bitmap == null);

        Matrix m = mPreview.getImageMatrix();
        float[] f = new float[9];
        m.getValues(f);

        Matrix matrix = new Matrix();
        Paint p = new Paint();
        Canvas canvas = new Canvas(bitmap);
        canvas.drawBitmap(bottomLayer, matrix, p);
        /*
         * FIX BUG : 5606
         * BUG COMMENT: Logic error
         * DATE: 2013-12-18
         */
//        RectF rectF = ImageEditOperationUtil.resizeRectF(bottomLayer, getWidth(), getHeight());
//        float scaleW = 1f;
//        float scaleH = 1f;
//        if(bottomLayer.getWidth() > getWidth() || bottomLayer.getHeight() > getHeight()) {
//            scaleW = (float)bottomLayer.getWidth() / (float)rectF.width();
//            scaleH = (float)bottomLayer.getHeight() / (float)rectF.height();
//        }
        matrix.postTranslate(-f[2], -f[5]);
//        matrix.postScale(scaleW, scaleH);
        matrix.postScale(1 / mPreview.getScale(), 1 / mPreview.getScale());
        canvas.setDrawFilter(new PaintFlagsDrawFilter(0, Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG)); //fixed the bug30623
        canvas.drawBitmap(getBitmap(), matrix, p);

        return bitmap;
    }


    ////////////////////////////////////////
    private void bottomRightScale(RectF bubbleRectF) {
        float img_w = mVisibilityRectF.width();
        float img_h = mVisibilityRectF.height();

        float txt_w = bubbleRectF.width();
        float txt_h = bubbleRectF.height();

        float scale_w = (txt_w > img_w/2) ? ((img_w/2)/txt_w) : 1;
        float scale_h = (txt_h > img_h/4) ? ((img_h/4)/txt_h) : 1;
        float scale = Math.min(scale_w,scale_h);

        float centerX = mBubbleVertex.getCenterX();
        float centerY = mBubbleVertex.getCenterY();
        mBoxMatrix.postScale(scale, scale);
        mBubbleVertex.updatePoints(mBoxMatrix, OPERATION_BOX_ZOOM, scale, scale, centerX, centerY);

        float dx = mVisibilityRectF.right  - mBubbleVertex.p3.x - Math.min(20f, txt_w * scale *0.5f);
        float dy = mVisibilityRectF.bottom - mBubbleVertex.p3.y - Math.min(20f, txt_h * scale *0.5f);
        mBoxMatrix.postTranslate(dx, dy);
        mBubbleVertex.updatePoints(mBoxMatrix, OPERATION_BOX_MOVE, 1, 1, mBubbleVertex.getCenterX(), mBubbleVertex.getCenterY());
    }
    //////////////////////////////////

    /**
     * set to update the box shape
     * @param labelShape update box shape
     */
    public void setCurrentLabelShape(int labelShape) {
        mLabelShape = labelShape;
    }

    /**
     * set to update the box attributes
     * @param attributes update box attributes
     */
    public void setCurrentViewAttributes(ViewAttributes attributes) {
        mUpdateViewAttrs = attributes;
    }

    public void addTextBubbleBox(int labelShape, ViewAttributes attributes, int[] labelDimen, Matrix labelBodyMatrix) {
        mBubbleVertex.reset();
        int screenWidth = ImageEditDesc.getScreenWidth();
//        int screenHeight = mImageEditDesc.getScreenHeight();
        TextBubbleBoxAttribute textBubbleBoxAttribute = null;
        LabelBoxAttribute labelBoxAttribute = null;
        if(labelShape > ImageEditConstants.LABEL_TITLE_SHAPE) {
            float left   = mVisibilityRectF.width() * 3/8 + mVisibilityRectF.left;
            float top    = mVisibilityRectF.top   + mVisibilityRectF.height()/8;
            float width  = mVisibilityRectF.width()/2;
            float height = mVisibilityRectF.height()/2;
            if (height / width > 0.7f) {
                height = width * 0.7f;
            }

            mBoxMatrix.setTranslate(left, top);
            mBubbleVertex.initBoxPoints(mBoxMatrix, (int)width, (int)height);
            mBubbleVertex.initArrowPoints();

            textBubbleBoxAttribute = new TextBubbleBoxAttribute(mContext, mBubbleVertex, attributes, attributes.getDrawText(), attributes.getTextSize());
        } else if(labelShape == ImageEditConstants.LABEL_LABEL_SHAPE) {
            int labelWidth = labelDimen[0];
            int labelHeight = labelDimen[1];

            int left = (screenWidth - labelWidth) *2/3;
            int top =(int)(mVisibilityRectF.height() / 5 + mVisibilityRectF.top);
            mBoxMatrix.setTranslate(left, top);
            mBubbleVertex.initBoxPoints(mBoxMatrix, labelWidth, labelHeight);
            labelBoxAttribute = new LabelBoxAttribute(mContext, mBubbleVertex, attributes, labelBodyMatrix);
        }

        createTextBox(labelShape, textBubbleBoxAttribute, attributes, labelBoxAttribute);

        if(labelShape == -1 && (mTextBubbleBox instanceof TitleBox)) {
            float centerX = mVisibilityRectF.centerX();
            float centerY = mVisibilityRectF.centerY();
            RectF textRectF = ((TitleBox)mTextBubbleBox).getPathRectF();
            float left = centerX - textRectF.width() / 2;
            float top = centerY - textRectF.height() / 2;
            mBoxMatrix.setTranslate(left, top);
            mBubbleVertex.initBoxPoints(mBoxMatrix, (int)textRectF.width(), (int)textRectF.height());
            bottomRightScale(textRectF);
        }

        Log.d(TAG, "addTextBubbleBox(): mBoxMatrix = " + mBoxMatrix
                + ", mVisibilityRectF = " + mVisibilityRectF + ", mVisibilityRectF = " + mVisibilityRectF);
        buildHandlePosition(mBoxMatrix, mVisibilityRectF.centerX(), mVisibilityRectF.centerY());

//        invalidate();
    }

    protected void buildHandlePosition(Matrix matrix, final float windowCenterX, final float windowCenterY) {
        float[] f = new float[9];
        matrix.getValues(f);
        float x1 = mBubbleVertex.p1.x;
        float y1 = mBubbleVertex.p1.y;

        float x2 = mBubbleVertex.p2.x;
        float y2 = mBubbleVertex.p2.y;

        float x3 = mBubbleVertex.p3.x;
        float y3 = mBubbleVertex.p3.y;

        float x4 = mBubbleVertex.p4.x;
        float y4 = mBubbleVertex.p4.y;

        float x5 = mBubbleVertex.p5.x;
        float y5 = mBubbleVertex.p5.y;

//        setLabelDeformHandle((int)x1, (int)y1, (int)x2, (int)y2, (int)x3, (int)y3, (int)x4, (int)y4);

        int boxHandleLeft = 0;
        int boxHandleTop = 0;
        int boxHandleRight = 0;
        int boxHandleBottom = 0;
        int boxHandleWidth = mBoxZoomHandle.getIntrinsicWidth() / 2;
        int boxHandleHeight = mBoxZoomHandle.getIntrinsicHeight() / 2;

        double distance1 = Math.sqrt((x1 - windowCenterX) * (x1 - windowCenterX)
                                        + (y1 - windowCenterY) * (y1 - windowCenterY));
        double distance2 = Math.sqrt((x2 - windowCenterX) * (x2 - windowCenterX)
                                    + (y2 - windowCenterY) * (y2 - windowCenterY));
        double distance3 = Math.sqrt((x3 - windowCenterX) * (x3 - windowCenterX)
                                    + (y3 - windowCenterY) * (y3 - windowCenterY));
        double distance4 = Math.sqrt((x4 - windowCenterX) * (x4 - windowCenterX)
                                    + (y4 - windowCenterY) * (y4 - windowCenterY));

        double distance = Math.min(Math.min(distance1, distance2), Math.min(distance3, distance4));
        if(distance == distance1){
            boxHandleLeft = (int) (x1 - boxHandleWidth);
            boxHandleTop = (int) (y1 - boxHandleHeight);
            boxHandleRight = (int) (x1 + boxHandleWidth);
            boxHandleBottom = (int) (y1 + boxHandleHeight);
        }else if(distance == distance2){
            boxHandleLeft = (int) (x2 - boxHandleWidth);
            boxHandleTop = (int) (y2 - boxHandleHeight);
            boxHandleRight = (int) (x2 + boxHandleWidth);
            boxHandleBottom = (int) (y2 + boxHandleHeight);
        }else if(distance == distance3){
            boxHandleLeft = (int) (x3- boxHandleWidth);
            boxHandleTop = (int) (y3 - boxHandleHeight);
            boxHandleRight = (int) (x3 + boxHandleWidth);
            boxHandleBottom = (int) (y3 + boxHandleHeight);
        }else{
            boxHandleLeft = (int) (x4 - boxHandleWidth);
            boxHandleTop = (int) (y4 - boxHandleHeight);
            boxHandleRight = (int) (x4 + boxHandleWidth);
            boxHandleBottom = (int) (y4 + boxHandleHeight);
        }
        mBoxZoomHandleRect.set(boxHandleLeft, boxHandleTop, boxHandleRight, boxHandleBottom);
        mBoxZoomHandle.setBounds(mBoxZoomHandleRect);

        mLabelRotateHandleRect.set(boxHandleLeft, boxHandleTop, boxHandleRight, boxHandleBottom);
        mLabelRotateHandle.setBounds(mLabelRotateHandleRect);

        int vertexHandleLeft = (int) (x5 - mVertexHandle.getIntrinsicWidth() / 2);
        int vertexHandleTop = (int) (y5 - mVertexHandle.getIntrinsicHeight() / 2);
        int vertexHandleRight = (int) (x5 + mVertexHandle.getIntrinsicWidth() / 2);
        int vertexHandelBottom = (int) (y5 + mVertexHandle.getIntrinsicHeight() / 2);
        mVertexHandleRect.set(vertexHandleLeft, vertexHandleTop, vertexHandleRight, vertexHandelBottom);
        mVertexHandle.setBounds(mVertexHandleRect);
    }

    private void setLabelDeformHandle(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) {
        int deformWidth = mDeformHandle1.getIntrinsicWidth() / 2;
        int deformHeight = mDeformHandle1.getIntrinsicHeight() / 2;

        mDeformHandleRect1.set(x1 - deformWidth, y1 - deformHeight, x1 + deformWidth, y1 + deformHeight);
        mDeformHandle1.setBounds(mDeformHandleRect1);

        mDeformHandleRect2.set(x2 - deformWidth, y2 - deformHeight, x2 + deformWidth, y2 + deformHeight);
        mDeformHandle2.setBounds(mDeformHandleRect2);

        mDeformHandleRect3.set(x3 - deformWidth, y3 - deformHeight, x3 + deformWidth, y3 + deformHeight);
        mDeformHandle3.setBounds(mDeformHandleRect3);

        mDeformHandleRect4.set(x4 - deformWidth, y4 - deformHeight, x4 + deformWidth, y4 + deformHeight);
        mDeformHandle4.setBounds(mDeformHandleRect4);
    }

    public void updateTextBox(int labelShape, ViewAttributes attributes, int[] labelDimen, Matrix labelBodyMatrix) {
        TextBubbleBoxAttribute textBubbleBoxAttribute = null;
        LabelBoxAttribute labelBoxAttribute = null;
        if(labelShape > ImageEditConstants.LABEL_TITLE_SHAPE) {
            mBoxMatrix.reset();
            mBoxMatrix.setTranslate(mBubbleVertex.p1.x, mBubbleVertex.p1.y);
            textBubbleBoxAttribute = new TextBubbleBoxAttribute(mContext, mBubbleVertex, attributes, attributes.getDrawText(), attributes.getTextSize());
        } else if(labelShape == ImageEditConstants.LABEL_LABEL_SHAPE) {
            int labelWidth = labelDimen[0];
            int labelHeight = labelDimen[1];
            mBubbleVertex.initBoxPoints(mBoxMatrix, labelWidth, labelHeight);
            labelBoxAttribute = new LabelBoxAttribute(mContext, mBubbleVertex, attributes, labelBodyMatrix);
        }

        createTextBox(labelShape, textBubbleBoxAttribute, attributes, labelBoxAttribute);

        if(labelShape == ImageEditConstants.LABEL_TITLE_SHAPE && (mTextBubbleBox instanceof TitleBox)) {
            RectF textRectF = ((TitleBox)mTextBubbleBox).getPathRectF();
            mBubbleVertex.initBoxPoints(mBoxMatrix, (int)textRectF.width(), (int)textRectF.height());
        }

        buildHandlePosition(mBoxMatrix, mVisibilityRectF.centerX(), mVisibilityRectF.centerY());

        invalidate();
    }

    private void createTextBox(int labelShape, TextBubbleBoxAttribute textBubbleBoxAttribute, ViewAttributes attributes, LabelBoxAttribute labelBoxAttribute) {
        mCanvas = null;
        mCanvas = new Canvas(getBitmap());
        boolean isWillDrawText = true;

        switch(labelShape) {
            case ImageEditConstants.BALLOON_STYLE_OVAL:
                //Oval text bubble box
                mTextBubbleBox = new OvalTextBubbleBox(textBubbleBoxAttribute, mContext);
                break;
            case ImageEditConstants.BALLOON_STYLE_EXPLOSION:
                //Explosion text bubble box
                mTextBubbleBox = new ExplosionTextBubbleBox(textBubbleBoxAttribute, mContext);
                break;
            case ImageEditConstants.BALLOON_STYLE_CLOUND:
                //Clound text bubble box
                mTextBubbleBox = new CloundTextBubbleBox(textBubbleBoxAttribute, mContext);
                break;
            case ImageEditConstants.BALLOON_STYLE_RECT:
                //Rectangle text bubble
                mTextBubbleBox = new RectTextBubbleBox(textBubbleBoxAttribute, mContext);
                break;
            case ImageEditConstants.BALLOON_STYLE_CHAMFER:
                //Chamfer text bubble box
                mTextBubbleBox = new ChamferTextBubbleBox(textBubbleBoxAttribute, mContext);
                break;
            case ImageEditConstants.LABEL_TITLE_SHAPE:
                mTextBubbleBox = new TitleBox(mContext, attributes);
                isWillDrawText = false;
                break;
            case ImageEditConstants.LABEL_LABEL_SHAPE:
                mTextBubbleBox = new LabelBox(labelBoxAttribute, mContext);
                isWillDrawText = false;
                break;
            default:
                mTextBubbleBox = new OvalTextBubbleBox(textBubbleBoxAttribute, mContext);
        }
        mTextBubbleBox.setWillDrawText(isWillDrawText);
    }
    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        /*canvas.drawLine(mBubbleVertex.p1.x - A, mBubbleVertex.p1.y - A, mBubbleVertex.p2.x + A, mBubbleVertex.p2.y - A, p1);
        canvas.drawLine(mBubbleVertex.p2.x + A, mBubbleVertex.p2.y - A, mBubbleVertex.p3.x + A, mBubbleVertex.p3.y + A, p1);
        canvas.drawLine(mBubbleVertex.p3.x + A, mBubbleVertex.p3.y + A, mBubbleVertex.p4.x - A, mBubbleVertex.p4.y + A, p1);
        canvas.drawLine(mBubbleVertex.p4.x - A, mBubbleVertex.p4.y + A, mBubbleVertex.p1.x - A, mBubbleVertex.p1.y - A, p1);*/

        if(mTextBubbleBox instanceof TitleBox) {
            ((TitleBox) mTextBubbleBox).drawTitle(canvas, mBubbleVertex, mBoxMatrix);
            if(mIsShowHandle) {
                if(handleId == 0) {
                    mBoxZoomHandle.draw(canvas);
                } /*else if(handleId == 1) {
                    mDeformHandle1.draw(canvas);
                    mDeformHandle2.draw(canvas);
                    mDeformHandle3.draw(canvas);
                    mDeformHandle4.draw(canvas);
                }*/ else if(handleId == 2) {
                    mLabelRotateHandle.draw(canvas);
                }
            }
        } else if(mTextBubbleBox instanceof LabelBox) {
            ((LabelBox)mTextBubbleBox).drawLabel(canvas, mBubbleVertex, mBoxMatrix);
            if(mIsShowHandle) {
                if(handleId == 0) {
                    mBoxZoomHandle.draw(canvas);
                } else if(handleId == 2){
                    mLabelRotateHandle.draw(canvas);
                }
            }
        } else {
            mTextBubbleBox.draw(canvas, mBoxMatrix, mBubbleVertex);

            if(mIsShowHandle) {
                mBoxZoomHandle.draw(canvas);
                mVertexHandle.draw(canvas);
            }
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if(mGestureDetector.onTouchEvent(event)) {
            return true;
        }
        float touchX = event.getX();
        float touchY = event.getY();

        float startX = startPointF.x;
        float startY = startPointF.y;
        float endX = event.getX();
        float endY = event.getY();

        float distanceX = endX - startX;
        float distanceY = endY - startY;

        int action = event.getAction();
        switch(action) {
            case MotionEvent.ACTION_DOWN:
                startPointF.set(event.getX(), event.getY());
                touchBegin(touchX, touchY);
                break;
            case MotionEvent.ACTION_MOVE:
                touchMoved(touchX, touchY);
                if(mMode == OPERATION_BOX_NONE) {
                    if(touchX == mLastStartX && touchY == mLastStartY){
                        break;
                    }

                    mLastStartX = touchX;
                    mLastStartY = touchY;
                    postTranslateCenter(distanceX, distanceY);
                    if(mPreview != null){
                        mPreview.postTranslateCenter(distanceX, distanceY);
                    }
                    startPointF.set(event.getX(), event.getY());
                }
                break;
            case MotionEvent.ACTION_UP:
                touchEnded(touchX, touchY);
                break;
        }

        return true;
    }

    private void touchBegin(float touchX, float touchY) {
        mBeginTouchX = touchX;
        mBeginTouchY = touchY;
        mBeginTouchCenterX = mBubbleVertex.getCenterX();
        mBeginTouchCenterY = mBubbleVertex.getCenterY();
        if(mIsShowHandle) {
            if(mBoxZoomHandleRect.contains((int)mBeginTouchX, (int)mBeginTouchY) && handleId == 0) {
                mMode = OPERATION_BOX_ZOOM; //scale the Box
                if(!(mTextBubbleBox instanceof TitleBox)) {
                    writeTextToBubble();
                    myEditHandler.sendEmptyMessage(0);
                }
            } else if(mVertexHandleRect.contains((int)mBeginTouchX, (int)mBeginTouchY)) {
                mMode = OPERATION_VERTEX_GROW; // move and rotate the triangle
            } else if(mBubbleVertex.contains(mBeginTouchX, mBeginTouchY)) {
                mMode = OPERATION_BOX_MOVE; // move the Box
            }  else if(mLabelRotateHandleRect.contains((int)mBeginTouchX, (int)mBeginTouchY) && handleId == 2) {
                mMode = OPERATION_LABEL_ROTATE;
            } else if(mDeformHandleRect1.contains((int)mBeginTouchX, (int)mBeginTouchY) || mDeformHandleRect2.contains((int)mBeginTouchX, (int)mBeginTouchY)
                    || mDeformHandleRect3.contains((int)mBeginTouchX, (int)mBeginTouchY) || mDeformHandleRect4.contains((int)mBeginTouchX, (int)mBeginTouchY)
                    && handleId == 1) {
                mMode = OPERATION_LABEL_DEFORM;
            } else {
                mMode = OPERATION_BOX_NONE;
                mIsShowHandle = false;

                if(!(mTextBubbleBox instanceof TitleBox)) {
                    writeTextToBubble();
                    myEditHandler.sendEmptyMessage(0);
                }
            }
        } else {
            if(mBubbleVertex.contains(mBeginTouchX, mBeginTouchY)) {
                mMode = OPERATION_BOX_MOVE;
                mIsShowHandle = true;
            } else if(mBubbleVertex.isContainsArrow(mBeginTouchX, mBeginTouchY)) {
                mMode = OPERATION_ARROW_STATUS;
                mIsShowHandle = true;
            } else {
                mMode = OPERATION_BOX_NONE;
                mIsShowHandle = false;

                if(!(mTextBubbleBox instanceof TitleBox)) {
                    writeTextToBubble();
                    myEditHandler.sendEmptyMessage(0);
                }
            }
        }

        mLastStartX = touchX;
        mLastStartY = touchY;
    }

    private void touchEnded(float touchX, float touchY) {
        if(mMode == OPERATION_BOX_MOVE) {
            mIsShowHandle = true;
            float distanceX = touchX - mBeginTouchX;
            float distanceY = touchY - mBeginTouchY;

            mBubbleVertex.p1.x += distanceX;
            mBubbleVertex.p1.y += distanceY;
            mBubbleVertex.p2.x += distanceX;
            mBubbleVertex.p2.y += distanceY;
            mBubbleVertex.p3.x += distanceX;
            mBubbleVertex.p3.y += distanceY;
            mBubbleVertex.p4.x += distanceX;
            mBubbleVertex.p4.y += distanceY;

            mBubbleVertex.updatePoints(mBoxMatrix, OPERATION_BOX_MOVE, mDegree, -1, mBubbleVertex.getCenterX(), mBubbleVertex.getCenterY());
            if(mTextBubbleBox instanceof TitleBox || mTextBubbleBox instanceof LabelBox) {
                mBubbleVertex.updatePoints(mBoxMatrix, 3, mTheta, mTheta, mBubbleVertex.getCenterX(), mBubbleVertex.getCenterY());
            }
        } else if(mMode == OPERATION_BOX_ZOOM) {
            mIsShowHandle = true;
        } else if(mMode == OPERATION_VERTEX_GROW) {
            mIsShowHandle = true;
            mBubbleVertex.updatePoints(mBoxMatrix, OPERATION_VERTEX_GROW, mDegree, -1, mBubbleVertex.getCenterX(), mBubbleVertex.getCenterY());
        } else if(mMode == OPERATION_ARROW_STATUS) {
            mIsShowHandle = true;
        } else if(mMode == OPERATION_LABEL_ROTATE) {
            mIsShowHandle = true;
            mBubbleVertex.updatePoints(mBoxMatrix, 3, mTheta, mTheta, mBubbleVertex.getCenterX(), mBubbleVertex.getCenterY());
        } else if(mMode == OPERATION_LABEL_DEFORM) {
            mIsShowHandle = true;
        }

        if(mIsShowHandle) {
            final float windowCenterX = mVisibilityRectF.centerX();
            final float windowCenterY = mVisibilityRectF.centerY();
            Log.d(TAG, "touchEnded(): windowCenterX = " + windowCenterX + ", windowCenterX = " + windowCenterX
                    + ", mBoxMatrix = " + mBoxMatrix);
            buildHandlePosition(mBoxMatrix, windowCenterX, windowCenterY);
        }

        mBeginTouchX = touchX;
        mBeginTouchY = touchY;

        invalidate();
    }

    private void touchMoved(float touchX, float touchY) {
        if(mMode == OPERATION_BOX_MOVE) {
            if(Math.abs(touchX - mBeginTouchX) <= 2 || Math.abs(touchY - mBeginTouchY) <= 2){
                return;
            }

            float distanceX = touchX - mBeginTouchX;
            float distanceY = touchY - mBeginTouchY;

            mBubbleVertex.p1.x += distanceX;
            mBubbleVertex.p1.y += distanceY;
            mBubbleVertex.p2.x += distanceX;
            mBubbleVertex.p2.y += distanceY;
            mBubbleVertex.p3.x += distanceX;
            mBubbleVertex.p3.y += distanceY;
            mBubbleVertex.p4.x += distanceX;
            mBubbleVertex.p4.y += distanceY;
            /**
             * FIX BUG: 49
             * BUG CAUSE: the textBubble will moved out of control range
             * FIX COMMENT: set the range for the textBubble and handler
             * Date: 2011-12-30
             */
            if (mBubbleVertex.p1.x <= -10
                    || mBubbleVertex.p2.x > mViewGroup.getWidth() + 10
                    || mBubbleVertex.p1.y <= -10
                    || mBubbleVertex.p3.y > mViewGroup.getHeight() + 10) {
                mBubbleVertex.p1.x -= distanceX;
                mBubbleVertex.p1.y -= distanceY;
                mBubbleVertex.p2.x -= distanceX;
                mBubbleVertex.p2.y -= distanceY;
                mBubbleVertex.p3.x -= distanceX;
                mBubbleVertex.p3.y -= distanceY;
                mBubbleVertex.p4.x -= distanceX;
                mBubbleVertex.p4.y -= distanceY;
                mBeginTouchX = touchX - distanceX;
                mBeginTouchY = touchY - distanceY;
                return;
            }

            mBoxMatrix.postTranslate(distanceX, distanceY);
            mBubbleVertex.updatePoints(mBoxMatrix, OPERATION_BOX_MOVE, mScale, -1, mBubbleVertex.getCenterX(), mBubbleVertex.getCenterY());
            if(!(mTextBubbleBox instanceof TitleBox) || !(mTextBubbleBox instanceof LabelBox)) {
                mBubbleVertex.updateArrowPoints(touchX, touchY, OPERATION_BOX_MOVE);
            } else {
                mBubbleVertex.updatePoints(mBoxMatrix, 3, mTheta, mTheta, mBubbleVertex.getCenterX(), mBubbleVertex.getCenterY());
            }

            mBeginTouchX = touchX;
            mBeginTouchY = touchY;
        } else if(mMode == OPERATION_VERTEX_GROW) {
            if (touchY < mVertexHandle.getIntrinsicWidth() / 3
                    || touchY > mViewGroup.getHeight()
                            - mVertexHandle.getIntrinsicWidth() / 3
                    || touchX < mVertexHandle.getIntrinsicWidth() / 3
                    || touchX > mViewGroup.getWidth()
                            - mVertexHandle.getIntrinsicWidth() / 3) {
                return;
            }
            mBubbleVertex.updateArrowPoints(touchX, touchY, OPERATION_VERTEX_GROW);
            mBeginTouchX = touchX;
            mBeginTouchY= touchY;
        } else if(mMode == OPERATION_BOX_ZOOM) {
            if(!(mTextBubbleBox instanceof TitleBox)) {
                if(Math.abs(touchX - mBubbleVertex.getCenterX()) < mMinBubbleHeight
                        || Math.abs(touchY - mBubbleVertex.getCenterY()) < mMinBubbleHeight) {
                    return;
                }
            }
            if (touchY < mBoxZoomHandle.getIntrinsicWidth()
                    || touchY > mViewGroup.getHeight()
                            - mBoxZoomHandle.getIntrinsicWidth()
                    || touchX < mBoxZoomHandle.getIntrinsicWidth()
                    || touchX > mViewGroup.getWidth()
                            - mBoxZoomHandle.getIntrinsicWidth()) {
                //avoid zoom handle
                return;
            }
            float centerX = mBeginTouchCenterX;
            float centerY = mBeginTouchCenterY;

            if((mTextBubbleBox instanceof TitleBox || mTextBubbleBox instanceof LabelBox) && handleId == 0) {
                float xxx = 0.0f;
                float yyy = 0.0f;
                centerX = mBubbleVertex.getCenterX();
                centerY = mBubbleVertex.getCenterY();
                if(touchX > centerX && touchY < centerY) {
                    centerX = mBubbleVertex.p4.x;
                    centerY = mBubbleVertex.p4.y;
                    xxx = Math.abs(mBubbleVertex.p1.x - touchX);
                    yyy = Math.abs(mBubbleVertex.p3.y - touchY);
                } else if(touchX > centerX && touchY > centerY) {
                    centerX = mBubbleVertex.p1.x;
                    centerY = mBubbleVertex.p1.y;
                    xxx = Math.abs(mBubbleVertex.p1.x - touchX);
                    yyy = Math.abs(mBubbleVertex.p2.y - touchY);
                } else if(touchX < centerX && touchY > centerY) {
                    centerX = mBubbleVertex.p2.x;
                    centerY = mBubbleVertex.p2.y;
                    xxx = Math.abs(mBubbleVertex.p2.x - touchX);
                    yyy = Math.abs(mBubbleVertex.p1.y - touchY);
                } else if(touchX < centerX && touchY < centerY) {
                    centerX = mBubbleVertex.p3.x;
                    centerY = mBubbleVertex.p3.y;
                    xxx = Math.abs(mBubbleVertex.p2.x - touchX);
                    yyy = Math.abs(mBubbleVertex.p4.y - touchY);
                }
                if(xxx < mMinBubbleHeight || yyy < mMinBubbleHeight) {
                    return;
                }
            }

            final float preWidth = Math.abs(mBeginTouchX - centerX);
            final float preHeight = Math.abs(mBeginTouchY - centerY);
            final float postWidth = Math.abs(touchX - centerX);
            final float postHeight = Math.abs(touchY - centerY);

            float scaleW = 1.0f;
            float scaleH = 1.0f;
            if(preWidth != 0) {
                scaleW = postWidth / preWidth;
                if(scaleW > 1.5f) {
                    scaleW = 1.0f;
                }
            }
            if(preHeight != 0) {
                scaleH = postHeight / preHeight;
                if(scaleH > 1.5f) {
                    scaleH = 1.0f;
                }
            }
            mScaleW = scaleW;
            mScaleH = scaleH;
            mBoxMatrix.postScale(mScaleW, mScaleH, centerX, centerY);
            mBubbleVertex.updatePoints(mBoxMatrix, OPERATION_BOX_ZOOM,  mScaleW, mScaleH, centerX, centerY);
            mBeginTouchX = touchX;
            mBeginTouchY= touchY;
        } else if(mMode == OPERATION_LABEL_ROTATE) {
            float preTheta = 0;
            float postTheta = 0;
            float centerX = mBeginTouchCenterX;
            float centerY = mBeginTouchCenterY;
            final float preDistanceX = mBeginTouchX - centerX;
            final float preDistanceY = mBeginTouchY - centerY;
            final float preDistance = (float) Math.sqrt(preDistanceX * preDistanceX + preDistanceY * preDistanceY);
            final float postDistanceX = touchX - centerX;
            final float postDistanceY = touchY - centerY;
            final float postDistance = (float) Math.sqrt(postDistanceX * postDistanceX + postDistanceY * postDistanceY);

            final float gapDistance = (float) Math.sqrt((touchX - mBeginTouchX) * (touchX - mBeginTouchX)
                                                    + (touchY - mBeginTouchY) * (touchY - mBeginTouchY));
            if(gapDistance <= 20){
                return;
            }
            if((touchX < centerX && touchY < centerY)
                    && (touchX > centerX && touchY > centerY)){
                preTheta = 90 * (preDistanceX / preDistance);
                postTheta = 90 * (postDistanceX / postDistance);
            }else{
                preTheta = 90 * (preDistanceY / preDistance);
                postTheta = 90 * (postDistanceY / postDistance);
            }
            final float updateTheta = postTheta - preTheta;

            if(touchX > centerX){
                mTheta += updateTheta;
                mBoxMatrix.postRotate(updateTheta, centerX, centerY);
            }else{
                mTheta -= updateTheta;
                mBoxMatrix.postRotate(-updateTheta, centerX, centerY);
            }
            float scale = 1.0f;
            if(preDistance != 0){
                scale = scale + (postDistance - preDistance) / preDistance;
            }

            mScale = scale;
            mBoxMatrix.postScale(mScale, mScale, centerX, centerY);
            mBubbleVertex.updatePoints(mBoxMatrix, 1,  mScale, mScale, mBubbleVertex.getCenterX(), mBubbleVertex.getCenterY());
            mBeginTouchX = touchX;
            mBeginTouchY = touchY;
        }  else if(mMode == OPERATION_LABEL_DEFORM) {
            float[] src = new float[] {mBubbleVertex.p1.x, mBubbleVertex.p1.y, mBubbleVertex.p2.x, mBubbleVertex.p2.y,
                    mBubbleVertex.p3.x, mBubbleVertex.p3.y, mBubbleVertex.p4.x, mBubbleVertex.p4.y};
            if(mDeformHandleRect1.contains((int)touchX, (int)touchY)) {
                mBubbleVertex.p1.x = touchX;
                mBubbleVertex.p1.y = touchY;
//                mBubbleVertex.updateDeformPoints(1, touchX, touchY);
            } else if(mDeformHandleRect2.contains((int)touchX, (int)touchY)) {
                mBubbleVertex.p2.x = touchX;
                mBubbleVertex.p2.y = touchY;
//                mBubbleVertex.updateDeformPoints(2, touchX, touchY);
            } else if(mDeformHandleRect3.contains((int)touchX, (int)touchY)) {
                mBubbleVertex.p3.x = touchX;
                mBubbleVertex.p3.y = touchY;
//                mBubbleVertex.updateDeformPoints(3, touchX, touchY);
            } else if(mDeformHandleRect4.contains((int)touchX, (int)touchY)) {
                mBubbleVertex.p4.x = touchX;
                mBubbleVertex.p4.y = touchY;
//                mBubbleVertex.updateDeformPoints(4, touchX, touchY);
            }

            float[] dst = new float[] {mBubbleVertex.p1.x, mBubbleVertex.p1.y, mBubbleVertex.p2.x, mBubbleVertex.p2.y,
                    mBubbleVertex.p3.x, mBubbleVertex.p3.y, mBubbleVertex.p4.x, mBubbleVertex.p4.y};

            mBoxMatrix.setTranslate(mBubbleVertex.p1.x, mBubbleVertex.p1.y);
            mBoxMatrix.setPolyToPoly(src, 0, dst, 0, src.length >> 1);

            mBeginTouchX = touchX;
            mBeginTouchY = touchY;
        } else if(mMode == OPERATION_BOX_NONE) {

        }
        mIsShowHandle = false;
        invalidate();
    }
    private float mTheta = 0.0f;
    protected void removeEdittingStatus() {
        if(mIsEditting) {
            mIsEditting = false;
            //fixed the bug31072, when double click the text bubble, the current view can catch the
            //double-click event and zoom in the view,  so when to save the edit status, the current
            //view will zoom in at first, and then zoom out.
            Log.d(TAG, "removeEdittingStatus(): getScale() = " + getScale());
            if(getScale() > 1.0) {
                resetScale();
            }
            mEditText.clearFocus();
            //fixed the bug31701, to click the window screen will hide the input method dialog while the input method dialog is showing.
            InputMethodManager inputMethodManager = (InputMethodManager)mEditText.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            inputMethodManager.hideSoftInputFromWindow(mEditText.getWindowToken(), 2);
            mRelativeLayout.removeView(mEditText);
            mViewGroup.removeView(mRelativeLayout);
        }
    }

    protected void writeTextToBubble() {
        if(mIsEditting) {
            String willDrawString = mEditText.getEditableText().toString();
            mTextBubbleBox.setText(willDrawString);
            mTextBubbleBox.setWillDrawText(true);
        }
    }

    class MyTextBubbleGestureDetector extends GestureDetector.SimpleOnGestureListener {

        @Override
        public boolean onSingleTapUp(MotionEvent event) {
            if(!(mTextBubbleBox instanceof TitleBox || mTextBubbleBox instanceof LabelBox)) {
                return false;
            }
            int x = (int)event.getX();
            int y = (int)event.getY();
            /*if(mTextBubbleBox instanceof LabelBox && mBoxZoomHandleRect.contains(x, y)) {
                mIsShowHandle = true;
                MyTextBubbleImageView.this.invalidate();
                return true;
            } else */if(mBoxZoomHandleRect.contains(x, y) && handleId == 0) {
                mIsShowHandle = true;
//                handleId = 1;
                handleId = 2;
                MyTextBubbleImageView.this.invalidate();
                return true;
            } else if((mDeformHandleRect1.contains(x, y) || mDeformHandleRect2.contains(x, y) || mDeformHandleRect3.contains(x, y) ||
                    mDeformHandleRect4.contains(x, y)) && handleId == 1) {
                mIsShowHandle = true;
                handleId = 2;
                MyTextBubbleImageView.this.invalidate();
                return true;
            } else if(mLabelRotateHandleRect.contains(x, y) && handleId == 2) {
                mIsShowHandle = true;
                handleId = 0;
                MyTextBubbleImageView.this.invalidate();
                return true;
            } else {
                mIsShowHandle = false;
                MyTextBubbleImageView.this.invalidate();
                return super.onSingleTapUp(event);
            }
        }

        @Override
        public void onLongPress(MotionEvent e) {
            /**
             * FIX BUG: 825
             * BUG CAUSE: Enhanced function
             * FIX COMMENT:
             * DATE: 2012-04-24
             */
            if(mBubbleVertex.contains(e.getX(), e.getY())) {
                Message msg = new Message();
                msg.what = ImageEditConstants.ACTION_TAB_LABEL_UPDATE;
                msg.arg1 = mLabelShape;
                msg.obj = mUpdateViewAttrs;
                mMainHandler.sendMessage(msg);
            }
            super.onLongPress(e);
        }

        @Override
        public boolean onDoubleTap(MotionEvent event) {
            /* SPRD: CID 109128 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE) @{
            float x = event.getX();
            float y = event.getY();
            @} */

            /*if(mBubbleVertex.contains(x, y) && !(mTextBubbleBox instanceof TitleBox)) {
                mIsEditting = true;
                mIsShowHandle = false;
                myTextBubbleImageView.invalidate();
                Log.d(TAG, "onDoubleTap(): will add EditText components to this ui");
                float boxLeft = mBubbleVertex.p1.x;
                float boxTop = mBubbleVertex.p1.y;
                float boxRight = mBubbleVertex.p3.x;
                float boxBottom = mBubbleVertex.p3.y;
                float editTextWidth = boxRight - boxLeft;
                float editTextHeight = boxBottom - boxTop;
                mRelativeLayout = new RelativeLayout(mContext);
                mRelativeLayout.setLayoutParams(new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
                mRelativeLayout.setPadding((int)boxLeft, (int)boxTop, 0, 0);

                Paint paint = mTextBubbleBox.mTextPaint;
                mEditText = new EditText(mContext);
                mEditText.setWidth((int)editTextWidth);
                mEditText.setHeight((int)editTextHeight);
                mEditText.setBackgroundColor(Color.TRANSPARENT);
                mEditText.setGravity(Gravity.LEFT | Gravity.TOP);
                mEditText.setTextSize(paint.getTextSize());
                mEditText.setTypeface(Typeface.SANS_SERIF);
                mEditText.setTextColor(paint.getColor());
                //fixed the bug3981, to limit the lenth is less than 200
                mEditText.setFilters(new InputFilter[] {new InputFilter.LengthFilter(200)});
                mRelativeLayout.addView(mEditText);
                mViewGroup.addView(mRelativeLayout);

                mEditText.requestFocus();
                mEditText.setFocusable(true);

                Editable editable = mEditText.getEditableText();
                editable.clear();
                String textStr = mTextBubbleBox.getText();
                String tempStr;
                if(mInitText == textStr) {
                    tempStr = "";
                } else {
                    tempStr = textStr;
                }
                editable.append(tempStr);
                int strLen = tempStr.length();
                Selection.setSelection(editable, strLen);
                mTextBubbleBox.setWillDrawText(false);
                ((InputMethodManager)mEditText.getContext().getSystemService(Context.INPUT_METHOD_SERVICE)).toggleSoftInput(0, 2);
            } else {*/
                //zoom in/out the bitmap;
                pupulateScale(event.getX(), event.getY(), 500);
                if(mPreview != null) {
                    mPreview.pupulateScale(event.getX(), event.getY(), 500);
                }
//            }
            //fixed the bug31943.
            return false;
        }
    }

    private Handler myEditHandler = new Handler() {
        public void handleMessage(Message msg) {
            removeEdittingStatus();
        }
    };

    public void pupulateScale(float centerX, float centerY, float durationMs){
        if(getScale() > 1F){
            zoomTo(1f, centerX, centerY, durationMs);
        }else{
            zoomTo(3f, centerX, centerY, durationMs);
        }
    }

    public float getOffsetX(){
        return mBubbleVertex.p1.x;
    }

    public float getOffsetY(){
        return mBubbleVertex.p1.y;
    }
}
