/** Created by Spreadst */
package com.android.sprdlauncher2;
import com.android.sprdlauncher2.CellLayout;
import com.android.sprdlauncher2.DeleteDropTarget;
import com.android.sprdlauncher2.DragView;
import com.android.sprdlauncher2.Workspace;
import com.android.sprdlauncher2.DragController;
import com.android.sprdlauncher2.DragSource;
import com.android.sprdlauncher2.DropTarget;
import com.android.sprdlauncher2.Launcher;
import com.android.sprdlauncher2.PagedView;

import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.PointF;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.util.FloatMath;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.LinearInterpolator;
import android.view.animation.ScaleAnimation;
import android.view.animation.TranslateAnimation;
import android.view.animation.Animation.AnimationListener;
import android.widget.Scroller;
import android.widget.Toast;

import java.util.ArrayList;
import com.android.sprdlauncher2.R;

public class Previews extends ViewGroup implements DragSource, DropTarget,
        View.OnLongClickListener {

    private int mLeftMargin;
    private int mRightMargin;
    private int mTopMargin;
    private int mMargin;
    private int mPreviewWidth;
    private int mThumbWidth;
    private int mPreviewHeight;
    private int mThumbMargin;
    private Launcher mLauncher;
    private SprdWorkspace mWorkspace;
    static final int MAX_ITEMS_NUM = 99;
    static final int MAX_ITEMS_NUM_PER_SCREEN = 9;
    static final int MIN_ITEMS_NUM = 2;
    private final int mHorNum = 3;
    // SPRD : fix bug280707
    private final int mVetNum = 3;
    private final int mAnimationDuration = 400;
    private final static int TOUCH_STATE_REST = 0;
    private final static int TOUCH_STATE_MULTI = 2;
    private int mTouchState = TOUCH_STATE_REST;
    private float mMultiTouchStart = 0.0f;
    private float mMultiTouchStop = 0.0f;
    private float mFirstMotionX1;
    private float mFirstMotionX2;
    private float mFirstMotionY1;
    private float mFirstMotionY2;
    private DragController mDragController;
    private boolean mIsSort;
    private ArrayList<Animate> mAnimate;
    private boolean mIsAnimEnd=true;
    static final String TAG = "Launcher.Previews";
    static final boolean LOGD = true;
    boolean mClickable = true;
    public static boolean IS_OUT_DRAGING;
    static int STATUSBAR_HEIGHT;
    public boolean touchable = true;
    public int mWindowWidth;
    /* SPRD: Bug 271612 @{ */
    public int mWindowHeight;

    /* @} */
    public Previews(Context context) {
        super(context);
        mContext = context;
        init();
    }

    public Previews(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public Previews(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mContext = context;
        init();
    }

    private void init() {
        mTouchSlop = ViewConfiguration.get(getContext()).getScaledTouchSlop();
        mScroller = new Scroller(mContext);
        mLeftMargin = (int) getResources().getDimension(
                R.dimen.home_preview_marginleft);
        mRightMargin = (int) getResources().getDimension(
                R.dimen.home_preview_marginright);
        mPreviewWidth = (int) getResources().getDimension(
                R.dimen.preview_view_width);
        mPreviewHeight = (int) getResources().getDimension(
                R.dimen.preview_view_height);
        /* SPRD : fix bug280707 @{
        mTopMargin = (int) getResources().getDimension(
                R.dimen.home_preview_margintop);
                */
        int windowH = ((Launcher) mContext).getWindowManager()
                .getDefaultDisplay().getHeight()
                - SprdUtilities.getStatusbarHeight(mContext);
        mTopMargin = (windowH - mPreviewHeight * mVetNum) / (mVetNum + 1);
        /* @} */
        mThumbWidth = (int) getResources().getDimension(
                R.dimen.preview_thumbnail_width);
        mThumbMargin = (int) getResources().getDimension(
                R.dimen.preview_thumbnail_margin);
        STATUSBAR_HEIGHT =(int) getResources().getDimension(
                R.dimen.status_bar_height);
        mAnimate = new ArrayList<Animate>();
//        initAnimate();
    }

    public void initAnimate() {
//        mAnimate.clear();
        while (mAnimate.size() < mWorkspace.getChildCount()) {
            Animate ani = new Animate();
            mAnimate.add(ani);
        }
        while (mAnimate.size() > mWorkspace.getChildCount()) {
            mAnimate.remove(mAnimate.size() - 1);
        }
//        for (int l = 0; l < mWorkspace.getChildCount() - 1; l++) {
//            Animate ani = new Animate();
//            mAnimate.add(ani);
//        }
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        if(!touchable){
            return true;
        }
        return super.dispatchTouchEvent(ev);
    }

    @Override
    protected void onLayout(boolean arg0, int arg1, int arg2, int arg3, int arg4) {
        int count = getChildCount();
        int childLeft = 0;
        int childTop = 0;
        int childRight = 0;
        int childBottom = 0;
        int margin = 0;
        int windowWidth = mWindowWidth;
        margin = (windowWidth - mPreviewWidth * mHorNum - mLeftMargin
                * (mHorNum - 1) - mRightMargin * (mHorNum - 1)) / 2;
//        margin=mLeftMargin;
        this.mMargin = margin;
        int index = 0;
        int sIndex = 0;
        int page = 0;
        if (count > 0) {
            for (int i = 0; i < count; i++) {
                View child = getChildAt(i);
                index = ((PreviewsBaseItem) child).mIndex;
                sIndex = index % MAX_ITEMS_NUM_PER_SCREEN;
                page = (int)index / MAX_ITEMS_NUM_PER_SCREEN;
                childLeft = margin + page * mWindowWidth + mPreviewWidth * (sIndex % mHorNum)
                        + (mLeftMargin + mRightMargin) * (index % mHorNum);
                childTop = mTopMargin + (mPreviewHeight + mTopMargin) * ((int) (sIndex / mHorNum));
                childRight = childLeft + mPreviewWidth;
                childBottom = childTop + mPreviewHeight;
                child.measure(arg3 - arg1, arg4 - arg2);
                child.layout(childLeft, childTop, childRight, childBottom);
            }
        }
    }

    /**
     * To initialize WorkspacePreviewsItem
     * @param index
     * @return
     */
    private ItemLocation getItemLocationByIndex(int index) {
        int margin = 0;
        ItemLocation itemLocation = new ItemLocation();
        if (index >= 0) {
            int sIndex = index % MAX_ITEMS_NUM_PER_SCREEN;
            int windowWidth = mWindowWidth;
            margin = (windowWidth - mPreviewWidth * mHorNum - mLeftMargin
                    * (mHorNum - 1) - mRightMargin * (mHorNum - 1)) / 2;
            itemLocation.mLeft = margin + mPreviewWidth * (sIndex % mHorNum)
                    + (mLeftMargin + mRightMargin) * (sIndex % mHorNum);
            itemLocation.mTop = mTopMargin * 2 + (mPreviewHeight + mTopMargin)
                    * ((int) (sIndex / mHorNum));
            itemLocation.mRight = itemLocation.mLeft + mPreviewWidth;
            itemLocation.mBottom = itemLocation.mTop + mPreviewHeight;
        }
        return itemLocation;
    }

    private View updateOnDismiss(View currentPreviewItem){
        int count = getChildCount();
        /* SPRD : fix bug300711 currentPreviewItem forced conversion to PreviewsDefaultItem
         * throws ClassCastException @{ */
        if (currentPreviewItem != null && (currentPreviewItem instanceof PreviewsDefaultItem)) {
            mWorkspace.mCurrentPage = ((PreviewsDefaultItem)currentPreviewItem).mIndex;
        }else{
            for (int i = 0; i < count; i++) {
                PreviewsBaseItem view = (PreviewsBaseItem) getChildAt(i);
                if (view.mIsCurrentScreen) {
                    mWorkspace.mCurrentPage = view.mIndex;
                    currentPreviewItem = view;
                    break;
                }
            }
        }
        /* @} */
        if (mIsSort) {
            mWorkspace.sortCellLayout(mCurrentIndexsList);
            mIsSort = false;
            for (int i = 0; i < count; i++) {
                PreviewsBaseItem view = (PreviewsBaseItem) getChildAt(i);
                if (view.mIsHomeScreen) {
                    mWorkspace.updateDefaultScreenNum(view.mIndex);
                    break;
                }
            }
        }
        return currentPreviewItem;
    }
    /**
     * Check whether there is being displayed on the dialog, if let it disappear.
     * Set long press cannot use.
     */
    private void readyToDismiss(){
        isDialogShowingAndClose();
        mClickable = false;
    }
    /**
     * Check whether there is being displayed on the dialog
     */
    protected boolean isDialogShowingAndClose(){
        int count = getChildCount();
        for(int i=0;i<count;i++){
            PreviewsBaseItem view = (PreviewsBaseItem) getChildAt(i);
            if(view instanceof PreviewsDefaultItem){
              if(((PreviewsDefaultItem) view).isAlertDialogShowing()){
                  ((PreviewsDefaultItem) view).closeAlertDialog();
                  return true;
              }
            }
        }
        return false;
    }
    /**
     * To initialize and display the animation when dismissing
     * @param currentScreen
     *            the currentScreen of workspace
     * @param child
     *            the animationing view ;if pressed back key ,it is null.
     */
    private void displayDismissAnimation(int workspaceCurrentScreen, View child) {
        child = updateOnDismiss(child);
        readyToDismiss();
        int workspaceWidth = mWindowWidth;
        /* SPRD: Bug 271612 @{ */
        // int workspaceHeight = mWorkspace.getHeight();
        int workspaceHeight = mWindowHeight;
        /* @} */
        int thumbnailWidth = mThumbWidth;
        int thumbnailHeight = (int)(workspaceHeight * (thumbnailWidth/(float)workspaceWidth));
        if(child != null){
            workspaceCurrentScreen = ((PreviewsBaseItem)child).mIndex;
        }
        ItemLocation location = getItemLocationByIndex(workspaceCurrentScreen);
        int thumbnailLeft = location.mLeft+mThumbMargin;
        int thumbnailRight = location.mRight-mThumbMargin;
        int thumbnailTop = location.mTop+mThumbMargin;
        int thumbnailBottom = thumbnailTop+thumbnailHeight;
        // get the center Point of the animationing view,the point relative of
        // the view itself.
        float animationFocusX = thumbnailWidth * thumbnailLeft
                / (float) (thumbnailLeft + (workspaceWidth - thumbnailRight));
        // get the pan and zoom of workspacePreviews.
        float multipleX = (thumbnailLeft + animationFocusX) / animationFocusX;
        float animationFocusY = thumbnailHeight * thumbnailTop
                / (float) (thumbnailTop + (workspaceHeight - thumbnailBottom));
        // the workspace and the workspacePreviews display animation at the same
        // time.
        Animation previewsAnimation = new ScaleAnimation(1, multipleX, 1,
                multipleX, Animation.RELATIVE_TO_SELF, animationFocusX
                        / (float) (thumbnailWidth), Animation.RELATIVE_TO_SELF,
                animationFocusY / (float) thumbnailHeight);
        previewsAnimation.setDuration(mAnimationDuration);
        previewsAnimation
                .setAnimationListener(new DisMissPreviewsAnimationListener(
                        workspaceCurrentScreen));
        Animation workspaceAnimation = new ScaleAnimation(1 / multipleX, 1,
                1 / multipleX, 1, Animation.RELATIVE_TO_SELF, animationFocusX
                        / (float) (thumbnailWidth), Animation.RELATIVE_TO_SELF,
                animationFocusY / (float) thumbnailHeight);
        workspaceAnimation.setDuration(mAnimationDuration);
        mWorkspace.setVisibility(View.VISIBLE);
        mWorkspace.getChildAt(workspaceCurrentScreen).startAnimation(
                workspaceAnimation);
        this.startAnimation(previewsAnimation);
        //back to the click workspace.cell
        mWorkspace.outPreview(workspaceAnimation, workspaceCurrentScreen);
        for (int i = 0; i < getChildCount(); i++) {
            PreviewsBaseItem view = (PreviewsBaseItem) getChildAt(i);
            if (view.mIndex == workspaceCurrentScreen) {
                view.setVisibility(View.GONE);
                break;
            }
        }
    }

    /**
     * To initialize animation when show the previews.
     * @param currentScreen
     *            is the currentScreen of workspace
     */
    private void displayPreviewsAnimation(int workspaceCurrentScreen) {
        mClickable = true;
        int workspaceWidth = mWindowWidth;
        int workspaceHeight = mWorkspace.getHeight();
        ItemLocation itemLocation = getItemLocationByIndex(workspaceCurrentScreen);
        int thumbnailWidth = mThumbWidth;
        int thumbnailHeight = (int)(workspaceHeight * (thumbnailWidth/(float)workspaceWidth));
        int thumbnailLeft = itemLocation.mLeft+mThumbMargin;
        int thumbnailRight = itemLocation.mRight-mThumbMargin;
        int thumbnailTop = itemLocation.mTop+mThumbMargin;
        int thumbnailBottom = thumbnailTop + thumbnailHeight;
        // get the center Point of the animationing view,the point relative of
        // the view itself.
        float animationFocusX = thumbnailWidth * thumbnailLeft
                / (float) (thumbnailLeft + (workspaceWidth - thumbnailRight));
        // get the pan and zoom of workspacePreviews.
        float multipleX = (thumbnailLeft + animationFocusX) / animationFocusX;
        float animationFocusY = thumbnailHeight * thumbnailTop
                / (float) (thumbnailTop + (workspaceHeight - thumbnailBottom));
        // the workspace and the workspacePreviews display animation at the same
        // time.
        Animation previewViewAnimation = new ScaleAnimation(multipleX, 1,
                multipleX, 1, Animation.RELATIVE_TO_SELF, animationFocusX
                        / (float) (thumbnailWidth), Animation.RELATIVE_TO_SELF,
                animationFocusY / (float) thumbnailHeight);
        /* SPRD: bug341567 Monkey 2014-08-08 NullPointerException. @{
        this.getChildAt(mWorkspace.getCurrentScreen()).setVisibility(View.GONE);*/
        View view = this.getChildAt(mWorkspace.getCurrentScreen());
        if(view != null) {
            view.setVisibility(View.GONE);
        }else{
            Log.w(TAG,"Previews.displayPreviewsAnimation()  the view is null,maybe currentScreen is "+mWorkspace.getCurrentScreen());
        }
        /* SPRD: bug341567 Monkey 2014-08-08 NullPointerException. @{*/
        previewViewAnimation.setInterpolator(new DecelerateInterpolator());
        this.startAnimation(previewViewAnimation);
        Animation workspaceScaleAnimation = new ScaleAnimation(1,
                1 / multipleX, 1, 1 / multipleX, Animation.RELATIVE_TO_SELF,
                animationFocusX / (float) (thumbnailWidth),
                Animation.RELATIVE_TO_SELF, animationFocusY
                        / (float) thumbnailHeight);
        workspaceScaleAnimation.setDuration(mAnimationDuration);
        workspaceScaleAnimation.setInterpolator(new DecelerateInterpolator());
        mWorkspace.getChildAt(mWorkspace.getCurrentScreen()).startAnimation(
                workspaceScaleAnimation);
        previewViewAnimation
                .setAnimationListener(new ShowPreviewsAnimationListener(
                        workspaceCurrentScreen));
    }

    /**
     * Initialization and display toolbarview away animation.
     */
    private void displayToolBarViewDownAnim() {
        Animation translateAnimation = new TranslateAnimation(
                Animation.ABSOLUTE, 0.0f, Animation.ABSOLUTE, 0.0f,
                Animation.RELATIVE_TO_SELF, 0.0f, Animation.RELATIVE_TO_SELF,
                1.0f);
        translateAnimation.setDuration(mAnimationDuration);
        translateAnimation.setFillAfter(true);
        //mLauncher.getmToolBarView().startAnimation(translateAnimation);
    }

    /**
     * Initialize the toolbarview display animation.
     */
    private void displayToolBarViewUpAnim() {
        Animation translateAnimation = new TranslateAnimation(
                Animation.ABSOLUTE, 0.0f, Animation.ABSOLUTE, 0.0f,
                Animation.RELATIVE_TO_SELF, 1.0f, Animation.RELATIVE_TO_SELF,
                0.0f);
        translateAnimation.setDuration(mAnimationDuration);
       // mLauncher.getmToolBarView().startAnimation(translateAnimation);
    }

    /**
     * To initialize the previews of workspace
     * @return
     */
    private void initWorkspacePreviewItems() {
        /* SPRD: fix bug325904, remove all child views from Previews. @{ */
        if (getChildCount() > 0) {
            removeAllViews();
        }
        /* @} */
        int count = mWorkspace.getChildCount();
        setVisibility(View.VISIBLE);
        /* SPRD: Fix bug 263766 , search view intercepted the touch Event of mine. @{ */
        bringToFront();
        /* @} */
        setFocusable(true);
        for (int i = 0; i < count + 1; i++) {
            initPreviewsItem(i);
        }
    }
    private void initPreviewsItem(int index) {
        LayoutInflater factory = LayoutInflater.from(getContext());
        PreviewsBaseItem child = null;
        if (index < mWorkspace.getChildCount()) {
            child = (PreviewsBaseItem) factory.inflate(
                    R.layout.previews_default_item, null);
        /* SPRD: Fix bug261296,do not show preview_add_item while reached MAX_PAGE. @{ */
        } else if (index == mWorkspace.getChildCount() && index < SprdWorkspace.MAX_PAGE) {
        /* @} */
            child = (PreviewsBaseItem) factory.inflate(
                    R.layout.previews_add_item, null);
        }
        if (child != null) {
            child.mId = getChildCount();
            child.setLauncher(mLauncher);
            child.setWorkspace(mWorkspace);
            child.setPreviews(this);
            child.setOnLongClickListener(this);
            child.initPreviewsItem(index);
            addView(child);
        }
    }

    private PreviewsBaseItem findChildByIndex(int index) {
        int count = getChildCount();
        for (int i = 0; i < count; i++) {
            PreviewsBaseItem child = (PreviewsBaseItem) getChildAt(i);
            if (index == child.mIndex) {
                return child;
            }
        }
        return null;
    }

    private void deleteDragView(){
        PreviewsBaseItem child = (PreviewsBaseItem) mDragView;
        if ((child != null) && (child instanceof PreviewsDefaultItem))
        {
            this.removeView(mDragView);
            int id = child.mId;
            int removeIndex = child.mIndex;
            removeView(child);
         // SPRD : fix bug202233 cellLayout not removed from database
            mWorkspace.deleteCellLayoutFromDatabase(id);
            mWorkspace.deleteCellLayout(id);
            int count = getChildCount();

            for (int i = 0; i < count; i++) {
                PreviewsBaseItem view = (PreviewsBaseItem) getChildAt(i);
                if (view.mIndex > removeIndex) {
                    view.mIndex = view.mIndex - 1;
                }
                if (view.mId > id) {
                    view.mId = view.mId - 1;
                }
                if (count <= MIN_ITEMS_NUM
                        && view instanceof PreviewsDefaultItem) {
                    ((PreviewsDefaultItem) view).showCloseView(false);
                }
            }
            updateCurScreenAndDefScreen(child);
            // when the child count is 9,delete a child then add the button of
            // add.
            if (mWorkspace.getChildCount() == getChildCount()
                    && getChildCount() < MAX_ITEMS_NUM) {
                initPreviewsItem(getChildCount());
            }
            mIsSort = true;
            initCurrentIndexs();
            initAnimate();
        }
    }

    protected void deleteChild(PreviewsBaseItem child) {
        int index = child.mIndex;
        int id = child.mId;
        /* SPRD: the min page count should more than two @{ */
        /* SPRD: Fix bug262687, we'd better user the child count of Workspace to ensure screen count. @{ */
        int count = mWorkspace.getChildCount();
        /* @} */
        if (count < SprdWorkspace.MIN_PAGE + 1) {
            Toast.makeText(mLauncher, R.string.to_min_page, Toast.LENGTH_SHORT)
                    .show();
            return;
        }
        /* @} */
        removeView(child);
        // SPRD : fix bug202233 cellLayout not removed from database
        /* SPRD: Fix bug 262879, delete the right screen. @{ */
        mWorkspace.deleteCellLayout(id);
        /* @} */
        count = getChildCount();
        for (int i = 0; i < count; i++) {
            PreviewsBaseItem view = (PreviewsBaseItem) getChildAt(i);
            if (view.mIndex > index) {
                view.mIndex = view.mIndex - 1;
            }
            if (view.mId > id) {
                view.mId = view.mId - 1;
            }
            /* SPRD: Fix bug262687, the min count of previewItem should equals Workspacce.MIN_PAGE + 1. @{ */
            if(count <= MIN_ITEMS_NUM + 1 && view instanceof PreviewsDefaultItem){
            /* @} */
                ((PreviewsDefaultItem)view).showCloseView(false);
            }
        }
        updateCurScreenAndDefScreen(child);
        //when the child count is 9,delete a child then add the button of add.
        if (mWorkspace.getChildCount() == getChildCount()
                && getChildCount() < MAX_ITEMS_NUM) {
            initPreviewsItem(getChildCount());
        }
        mIsSort = true;
        initCurrentIndexs();
        initAnimate();
    }

    private void updateCurScreenAndDefScreen(PreviewsBaseItem child){
        final int count = getChildCount();
        if (child.mIsCurrentScreen) {
            PreviewsDefaultItem view = (PreviewsDefaultItem) findChildByIndex(0);
            if (view != null) {
                view.setCurrentScreenColor();
                mWorkspace.updateCurrentScreen(0);
            }
        }else{
            for(int i=0;i<count;i++){
                PreviewsBaseItem view = (PreviewsBaseItem)getChildAt(i);
                if(view.mIsCurrentScreen){
                    mWorkspace.updateCurrentScreen(view.mIndex);
                }
            }
        }
       if (child.mIsHomeScreen) {
            PreviewsDefaultItem view = (PreviewsDefaultItem) findChildByIndex(0);
            if (view != null) {
                view.setDefaultScreenColor();
                mWorkspace.updateDefaultScreenNum(0);
            }
       }else{
            for(int i=0;i<count;i++){
                PreviewsBaseItem view = (PreviewsBaseItem)getChildAt(i);
                if(view.mIsHomeScreen){
                    mWorkspace.updateDefaultScreenNum(view.mIndex);
                }
            }
       }
    }

    protected void addChild() {
        this.removeViewAt(getChildCount() - 1);
        /* SPRD: add for UUI empty screens @{ */
        long newId = LauncherAppState.getLauncherProvider().generateNewScreenId();
        mWorkspace.insertNewWorkspaceScreenWithDb(newId);
        /* @} */
        int count = mWorkspace.getChildCount();
        initPreviewsItem(count-1);
        initPreviewsItem(count);
        /* SPRD: Fix bug262687,update CloseView of previewItem after add an item. @{ */
        for (int i = 0; i < getChildCount(); i++) {
            PreviewsBaseItem view = (PreviewsBaseItem)getChildAt(i);
            if (view instanceof PreviewsDefaultItem) {
                ((PreviewsDefaultItem)view).showCloseView(true);
            }
        }
        /* @} */
        initCurrentIndexs();
        initAnimate();
    }

    /**
     * start show previews
     */
    public void showPreviews() {
        mLauncher.getDragLayer().clearAllResizeFrames();
        dissmissed = false;
        initWorkspacePreviewItems();
        initAnimate();
        displayPreviewsAnimation(mWorkspace.getCurrentScreen());
        mLauncher.getSearchBar().hideSearchBar(true);
        mLauncher.hidePageIndicator(true);
        mLauncher.hideHotseat(true);
    }

    /**
     * dismiss previews
     * @param workspaceCurrentScreen
     *            the currentScreen of workspace
     * @param child
     *            the child of workspacePreviews
     */
    public void dismissPreview(int workspaceCurrentScreen, View child) {
        dissmissed = true;
        displayDismissAnimation(workspaceCurrentScreen, child);
        mLauncher.getSearchBar().showSearchBar(true);
        mLauncher.hidePageIndicator(false);
        mLauncher.showHotseat(true);
        mAnimate.clear();
        snapToScreen(curScreen); // SPRD:fix bug 278620,the previews displayed abnormal when lock the screen.
    }

    /**
     *
     */
    public void dismissPreview() {
        for (int i = 0; i < getChildCount(); i++) {
            PreviewsBaseItem child = (PreviewsBaseItem) getChildAt(i);

            if (child instanceof PreviewsDefaultItem) {
                dismissPreview(child.mIndex, child);
            }
        }
    }

    public boolean dissmissed(){
        return dissmissed;
    }
    private Context mContext;
    private boolean dissmissed = true;

    private int curScreen = 0; // 当前屏幕

    private Scroller mScroller = null;
//    private static final int TOUCH_STATE_REST = 0;
    private static final int TOUCH_STATE_SCROLLING = 5;
//    private int mTouchState = TOUCH_STATE_REST;
    public static int SNAP_VELOCITY = 600;
    private int mTouchSlop = 0;
    private float mLastMotionX = 0;
    private VelocityTracker mVelocityTracker = null;
    // startScroll 滑到下一屏
    public void startMove() {
        curScreen++;
        // 3s
        mScroller.startScroll((curScreen - 1) * getWidth(), 0, getWidth(), 0, 3000);
        invalidate();
    }

    //停止滑动
    public void stopMove() {
        if (mScroller != null) {
            // 如果正在滑动，则停止，并直接滑到目标屏
            if (!mScroller.isFinished()) {
                int scrollCurX = mScroller.getCurrX();
                // 计算目标屏
                int descScreen = (scrollCurX + getWidth() / 2) / getWidth();
                mScroller.abortAnimation();
                //立即滑到目标屏
                scrollTo(descScreen * getWidth(), 0);
                mScroller.forceFinished(true);
                curScreen = descScreen;
            }
        }
    }

    // ֻ执行startScroll后,控制滑动流程，
    @Override
    public void computeScroll() {
        // 根据滑动开始时间获取当前坐标，并返回是否仍处于滑动状态
        if (mScroller.computeScrollOffset()) {
            // 按计划滑动
            scrollTo(mScroller.getCurrX(), mScroller.getCurrY());
            postInvalidate();
        }
    }
    // 缓慢滑屏的处理逻辑
    private void snapToDestination() {
        int destScreen = (getScrollX() + mWindowWidth / 2) / mWindowWidth;
        snapToScreen(destScreen);
    }

    private void snapToScreen(int whichScreen) {
        curScreen = whichScreen;
        int totalPages = mWorkspace.getChildCount() / MAX_ITEMS_NUM_PER_SCREEN;
        if (curScreen > totalPages)
            curScreen = totalPages;
        int dx = curScreen * mWindowWidth - getScrollX();
        mScroller.startScroll(getScrollX(), 0, dx, 0, Math.abs(dx) * 2);
        invalidate();
    }
    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        if (mLauncher.isWorkspaceVisible()) {
            return false;
        }

        final int action = ev.getAction();
        if ((action == MotionEvent.ACTION_MOVE)
                && (mTouchState != TOUCH_STATE_REST)) {
            return true;
        }
        final float x = ev.getX();
        switch (action & MotionEvent.ACTION_MASK) {
        case MotionEvent.ACTION_MOVE:
            final int xDiff = (int) Math.abs(mLastMotionX - x);
            // 是滑动,不是点击
            if (xDiff > mTouchSlop) {
                mTouchState = TOUCH_STATE_SCROLLING;
            }
            break;
        case MotionEvent.ACTION_DOWN:
            mLastMotionX = x;
            mTouchState = mScroller.isFinished() ? TOUCH_STATE_REST : TOUCH_STATE_SCROLLING;
            break;
        case MotionEvent.ACTION_POINTER_DOWN:

            mTouchState = TOUCH_STATE_MULTI;
            mMultiTouchStart = spacing(ev);
            mFirstMotionX1 = ev.getX(0);
            mFirstMotionX2 = ev.getX(1);
            mFirstMotionY1 = ev.getY(0);
            mFirstMotionY2 = ev.getY(1);
            break;
        case MotionEvent.ACTION_POINTER_UP:
        case MotionEvent.ACTION_CANCEL:
            mTouchState = TOUCH_STATE_REST;
            break;

        }
        return mTouchState != TOUCH_STATE_REST;
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        if (mLauncher.isWorkspaceVisible()) {
            return false;
        }
        final int action = ev.getAction();
        if (mVelocityTracker == null) {
            mVelocityTracker = VelocityTracker.obtain();
        }
        mVelocityTracker.addMovement(ev);
        float x = ev.getX();
        switch (action & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_POINTER_UP:


                if (mTouchState == TOUCH_STATE_MULTI) {
                    mMultiTouchStop = spacing(ev);
                    multiTouch();
                    mTouchState = TOUCH_STATE_REST;
                }
                break;
            case MotionEvent.ACTION_UP:
                final VelocityTracker velocityTracker = mVelocityTracker;
                velocityTracker.computeCurrentVelocity(1000);
                int velocityX = (int) velocityTracker.getXVelocity();
                // 快速右滑,不管滑动是否过半屏，都直接切上一屏（如果有）
                if (velocityX > SNAP_VELOCITY && curScreen > 0) {
                    snapToScreen(curScreen - 1);
                }
                // 快速左滑,不管滑动是否过半屏，都直接切下一屏（如果有）
                else if (velocityX < -SNAP_VELOCITY && curScreen < (getChildCount() - 1)) {
                    snapToScreen(curScreen + 1);
                }
                // 缓慢滑动,上屏/下屏或者停在当前屏取决于是否滑过半
                else {
                    snapToDestination();
                }
                //及时回收
                if (mVelocityTracker != null) {
                    mVelocityTracker.recycle();
                    mVelocityTracker = null;
                }
                mTouchState = TOUCH_STATE_REST;
                break;
            case MotionEvent.ACTION_MOVE:
                int detaX = (int) (mLastMotionX - x);
                //随手指滑动
                scrollBy(detaX, 0);
                mLastMotionX = x;
                break;
            case MotionEvent.ACTION_DOWN: {
             // 如果正在滑动时按下，则停止滑动,准备开始新一轮的move
                if (mScroller != null) {
                    if (!mScroller.isFinished()) {
                        mScroller.abortAnimation();
                    }
                }
                mLastMotionX = x;
//                for (int index = 0; index < getChildCount(); index++) {
//                    PreviewsBaseItem child = (PreviewsBaseItem) getChildAt(index);
//                    if (child.mIsCurrentScreen) {
//                        dismissPreview(index, child);
//                        break;
//                    }
//                }
            }
                break;
            case MotionEvent.ACTION_CANCEL:
                mTouchState = TOUCH_STATE_REST;
                break;

        }
        return true;
    }

    /**
     * Determine the space between the first two fingers
     */
    private float spacing(MotionEvent event) {
        float x = event.getX(0) - event.getX(1);
        float y = event.getY(0) - event.getY(1);
        return FloatMath.sqrt(x * x + y * y);
    }

    private void multiTouch() {
        if ((mMultiTouchStop - mMultiTouchStart) > 5.0f
                && !mLauncher.isWorkspaceVisible()) {
            for (int i = 0; i < getChildCount(); i++) {
                PreviewsBaseItem child = (PreviewsBaseItem) getChildAt(i);
                if (mFirstMotionX1 > child.getLeft()
                        && mFirstMotionX1 < child.getRight()
                        && mFirstMotionX2 > child.getLeft()
                        && mFirstMotionX2 < child.getRight()
                        && mFirstMotionY1 > child.getTop()
                        && mFirstMotionY1 < child.getBottom()
                        && mFirstMotionY2 > child.getTop()
                        && mFirstMotionY2 < child.getBottom()) {
                    if (child instanceof PreviewsDefaultItem) {
                        dismissPreview(child.mIndex, child);
                        break;
                    }
                }
            }
        }
    }

    /**
     * the listener of previewsAnimation
     */
    class ShowPreviewsAnimationListener implements AnimationListener {
        private int workspaceCurrentScreen;

        public ShowPreviewsAnimationListener(int workspaceCurrentScreen) {
            this.workspaceCurrentScreen = workspaceCurrentScreen;
        }

        public void onAnimationStart(Animation arg0) {
            displayToolBarViewDownAnim();
       //     mLauncher.getmScreenNumView().setVisibility(View.GONE);
      //      mWorkspace.cancelAllScreenOwnerWidgetAnim();
        }

        public void onAnimationRepeat(Animation arg0) {
        }

        public void onAnimationEnd(Animation arg0) {
            mWorkspace.setVisibility(View.GONE);
       //     mLauncher.getmToolBarView().setVisibility(View.GONE);
            if(workspaceCurrentScreen >=0 && workspaceCurrentScreen < Previews.this.getChildCount()){
                Previews.this.getChildAt(workspaceCurrentScreen).setVisibility(View.VISIBLE);
            }
            View v = mWorkspace.getChildAt(workspaceCurrentScreen);
            if(v != null){
                v.clearAnimation();
            }
            Previews.this.clearAnimation();
        }
    }

    /**
     * the listener of DismissPreviewsAnimation
     */
    class DisMissPreviewsAnimationListener implements AnimationListener {
        private int workspaceCurrentScreen;

        public DisMissPreviewsAnimationListener(int workspaceCurrentScreen) {
            this.workspaceCurrentScreen = workspaceCurrentScreen;
        }

        public void onAnimationStart(Animation arg0) {
            displayToolBarViewUpAnim();
        }

        public void onAnimationRepeat(Animation arg0) {
        }

        public void onAnimationEnd(Animation arg0) {
            Previews.this.setVisibility(View.GONE);
            mWorkspace.snapToPage(workspaceCurrentScreen);
            mWorkspace.setVisibility(View.VISIBLE);
         //   mLauncher.getmScreenNumView().setVisibility(View.VISIBLE);
         //   mLauncher.getmToolBarView().setVisibility(View.VISIBLE);
         //   mWorkspace.updateOwnerWidget(workspaceCurrentScreen, false);
            /** view null bug 151977 start**/
            View view = mWorkspace.getChildAt(workspaceCurrentScreen);
            if(view != null){
                view.clearAnimation();
            }
            /** view null bug 151977 end**/
            mDragController.removeDropTarget(Previews.this);
            Previews.this.clearAnimation();
            for (int i = 0; i < getChildCount(); i++) {
                PreviewsBaseItem child = (PreviewsBaseItem) getChildAt(i);
                child.recycleThumbnailBitmap();
            }

            /* SPRD: bug321331 2014-06-10 don't hold memory after previews disappear. @{ */
            Previews.this.removeAllViews();
            /* SPRD: bug321331 2014-06-10 don't hold memory after previews disappear. @} */
        }
    }

    public void setDragController(DragController dragger) {
        mDragController = dragger;
    }

    public Rect estimateDropLocation(DragSource source, int x, int y,
            int xOffset, int yOffset, DragView dragView, Object dragInfo,
            Rect recycle) {
        return null;
    }

    private int getLocationIndex(int x,int y){
        int index =  0;
        x -= curScreen * mLauncher.getWindowManager().getDefaultDisplay().getWidth();
        x -= this.mMargin;
        float previewsWidth = this.getWidth();
        float previewsHeight= this.getHeight();
        float everyRectWidth  = mPreviewWidth;
        float everyRectHeight  = mPreviewHeight;
        int count = getChildCount();
        for (int i=0; i < mHorNum; i++) {
            float left = i*everyRectWidth;
            float right = left+everyRectWidth;

            if(x>=left && x< right){
                for(int j=0;j<mHorNum;j++){
                    float top = j*everyRectHeight;
                    float bottom = top+everyRectHeight;
                    if(y>=top && y< bottom){
                        index = j*mHorNum+i;
                        break;
                    }
                }
            }
        }
        if (index >= count - 1) {
            return count - 1;
        } else {
            return index + curScreen * MAX_ITEMS_NUM_PER_SCREEN;
        }
    }

    private int mDragViewIndex;
    private PreviewsBaseItem mDragView;
    private ArrayList<Integer> mCurrentIndexsList;

    private void initCurrentIndexs() {
        mCurrentIndexsList = new ArrayList<Integer>();
        int count = getChildCount();
        for (int i = 0; i < count; i++) {
            for (int j = 0; j < count; j++) {
                PreviewsBaseItem child = (PreviewsBaseItem) getChildAt(j);
                if (child.mIndex == i) {
                    mCurrentIndexsList.add(child.mId);
                }
            }
        }
        if (getChildAt(getChildCount() - 1) instanceof PreviewsAddItem) {
            mCurrentIndexsList.remove(mCurrentIndexsList.size() - 1);
        }
    }

    private void sortChildren(int upIndex, int downIndex) {
        if (upIndex == downIndex || upIndex > getChildCount() - 1
                || downIndex > getChildCount() - 1) {
            mIsAnimEnd = true;
            return;
        }
        int count = getChildCount();
        boolean hasStartAni = false;
        for (int i = 0; i < count; i++) {
            PreviewsBaseItem child = (PreviewsBaseItem) getChildAt(i);
            if (upIndex > downIndex) {
                if (child.mIndex >= downIndex && child.mIndex < upIndex
                        && child != mDragView) {
                    mAnimate.get(child.mIndex).start(child,
                            getRectByIndex(child.mIndex),
                            getRectByIndex(child.mIndex + 1));
                    child.mIndex = child.mIndex + 1;
                    hasStartAni = true;
                }
            } else if (upIndex < downIndex) {
                if (child.mIndex > upIndex && child.mIndex <= downIndex
                        && child != mDragView) {
                    mAnimate.get(child.mIndex).start(child,
                            getRectByIndex(child.mIndex),
                            getRectByIndex(child.mIndex - 1));
                    child.mIndex = child.mIndex - 1;
                    hasStartAni = true;
                }
            }
        }
        mDragView.mIndex = downIndex;
        if(!hasStartAni){
            mIsAnimEnd = true;
        }
    }

    private Rect getRectByIndex(int index) {
        Rect tempRect = new Rect();
        ItemLocation location = getItemLocationByIndex(index);
        tempRect.set(location.mLeft, location.mTop, location.mRight,
                location.mBottom);
        return tempRect;
    }
/*
    public boolean onDrop(DragSource source, int x, int y, int xOffset,
            int yOffset, DragView dragView, Object dragInfo) {
        drop();
        return true;
    }
*/
    protected void drop(){
        IS_OUT_DRAGING = false;
        int count = getChildCount();
        for (int i = 0; i < count; i++) {
            PreviewsBaseItem child = (PreviewsBaseItem) getChildAt(i);
            if (child instanceof PreviewsAddItem
                    && child.mIndex < getChildCount() - 1) {
                removeView(child);
                mWorkspace.addCellLayout();
                initPreviewsItem(child.mIndex);
                initPreviewsItem(mWorkspace.getChildCount());
                if(count == MIN_ITEMS_NUM){
                    for(int j=0;j<MIN_ITEMS_NUM;j++){
                        PreviewsBaseItem view = (PreviewsBaseItem)getChildAt(j);
                        ((PreviewsDefaultItem)view).showCloseView(true);
                    }
                }
                break;
            }
        }
        initCurrentIndexs();
    }

    public boolean onLongClick(View v) {
        if (!v.isInTouchMode() || mLauncher.isWorkspaceLocked() || !mClickable) {
            return false;
        }
        if(v instanceof PreviewsAddItem){
//            v.setBackgroundDrawable(getResources().getDrawable(R.drawable.preview_add_pressed_bg));
            //fix bug204036 on 2013/8/20 AddItem should not be dragged start
            return false;
            //fix bug204036 on 2013/8/20 AddItem should not be dragged end
        }
        mDragView = (PreviewsBaseItem) v;
        mDragView.setVisibility(View.GONE);
        mDragViewIndex = mDragView.mIndex;
        mDragController.addDropTarget(Previews.this);
        mDragController.startDrag(v, this, null,
                DragController.DRAG_ACTION_MOVE, null);
        return true;
    }

    class ItemLocation {
        int mLeft;
        int mRight;
        int mTop;
        int mBottom;
    }

    public void setLauncher(Launcher launcher) {
        this.mLauncher = launcher;
        mWindowWidth = mLauncher.getWindowManager().getDefaultDisplay().getWidth();
        /* SPRD: Bug 271612 @{ */
        mWindowHeight = mLauncher.getWindowManager().getDefaultDisplay().getHeight();
        /* @} */
    }

    public void setWorkspace(SprdWorkspace workspace) {
        this.mWorkspace = workspace;
//        initAnimate();
    }

    class Animate implements Runnable {

        private Scroller mScroller;

        private View mTargetView;

        Animate() {
            DecelerateInterpolator mDecelerateInterpolator = new DecelerateInterpolator();
            this.mScroller = new Scroller(getContext(), mDecelerateInterpolator);
        }

        public void run() {
            boolean isEnd = !mScroller.computeScrollOffset();
            int x = mScroller.getCurrX();
            int y = mScroller.getCurrY();
            final View mDrawView = this.mTargetView;
            int k = mDrawView.getWidth() + x;
            int l = mDrawView.getHeight() + y;
            mDrawView.layout(x, y, k, l);
            Previews.this.postInvalidate();
            if (isEnd) {
                mIsAnimEnd = true;
                relayoutChilds();
                return;
            } else {
                Previews.this.post(this);
            }
        }

        public void start(View targetView, Rect sourceRect, Rect targetRect) {
            stop();
            this.mTargetView = targetView;
            int left = sourceRect.left;
            int top = sourceRect.top;
            int newLeft = targetRect.left;
            int newTop = targetRect.top;
            int moveX = newLeft - left;
            int moveY = newTop - top;
            this.mScroller.startScroll(left, top, moveX, moveY, 150);
            Previews.this.post(this);
        }

        public void stop() {
            Previews.this.removeCallbacks(this);
        }

    }

    @Override
    public boolean isDropEnabled() {
        return true;
    }

    private static int DELETE_ANIMATION_DURATION = 85;
    @Override
    public void onDrop(DragObject dragObject) {
         //drop();
        final Rect from = new Rect();
        final DragLayer dragLayer = mLauncher.getDragLayer();
        dragLayer.getViewRectRelativeToSelf(dragObject.dragView, from);
        final float scale = 1.0f;
        dragLayer.animateView(dragObject.dragView, from, from, scale, 1f, 1f, 0.9f, 0.9f,
                DELETE_ANIMATION_DURATION, new DecelerateInterpolator(2),
                new LinearInterpolator(), null, DragLayer.ANIMATION_END_DISAPPEAR, null);
        sortChilds();
    }

    @Override
    public void onDragEnter(DragObject dragObject) {
    }

    @Override
    public void onDragOver(DragObject dragObject) {
        if(mIsAnimEnd){
            int currentIndex = getLocationIndex(dragObject.x, dragObject.y);
            // SPRD: fx260911
            if (currentIndex >= getChildCount() - 1) {
                return;
            }
            if (mDragViewIndex != currentIndex) {
                mIsAnimEnd = false;
                mIsSort = true;
                mDragView.mIndex = currentIndex;
                sortChildren(mDragViewIndex, currentIndex);
                mDragViewIndex = currentIndex;
            }
        }
    }

    @Override
    public void getHitRect(Rect outRect) {
//	    outRect.left = 0;
//	    outRect.right = this.getWidth()-outRect.left;
//	    outRect.top = 0;
//	    outRect.bottom = this.getHeight()-outRect.top;
        outRect.set(0, 0, this.getWidth(), this.getHeight());
    }



    @Override
    public void onDragExit(DragObject dragObject) {
        if(mDragView instanceof PreviewsAddItem){
            mDragView.setBackgroundDrawable(getResources().getDrawable(R.drawable.preview_add_background));
        }
    }

/*    @Override
    public DropTarget getDropTargetDelegate(DragObject dragObject) {
        return null;
    }*/

    @Override
    public boolean acceptDrop(DragObject dragObject) {
        return true;
    }

    @Override
    public void getLocationInDragLayer(int[] loc) {
//	    if(loc != null){
//	        loc[0] = 0;
//	        loc[1] = 0;
//	    }
        mLauncher.getDragLayer().getLocationInDragLayer(this, loc);
    }

    private void sortChilds(){
        int count = getChildCount();
        for (int i = 0; i < count; i++) {
            PreviewsBaseItem child = (PreviewsBaseItem) getChildAt(i);
            // add preview is in front
            if (child instanceof PreviewsAddItem
                    && child.mIndex < getChildCount() - 1) {
                // remove add
                removeView(child);
                int removeIndex = child.mIndex;
                int id = child.mId;
                int viewsCount = getChildCount();
                for (int j = 0; j < viewsCount; j++) {
                    PreviewsBaseItem view = (PreviewsBaseItem) getChildAt(j);
                    if (view != null) {
                        if (view.mIndex > removeIndex) {
                            view.mIndex = view.mIndex - 1;
                        }
                        if (view.mId > id) {
                            view.mId = view.mId - 1;
                        }
                    }
                }
                initPreviewsItem(mWorkspace.getChildCount());
                relayoutChilds();
                break;
            }
        }
        initCurrentIndexs();
    }

    private void relayoutChilds(){
        this.requestLayout();
        this.invalidate();
    }

    @Override
    public void onDropCompleted(View target, DragObject dragObject,boolean isFlingToDelete, boolean success) {
        mDragController.removeDropTarget(this);
        mDragView.setVisibility(View.VISIBLE);
        if(target != null)
        {
            boolean shouldDelete = (getChildCount() > MIN_ITEMS_NUM) && (mDragView instanceof PreviewsDefaultItem);
            if (target instanceof Workspace || !shouldDelete){
                sortChilds();
            }
            else if (target instanceof DeleteDropTarget)
            {
                PreviewsBaseItem child = (PreviewsBaseItem) mDragView;
                int mId = child.mId;
                CellLayout cell = (CellLayout) mWorkspace.getChildAt(mId);
//                CellLayoutChildren cellChild = (CellLayoutChildren) cell.getChildAt(0);
                if (cell.getChildCount() == 0) {
                    deleteDragView();
                }
                else {
                    sortChilds();
                    AlertDialog.Builder builder = new Builder(mContext);
                    builder.setMessage(mContext.getString(R.string.delete_preview_tip));
                    builder.setTitle(mContext.getString(R.string.delete_preview_alter));
                    builder.setPositiveButton(
                            mContext.getString(R.string.delete_preview_action),
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    deleteDragView();
                                    dialog.dismiss();
                                }
                            });
                    builder.setNegativeButton(mContext.getString(R.string.cancel_action),
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    dialog.dismiss();
                                }
                            });
                    builder.create().show();
                }
            }
        }
    }

    @Override
    public void onFlingToDelete(DragObject dragObject, int x, int y, PointF vec) {
        // TODO Auto-generated method stub

    }

    @Override
    public void getHitRectRelativeToDragLayer(Rect outRect) {
        // TODO Auto-generated method stub
        mLauncher.getDragLayer().getDescendantRectRelativeToSelf(this, outRect);
    }

    @Override
    public boolean supportsFlingToDelete() {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public void onFlingToDeleteCompleted() {
        // TODO Auto-generated method stub

    }
}
