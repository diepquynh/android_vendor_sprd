package com.thundersoft.advancedfilter;
import java.util.ArrayList;

import android.content.res.Resources;
import android.content.Context;
import android.graphics.Bitmap;
import android.hardware.Camera;
import android.os.Handler;
import android.util.SparseArray;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.GridView;
import android.widget.HorizontalScrollView;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.LinearLayout;
import android.widget.FrameLayout;

import com.android.camera.PhotoController;
import com.ucamera.ucam.modules.utils.LogUtils;
import com.ucamera.ucam.modules.utils.UiUtils;

import com.android.camera2.R;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.graphics.Paint.FontMetricsInt;
import android.graphics.Rect;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.Canvas;
import android.widget.Toast;
import com.ucamera.ucam.modules.ufilter.UcamFilterPhotoModule;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;

public class SmallAdvancedFilter implements View.OnTouchListener, TsAdvancedFilterNative.OnReceiveBufferListener {
    private static final String TAG = "SmallAdvancedFilter";

    private static final int SNAP_VELOCITY = 80;
    private ViewGroup viewGroup;
    private Context context;
    private LinearLayout layout;
    private HorizontalScrollView hscrllV;
    private boolean isDown;
    private float downX;
    private float lastMovedX;
    private VelocityTracker mVelocityTracker;
    private GridView imgGridView;
    private TsAdvancedFilterSmallGridAdapter mAdapter;
    private ArrayList<View> chacheViews;
    private int scrnNumber;
    private int numberPerSrcn;
    private int currentScrn;
    private int imgW, imgH;
    private float itemOffsetW;
    private int scrnW;
    private boolean isReleaseEffectRes;
    private SparseArray<int []>filtersTable;
    private SparseArray<Integer>gpuEffectTypes;
    private Handler handler = new Handler();
    private int mFilterType;
    private ArrayList<String> textName;
    private ArrayList<Integer> mFilterImage;
    private ArrayList<Integer>mFilterSelectedImage;
    private boolean SupRealPreviewThum = false;
    public UcamFilterPhotoModule mUcamFilterPhotoModule;
    private static Toast mToast;

    private int mPreposition = 0;
    private SettingsManager mSettingManager;
    private int mOrientation;

    public SmallAdvancedFilter(Context context, ViewGroup viewGroup) {
        /*
         * SmallAdvancedFilter implements TsAdvancedFilterNative.OnReceiveBufferListener interface,
         * to callback onReceiveBuffer(int, int[], int, int) method
         * This must do.
         */
        TsAdvancedFilterResInitializer.initializeRes(context);
        TsAdvancedFilterNative.setResPath(TsAdvancedFilterResInitializer.RESOURCE_DIRECTORY);
        TsAdvancedFilterNative.setOnReceiveBufferListener(this);
        initFiltersTable();
        this.context = context;
        this.viewGroup = viewGroup;
//        mCamera = (CameraActivity)context;
        initData();
        initImgGirdView();

        mSettingManager = new SettingsManager(context);

        setFilterType(TsAdvancedFilterNative.ADVANCEDFILTER_AUTUMN);
    }
    private void initFiltersTable() {
        filtersTable = new SparseArray<int[]>();
        int itable = 0;
        /*
         * small filter types
         */

        //filtersTable.put(itable++,new int[]{TsAdvancedFilterNative.ADVANCEDFILTER_FRANCE, TsAdvancedFilterNative.ADVANCEDFILTER_AUTUMN,
        //        TsAdvancedFilterNative.ADVANCEDFILTER_JIANGNAN, TsAdvancedFilterNative.ADVANCEDFILTER_RAINBROW});

        //filtersTable.put(itable++,new int[]{TsAdvancedFilterNative.ADVANCEDFILTER_RAINBROW,TsAdvancedFilterNative.ADVANCEDFILTER_BLESS,
        //        TsAdvancedFilterNative.ADVANCEDFILTER_BLUE,TsAdvancedFilterNative.ADVANCEDFILTER_REFLECTION});

        //filtersTable.put(itable++,new int[]{TsAdvancedFilterNative.ADVANCEDFILTER_REFLECTION,TsAdvancedFilterNative.ADVANCEDFILTER_1839,
        //        TsAdvancedFilterNative.ADVANCEDFILTER_EDGE,TsAdvancedFilterNative.ADVANCEDFILTER_INVERT});

        //filtersTable.put(itable++,new int[]{TsAdvancedFilterNative.ADVANCEDFILTER_INVERT,TsAdvancedFilterNative.ADVANCEDFILTER_NOSTALGIA,
        //        TsAdvancedFilterNative.ADVANCEDFILTER_SKETCH, TsAdvancedFilterNative.ADVANCEDFILTER_EMBOSS});

        /******************************************add new filters **********************************************************************/
        //filtersTable.put(itable++,new int[]{TsAdvancedFilterNative.ADVANCEDFILTER_EMBOSS, TsAdvancedFilterNative.ADVANCEDFILTER_BLACKWHITE,
        //TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_RIGHT, TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_DOWN});

        filtersTable.put(itable++,new int[]{TsAdvancedFilterNative.ADVANCEDFILTER_AUTUMN, TsAdvancedFilterNative.ADVANCEDFILTER_JIANGNAN,
        TsAdvancedFilterNative.ADVANCEDFILTER_RAINBROW, TsAdvancedFilterNative.ADVANCEDFILTER_EDGE});

        filtersTable.put(itable++,new int[]{TsAdvancedFilterNative.ADVANCEDFILTER_INVERT, TsAdvancedFilterNative.ADVANCEDFILTER_NOSTALGIA,
        TsAdvancedFilterNative.ADVANCEDFILTER_SKETCH, TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_RIGHT});

        /*
         * Big preveiw filter types
         */
        int gpuIndex = 0;
        gpuEffectTypes = new SparseArray<Integer>();

        //gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_FRANCE);
        gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_AUTUMN);
        gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_JIANGNAN);
        //gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_RAINBROW);//SPRD:Fix bug 464000 filter menu double icons

        gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_RAINBROW);
        //gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_BLESS);
        //gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_BLUE);
        //gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_REFLECTION);//SPRD:Fix bug 464000 filter menu double icons

        //gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_REFLECTION);
        //gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_1839);
        gpuEffectTypes.put(gpuIndex++,TsAdvancedFilterNative.ADVANCEDFILTER_EDGE);
        //gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_INVERT);//SPRD:Fix bug 464000 filter menu double icons

        gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_INVERT);
        gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_NOSTALGIA);
        gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_SKETCH);
        //gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_EMBOSS);//SPRD:Fix bug 464000 filter menu double icons

        //gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_EMBOSS);
        //gpuEffectTypes.put(gpuIndex++, TsAdvancedFilterNative.ADVANCEDFILTER_BLACKWHITE);
        gpuEffectTypes.put(gpuIndex++,TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_RIGHT);
        //gpuEffectTypes.put(gpuIndex++,TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_DOWN);
    }

    private void initData() {
        scrnW = UiUtils.screenWidth()+UiUtils.screenWidth()/8;
        currentScrn = 0;
        numberPerSrcn = 4;
        scrnNumber = filtersTable.size();
        chacheViews = new ArrayList<View>();
        isDown = false;
        downX = 0.0f;
        lastMovedX = 0.0f;
        textName = new ArrayList<String>();

        //textName.add(context.getResources().getString(R.string.filter_name_huanghun));
        textName.add(context.getResources().getString(R.string.filter_name_riguang));
        textName.add(context.getResources().getString(R.string.filter_name_landiao));
        //textName.add(context.getResources().getString(R.string.filter_name_tianmei));//SPRD:Fix bug 464000 filter menu double icons

        textName.add(context.getResources().getString(R.string.filter_name_tianmei));
        //textName.add(context.getResources().getString(R.string.filter_name_pugongying));
        //textName.add(context.getResources().getString(R.string.filter_name_danya));
        //textName.add(context.getResources().getString(R.string.filter_name_zuhe));//SPRD:Fix bug 464000 filter menu double icons

        //textName.add(context.getResources().getString(R.string.filter_name_zuhe));
        //textName.add(context.getResources().getString(R.string.filter_name_huaijiu));
        textName.add(context.getResources().getString(R.string.filter_name_sumiao));
        //textName.add(context.getResources().getString(R.string.filter_name_fanse));//SPRD:Fix bug 464000 filter menu double icons

        textName.add(context.getResources().getString(R.string.filter_name_fanse));
        textName.add(context.getResources().getString(R.string.filter_name_kafei));
        textName.add(context.getResources().getString(R.string.filter_name_huiyi));
        //textName.add(context.getResources().getString(R.string.filter_name_zhonghui));//SPRD:Fix bug 464000 filter menu double icons

        //textName.add(context.getResources().getString(R.string.filter_name_zhonghui));
        //textName.add(context.getResources().getString(R.string.filter_name_pingdan));
        textName.add(context.getResources().getString(R.string.filter_name_shuipingduiqi));
        //textName.add(context.getResources().getString(R.string.filter_name_chuizhiduiqi));

        if (!SupRealPreviewThum) {
            mFilterImage = new ArrayList<Integer>();
            //mFilterImage.add(R.drawable.filter_dusk);
            mFilterImage.add(R.drawable.filter_sunlight);
            mFilterImage.add(R.drawable.filter_blue);
            //mFilterImage.add(R.drawable.filter_sweet);//SPRD:Fix bug 464000 filter menu double icons

            mFilterImage.add(R.drawable.filter_sweet);
            //mFilterImage.add(R.drawable.filter_dandelion);
            //mFilterImage.add(R.drawable.filter_elegant);
            //mFilterImage.add(R.drawable.filter_group);//SPRD:Fix bug 464000 filter menu double icons

            //mFilterImage.add(R.drawable.filter_group);
            //mFilterImage.add(R.drawable.filter_oldstyle);
            mFilterImage.add(R.drawable.filter_sketch);
            //mFilterImage.add(R.drawable.filter_negative);//SPRD:Fix bug 464000 filter menu double icons

            mFilterImage.add(R.drawable.filter_negative);
            mFilterImage.add(R.drawable.filter_coffee);
            mFilterImage.add(R.drawable.filter_memory);
            //mFilterImage.add(R.drawable.filter_gray);//SPRD:Fix bug 464000 filter menu double icons

            //mFilterImage.add(R.drawable.filter_gray);
            //mFilterImage.add(R.drawable.filter_insipid);
            mFilterImage.add(R.drawable.filter_h_symmetry);
            //mFilterImage.add(R.drawable.filter_v_symmetry);
        }
        /* Dream Camera ui check 190, 191 */
        if (!SupRealPreviewThum) {
            mFilterSelectedImage = new ArrayList<Integer>();
            mFilterSelectedImage.add(R.drawable.filter_sunlight_selected);
            mFilterSelectedImage.add(R.drawable.filter_blue_selected);

            mFilterSelectedImage.add(R.drawable.filter_sweet_selected);
            mFilterSelectedImage.add(R.drawable.filter_sketch_selected);
            mFilterSelectedImage.add(R.drawable.filter_negative_selected);

            mFilterSelectedImage.add(R.drawable.filter_coffee_selected);
            mFilterSelectedImage.add(R.drawable.filter_memory_selected);
            mFilterSelectedImage.add(R.drawable.filter_h_symmetry_selected);
        }
    }
    private void initImgGirdView() {
      layout = (LinearLayout) viewGroup.findViewById(R.id.cpu_small_effects_layout_id);
      hscrllV = (HorizontalScrollView) viewGroup.findViewById(R.id.cpu_small_effects_horizontalScrollV_id);
      //hscrllV.setOnTouchListener(this);//SPRD:Fix bug 464000 filter menu double icons
        int effectNumber = gpuEffectTypes.size();
        int horizontalSpace = computeK(3.0f, scrnW);
        int itemLayoutW = scrnW / numberPerSrcn - horizontalSpace;
        int itemLayoutH = itemLayoutW;
        //float offset = 0.2f;
        float offset = 0;
        float itemW = scrnW / (numberPerSrcn + offset);
        int layoutW = (int) (itemW * effectNumber);
        itemOffsetW = itemLayoutW * offset;
        imgGridView = (GridView) viewGroup.findViewById(R.id.cpu_small_effects_gridv_id);
        imgGridView.setNumColumns(effectNumber);
        imgGridView.setLayoutParams(new LinearLayout.LayoutParams(layoutW,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        imgGridView.setHorizontalSpacing(horizontalSpace);
        GridView.LayoutParams itemLayoutLp = new GridView.LayoutParams(itemLayoutW, itemLayoutH);
        int padding = computeK(12.0f, scrnW);
        imgW = itemLayoutW - padding * 2;
        imgH = imgW;
        for (int i = 0; i < effectNumber; i++) {
         //ImageView imgV = new ImageView(context);
            FilterImageView imgV = new FilterImageView(context);
            imgV.setScaleType(ScaleType.CENTER_INSIDE);
            imgV.setLayoutParams(itemLayoutLp);
            if (SupRealPreviewThum) {
//                imgV.setBackgroundResource(R.drawable.ucam_cpu_effect_btn_selector);
            } else {
                /* Dream Camera ui check 190, 191 */
                imgV.setImageResource(mFilterImage.get(i));
            }
            if (i == 0) {
                /* Dream Camera ui check 190, 191 */
                imgV.setImageResource(mFilterSelectedImage.get(i));
            }
            imgV.setPadding(padding, padding, padding, padding);
            imgV.setText(textName.get(i));
            chacheViews.add(imgV);
        }
        mAdapter = new TsAdvancedFilterSmallGridAdapter(chacheViews);
//        imgGridView.setOnTouchListener(new View.OnTouchListener() {
//            @Override
//            public boolean onTouch(View v, MotionEvent event) {
//                return MotionEvent.ACTION_MOVE == event.getAction() ? true : false;
//            }
//        });
        imgGridView.setAdapter(mAdapter);
        imgGridView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                int firstIndex = firstIndexInCurrentScrn();
                int lastIndex = lastIndexInCurrentScrn();
                /* SPRD:Fix bug 464000 filter menu double icons @{
                if (position > lastIndex || position < firstIndex) {
                    return;
                }
                @} */
                /* Dream Camera ui check 190, 191 */
                ((ImageView)view).setImageResource(mFilterSelectedImage.get(position));
                if(mPreposition != position){
                    /* Dream Camera ui check 190, 191 */
                    ((ImageView)imgGridView.getChildAt(mPreposition)).setImageResource(mFilterImage.get(mPreposition));
                    mPreposition = position;
                }
               int filterType = gpuEffectTypes.get(position);
               setFilterType(filterType);
               hideFaceForSpecialFilter(isFaceDetection);
            }
        });
    }

    public void updateUI(float aspectRatio) {
        Resources res = context.getResources();
        int mBottomBarOptimalHeight = res.getDimensionPixelSize(R.dimen.bottom_bar_height_optimal);
        int mBottomBarMaxHeight = res.getDimensionPixelSize(R.dimen.bottom_bar_height_max);
        int top = layout.getPaddingTop();
        if (aspectRatio > 4f / 3f) {
            layout.setPadding(0, top, 0,  mBottomBarOptimalHeight);
        } else {
            layout.setPadding(0, top, 0, mBottomBarMaxHeight);
        }
    }

    private int computeK(float k, float screenWidth) {
        float sf = screenWidth / 540.0f;
        int rp = (int) (k * sf);
        int si = (int) sf;
        float fs = sf - si;
        if (fs > 0.5f) {
            rp += 1;
        }
        return rp;
    }

    public void setVisibility(int visiblity) {
        layout.setVisibility(visiblity);
        if (visiblity != View.VISIBLE) {
            isReleaseEffectRes = true;
        }
    }

    public int getFilterType(){
        return mFilterType;
    }

    /*
     * Change filter type
     * @param filterType  filter type
     */
    public void setFilterType(int filterType) {
        mSettingManager.set(SettingsManager.SCOPE_GLOBAL,
                Keys.KEY_CAMERA_FILTER_TYPE, filterType);
        mFilterType = filterType;
        updateFilterTypeUI();
    }

    public boolean isVisible () {
       return layout.getVisibility() == View.VISIBLE;
    }

    // SPRD: nj dream camera debug 117
    public void setDreamFilterType(int filterType) {
        for (int i = 0; i < gpuEffectTypes.size(); i++) {
            if (filterType == gpuEffectTypes.get(i) && i != 0) {
                ((ImageView)imgGridView.getChildAt(i)).setImageResource(mFilterSelectedImage.get(i));
                ((ImageView)imgGridView.getChildAt(0)).setImageResource(mFilterImage.get(0));
                mPreposition = i;
                setFilterType(filterType);
                return;
            }
        }
    }

    private synchronized void changeScrn(int which) {
        boolean isEdge = false;
        if (which >= scrnNumber) {
            which = scrnNumber - 1;
            isEdge = true;
        } else if (which < 0) {
            which = 0;
            isEdge = true;
        }
        currentScrn = which;
        if (isEdge) {
            return;
        }
        float lsw = itemOffsetW * which;
        int tox = (int) (currentScrn * scrnW - lsw);
        hscrllV.scrollTo(tox, 0);
    }

    private synchronized int getCurrentScrn() {
        return currentScrn;
    }

    private int[] filtersInCurrentScrn() {
        int cs = getCurrentScrn();
        return filtersTable.get(cs);
    }

    private int firstIndexInCurrentScrn() {
        int index = getCurrentScrn() * numberPerSrcn;
        return index;
    }

    private int lastIndexInCurrentScrn() {
        int fIndex = firstIndexInCurrentScrn();
        int lIndex = fIndex + numberPerSrcn - 1;
        return lIndex;
    }

    private View getItemViewFromGridView(int index) {
        final ArrayList<View> lv = chacheViews;
        if (index < 0 || index >= lv.size()) {
            return null;
        }
        View v = lv.get(index);
        return v;
    }

    /*
     * Receive YUV frame data from camera, and push YUV data to sub thread to process
     *
     */
    public void setPreviewData(final byte[] yuvData, final int frameWidth, final int frameHeight) {
        if (yuvData != null) {
            if (isReleaseEffectRes) {
                isReleaseEffectRes = false;
                TsAdvancedFilterNative.releaseEffectRes();
            }
            /*
             * Accordint to camera facing
             */
            int cameraFacing = TsAdvancedFilterNative.CAMERA_FACING_BACK;
            if (FilterTranslationUtils.getInstance().getCameraId() == Camera.CameraInfo.CAMERA_FACING_FRONT) {
                cameraFacing = TsAdvancedFilterNative.CAMERA_FACING_FRONT;
            }
            int effects[] =  filtersInCurrentScrn();
            /*
             * Function  doEffect
             * process effects.lenght number filters,at the same time
             *
             * @param yuvBuf, the source yuv420sp data which will be processed.Must yuvBuf not null.
             * @param frameWidth, the width of YUV Frame. Must frameWidth > 0
             * @param frameHeight, the height of YUV Frame. Must frameHeight > 0
             * @param dstW, the width of target which you need. Must dstW > 0 && dstW < frameWidth
             * @param dstH, the height of target which you need. Must dstH > 0 && dstH < frameHeight
             *
             * @param matrix, 4 x 4 matrix, has 16 elements float array.Must not null.
             * use matrix to change camera preview orientation, when switch camera facing or surface changed.
             * @see android.opengl.Matrix
             *
             * @param cameraFacing, camera facing.
             * @see the defination of CAMERA_FACING_BACK CAMERA_FACING_FRONT
             *
             * @param effects, the filter ids array which you expect to deal with.
             * @see the defination of ADVANCEDFILTER_MOVED_COLOR
                 ADVANCEDFILTER_EDGE
                 ADVANCEDFILTER_MIRROR_RIGHT2LEFT
                 ADVANCEDFILTER_BROWN
                 etc
             */
            TsAdvancedFilterNative.doEffect(yuvData, frameWidth, frameHeight,
                    imgW, imgH, FilterTranslationUtils.getInstance().getMMatrix(), cameraFacing, effects);
        }
    }
    /*
     * @param what, filter index
     * @param colors, color image pixels array
     * @param width, color image width
     * @param height color image height
     *
     * This method is callback and called by JNI C++
     * @see com.thundersoft.advancedfilter.TsAdvancedFilterNative.MassageSender#onReceiveBuffer(int, int[], int, int)
     *
     * This method is called in sub thread, so can not updata UI state in this method.
     *
     */
    @Override
    public void onReceiveBuffer(int index, int[] colors, int width, int height) {
            if (colors == null || index < 0) {
                return;
            }
            /*
             * Create Android Bitmap, according color pixels
             */
            final Bitmap bmp = Bitmap.createBitmap(colors, width, height, Bitmap.Config.ARGB_8888);
            int whickImg = index;
//            if (index == 0) {
//                updataOffsetImg(bmp);
//            }
            whickImg += firstIndexInCurrentScrn();
            final ImageView imgV = (ImageView) getItemViewFromGridView(whickImg);
//            LogUtils.debug("OwenX", "onReceiveBuffer(): index is " + index + ", whickImg is " + whickImg + ", imgV is " + imgV);
            if (imgV == null) {
                return;
            }
            /*
             * Set Bitmap to ImageView
             * Must post to Android UI thread
             */
            handler.post(new Runnable() {
                public void run() {
                    imgV.setImageBitmap(bmp);
                }
            });
    }
    private void updataOffsetImg(final Bitmap offsetImg) {
        final int lastNext = lastIndexInCurrentScrn() + 1;
        View offsetView = getItemViewFromGridView(lastNext);
        if (null != offsetView) {
            final ImageView offsetImgV = (ImageView) offsetView;
            handler.post(new Runnable() {
                public void run() {
                    offsetImgV.setImageBitmap(offsetImg);
                }
            });
        }
    }

    private void startDown(float downX) {
        if (!isDown) {
            this.downX = downX;
            isDown = true;
        }
    }

    private void endUp() {
        isDown = false;
        downX = 0.0f;
        if (mVelocityTracker != null) {
            mVelocityTracker.recycle();
            mVelocityTracker = null;
        }
    }

    private void scrollBy(int byX) {
        if (currentScrn <= 0 && byX > 0) {
            return;
        }
        if (currentScrn >= scrnNumber - 1 && byX < 0) {
            return;
        }
        hscrllV.scrollBy(byX, 0);
    }

    private void scrollToDst(MotionEvent event) {
        float upX = event.getX();
        int dx = (int) (downX - upX);
        int whichScrn = (dx + scrnW / 2 + currentScrn * scrnW) / scrnW;
        changeScrn(whichScrn);
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        if (mVelocityTracker == null) {
            mVelocityTracker = VelocityTracker.obtain();
        }
        mVelocityTracker.addMovement(event);
        final int action = event.getAction();
        LogUtils.debug(TAG + "OwenX", "onTouch(): action is " + action);
        switch (action) {
            case MotionEvent.ACTION_DOWN:
                downX = event.getX();
                break;
            case MotionEvent.ACTION_MOVE:
                float x = event.getX();
                int moved = (int) (lastMovedX - x);
                int startix = (int) lastMovedX;
                startDown(x);
                lastMovedX = x;
                if (startix != 0) {
                    scrollBy(moved);
                }
                break;
            case MotionEvent.ACTION_UP:
                final VelocityTracker velocityTracker = mVelocityTracker;
                velocityTracker.computeCurrentVelocity(1000);
                int velocityX = (int) velocityTracker.getXVelocity();
                if (velocityX > SNAP_VELOCITY) {
                    changeScrn(currentScrn - 1);
                } else if (velocityX < -SNAP_VELOCITY) {
                    changeScrn(currentScrn + 1);
                } else {
                    scrollToDst(event);
                }
                endUp();
                break;
        }
        return true;
    }

    public void onDestroy() {
        TsAdvancedFilterNative.destroy();
    }

    public boolean isSupportRealPreviewThum() {
        return SupRealPreviewThum;
    }

    /*
     * SPRD:Fix bug 484608 Face detection is mutual exclusion with some filter
     * mode.@{
     */
    public static void showToast(Context context, int msg, int duration) {
        if (mToast == null) {
            mToast = Toast.makeText(context, msg, duration);
        } else {
            mToast.setText(msg);
        }
        mToast.show();
    }

    private boolean isFaceDetection = false;

    public void hideFaceForSpecialFilter(boolean face) {
        isFaceDetection = face;
        if (!face) {return;}
        if (mFilterType == 103 || mFilterType == 101 || mFilterType == 405) {
            mUcamFilterPhotoModule.stopFaceDetection();
            showToast(context, R.string.face_filter_mutex, Toast.LENGTH_SHORT);
        } else {
            // SPRD: Fix bug 587781, Filter mode, The camera occasionally cannot connect to the camera.
            if (mUcamFilterPhotoModule.getCameraState() != PhotoController.SNAPSHOT_IN_PROGRESS) {
                mUcamFilterPhotoModule.startFaceDetection();
                mSettingManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_AI_DATECT, "face");
            }
        }
    }

    public void setUcamFilterPhotoModule(UcamFilterPhotoModule mUcamFilterModule) {
        mUcamFilterPhotoModule = mUcamFilterModule;
    }
    /*@}*/

    /* SPRD: Fix bug 578679,  Add Filter type of symmetry right. @{ */
    public void setOrientation(int orientation) {
        mOrientation = orientation;
        updateFilterTypeUI();
    }

    public void updateFilterTypeUI() {
        if (mFilterType == TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_RIGHT) {
            if (mOrientation == 0) {
                TsAdvancedFilterNative
                        .setEffectType(TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_RIGHT);
            } else if (mOrientation == 90) {
                TsAdvancedFilterNative
                        .setEffectType(TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_UP);
            } else if (mOrientation == 180) {
                TsAdvancedFilterNative
                        .setEffectType(TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_LEFT);
            } else if (mOrientation == 270) {
                TsAdvancedFilterNative
                        .setEffectType(TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_DOWN);
            }
            return;
        }
        TsAdvancedFilterNative.setEffectType(mFilterType);
    }
    /* @}*/

    public void resetFilterItem() {
        imgGridView.getChildAt(0).setBackgroundResource(R.drawable.ucam_cpu_effect_bg_selected);
//        imgGridView.getChildAt(mPreposition).setBackgroundResource(R.drawable.ucam_cpu_effect_btn_selector);
        imgGridView.getChildAt(mPreposition).setBackgroundColor(Color.TRANSPARENT);
        mPreposition = 0;
       int filterType = gpuEffectTypes.get(0);
       setFilterType(filterType);
    }

}

class FilterImageView extends ImageView {
    public FilterImageView(Context context) {
        super(context);
        // TODO Auto-generated constructor stub
    }

    public FilterImageView(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
    }

    public FilterImageView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        // TODO Auto-generated constructor stub
    }

    String text;

    public void setText(String t) {
        text = t;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        int h = this.getHeight();
        int w = this.getWidth();
        Rect targetRect = new Rect(0, h + h / 2 + h / 12, w, 0);
        Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        paint.setStrokeWidth(3);
        paint.setTextSize(23);
        paint.setColor(Color.TRANSPARENT);
        paint.setAlpha(0);
        canvas.drawRect(targetRect, paint);
        paint.setColor(Color.WHITE);
        FontMetricsInt fontMetrics = paint.getFontMetricsInt();
        int baseline = targetRect.top
                + (targetRect.bottom - targetRect.top - fontMetrics.bottom + fontMetrics.top)
                / 2 - fontMetrics.top;
        paint.setTextAlign(Paint.Align.CENTER);
        paint.setFakeBoldText(true);
        Typeface font = Typeface.create(Typeface.SANS_SERIF, Typeface.BOLD);
        paint.setTypeface(font);
        paint.setShadowLayer(20, 2, 2, Color.BLACK);
        canvas.drawText(text, targetRect.centerX(), baseline, paint);
    }
}
