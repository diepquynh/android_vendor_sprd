package com.android.sprdlauncher2;

import java.util.ArrayList;

import com.android.sprdlauncher2.Workspace.State;
import com.sprd.sprdlauncher2.OwnerWidgetInfo;
import com.sprd.sprdlauncher2.WidgetUpdate;
import com.sprd.sprdlauncher2.effect.EffectFactory;
import com.sprd.sprdlauncher2.effect.EffectInfo;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.graphics.Bitmap;
import android.graphics.Camera;
import android.graphics.Matrix;
import android.util.AttributeSet;
import android.util.FloatMath;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.MeasureSpec;
import android.view.View.OnClickListener;
import android.view.animation.Animation;
import android.view.animation.Transformation;

public class SprdWorkspace extends Workspace {
    private static final String TAG = "Launcher.SprdWorkspace";
    private boolean allowEmptyScreen = true;

    /* SPRD: Feature 255891,Porting from SprdLauncher(Android4.1). @{ */
    public static final int ANIMATION_DEFAULT = 0;
    private static float CAMERA_DISTANCE = 6500;
    private EffectInfo mCurentAnimInfo;
    // Camera and Matrix used to determine the final position of a neighboring CellLayout
    private final Matrix mMatrix = new Matrix();
    private final Camera mCamera = new Camera();
    private final float mTempFloat2[] = new float[2];
    protected float mOverScrollMaxBackgroundAlpha = 0.0f;
    protected int mOverScrollPageIndex = -1;
    // Y rotation to apply to the workspace screens
    private static final float WORKSPACE_ROTATION = 12.5f;
    /* @} */

  //SPRD: Feature 251656, Remove the application drawer view
    // SPRD: Fix bug 321525, there is no need to set the visibility of dragSourceView.
    //private View mDragSourceView;
    /**
     * Used to inflate the Workspace from XML.
     *
     * @param context
     *            The application's context.
     * @param attrs
     *            The attributes set containing the Workspace's customization
     *            values.
     */
    public SprdWorkspace(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    /**
     * Used to inflate the Workspace from XML.
     *
     * @param context
     *            The application's context.
     * @param attrs
     *            The attributes set containing the Workspace's customization
     *            values.
     * @param defStyle
     *            Unused.
     */
    public SprdWorkspace(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    public void onDragStart(final DragSource source, Object info, int dragAction) {
        /* SPRD: add for UUI previews @{ */
        if (mLauncher.isWorkspacePreviewsVisible()) {
            return;
        }
        /* @} */
        super.onDragStart(source, info, dragAction);
    }

    @Override
    public void onDragEnd() {
        super.onDragEnd();
        /*
         * SPRD: bug258558 2014-01-23 new requirement to allow delete in edit
         * mode @{
         */
        updateChildrenDeletable(true /* allow show delete image */);
        /*
         * SPRD: bug258558 2014-01-23 new requirement to allow delete in edit
         * mode @}
         */

        /* SPRD: bug316052 2014-06-05 OOM problem optimize. @{ */
        // SPRD: Fix bug 321525, there is no need to set the visibility of dragSourceView.
        //mDragSourceView = null;
        /* SPRD: bug316052 2014-06-05 OOM problem optimize. @} */
    }

    @Override
    protected void initWorkspace() {
        // SPRD: add for UUI previews
        initDefaultPage();
        super.initWorkspace();
        /* SPRD: Feature 255891,Porting from SprdLauncher(Android4.1). @{ */
        initAnimationStyle();
        /* @} */
    }

    /*
     * SPRD: add for UUI : save the empty screen to db, so it won't disappear
     * after reboot @{
     */
    public long insertNewWorkspaceScreenWithDb(long screenId) {
        int insertIndex = mScreenOrder.indexOf(EXTRA_EMPTY_SCREEN_ID);
        if (insertIndex < 0) {
            insertIndex = mScreenOrder.size();
        }
        insertNewWorkspaceScreenBeforeEmptyScreen(screenId);
        mLauncher.insertEmptyScreen(screenId, insertIndex);
        return screenId;
    }

    /* @} */

    @Override
    public void stripEmptyScreens() {
        if (allowEmptyScreen) {
            return;
        }
        super.stripEmptyScreens();
    }

    /**
     * Called directly from a CellLayout (not by the framework), after we've been added as a
     * listener via setOnInterceptTouchEventListener(). This allows us to tell the CellLayout
     * that it should intercept touch events, which is not something that is normally supported.
     */
    @Override
    public boolean onTouch(View v, MotionEvent event) {
        /*SPRD: Feature 253075,icon can drag in edit mode@{ */
        return (isSmall() || mIsSwitchingState);
        /* @} */
    }

    @Override
    protected void onPageEndMoving() {
        super.onPageEndMoving();
        /* SPRD: Feature 255891,porting from SprdLauncher(Android4.1). @{ */
        mOverScrollMaxBackgroundAlpha = 0.0f;
        mOverScrollPageIndex = -1;
        /* @} */
    }

    @Override
    protected void screenScrolled(int screenCenter) {
        /* SPRD: Feature 255891,Porting from SprdLauncher(Android4.1). @{ */
        if (LauncherAppState.getInstance().isScreenLarge()) {
            // We don't call super.screenScrolled() here because we handle the adjacent pages alpha
            // ourselves (for efficiency), and there are no scrolling indicators to update.
            screenScrolledLargeUI(screenCenter);
        } else {
            super.screenScrolled(screenCenter);
            screenScrolledStandardUI(screenCenter);
        }
        /* @} */
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        /* SPRD: Feature 255891,Porting from SprdLauncher(Android4.1). @{ */
        registerPreferenceListenerForWorkspaceStyle();
        /* @} */
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        /* SPRD: Feature 255891,Porting from SprdLauncher(Android4.1). @{ */
        unregisterPreferenceListenerForWorkspaceStyle();
        /* @} */
    }

    @Override
    public boolean isSmall() {
        // SPRD: Feature 253075,icon can drag in edit mode
        return mState == State.SMALL || (mState == State.SPRING_LOADED && !mLauncher.isEditingMode());
    }

    public void beginDragShared(View child, DragSource source) {
        super.beginDragShared(child, source);
     // SPRD: Feature 251656, Remove the application drawer view
        // SPRD: Fix bug 321525, there is no need to set the visibility of dragSourceView.
        //mDragSourceView = child;
    }

    /* SPRD: add for UUI previews @{ */
    private SharedPreferences mCustomizePreference = null;
    public static final int MAX_PAGE = 12;
    public static final int MIN_PAGE = 2;

    public void addCellLayout() {
        final LayoutInflater mInflater = mLauncher.getLayoutInflater();
        View child = mInflater.inflate(R.layout.workspace_screen, null);
        addView(child, getChildCount());
        child.setOnLongClickListener(mLongClickListener);
        updateHomeScreenNum();
    }

    private void updateHomeScreenNum() {
        updateCurrentScreen(mCurrentPage);
    }

    public void updateCurrentScreen(int currentScreen) {
        mCurrentPage = Math
                .max(0, Math.min(currentScreen, getChildCount() - 1));
    }

    public void sortCellLayout(ArrayList<Integer> newOrder) {
        mLauncher.getModel().sortPreviewItems(mLauncher, newOrder);
        ArrayList<CellLayout> oldChildren = new ArrayList<CellLayout>();
        int cellLayoutCount = getChildCount();
        for (int i = 0; i < cellLayoutCount; i++) {
            CellLayout view = (CellLayout) getChildAt(i);
            oldChildren.add(i, view);
        }
        if (oldChildren.size() == newOrder.size()) {
            removeAllViews();
            for (int j = 0; j < newOrder.size(); j++) {
                CellLayout view = oldChildren.get(newOrder.get(j));
                view.updateCellInfoScreen(j);
                int cellChildCount = view.getChildCount();
                for (int t = 0; t < cellChildCount; t++) {
                    View childView = view.getChildAt(t);
                    if (childView != null) {
                        ItemInfo tag = (ItemInfo) childView.getTag();
                        if (tag != null) {
                            tag.screenId = j;
                        }
                    }
                }
                addView(view, j);
            }
        }
    }

    public void updateDefaultScreenNum(int defaultScreenNum) {
        if (mCustomizePreference == null) {
            mCustomizePreference = (SharedPreferences) mLauncher
                    .getSharedPreferences(SprdLauncher.CUSTOMIZEPREF,
                            Context.MODE_PRIVATE);
        }
        Editor editor = mCustomizePreference.edit();
        editor.putInt(SprdLauncher.HOME_DEFAULT_SCREEN_TAG, defaultScreenNum);
        editor.commit();
        mDefaultPage =  Math.max(0, Math.min(defaultScreenNum, getChildCount() - 1));
    }

    public void outPreview(final Animation preAni, int currentScreen) {
        setScreen(currentScreen);
        invalidate();
    }

    void setScreen(int currentScreen) {
        if (!mScroller.isFinished()) mScroller.abortAnimation();
          mCurrentPage = Math.max(0, Math.min(currentScreen, getChildCount() - 1));
          /* SPRD: Bug 271612 @{ */
          // scrollTo(this.getCurrentScreen() * getWidth(), 0);
          scrollTo(this.getCurrentScreen()
                  * mLauncher.getWindowManager().getDefaultDisplay().getWidth(), 0);
          /* @} */
    }

    public int getCurrentScreen() {
        return this.mCurrentPage;
    }

    public void deleteCellLayoutFromDatabase(int index) {
        Log.d(TAG,"deleteCellLayoutFromDatabase");
        // SPRD: only the empty celllayout can be removed
        /*CellLayout cellLayout = (CellLayout) getChildAt(index);
        int childCount = cellLayout.getChildCount();
        for (int j = 0; j < childCount; j++) {
            final View view = cellLayout.getChildAt(j);
            Object tag = view.getTag();

            if (tag instanceof ShortcutInfo) {
                final ShortcutInfo info = (ShortcutInfo) tag;
                LauncherModel.deleteItemFromDatabase(mLauncher, info);
            } else if (tag instanceof FolderInfo) {
                final FolderInfo info = (FolderInfo) tag;
                LauncherModel.deleteFolderContentsFromDatabase(mLauncher, info);
            } else if (tag instanceof LauncherAppWidgetInfo) {
                final LauncherAppWidgetInfo info = (LauncherAppWidgetInfo) tag;
                LauncherModel.deleteItemFromDatabase(mLauncher, info);
            }
        }*/
    }

    public void deleteCellLayout(int index) {
        CellLayout cellLayout = (CellLayout) getChildAt(index);
        /* SPRD: Fix bug 280246 @{ */
        int realScreenCount = 0;
        int screenCount = mScreenOrder.size();
        for (int i = 0; i < screenCount; i++) {
            long screenId = mScreenOrder.get(i);
            if (EXTRA_EMPTY_SCREEN_ID != screenId && CUSTOM_CONTENT_SCREEN_ID != screenId) {
                realScreenCount++;
            }
        }
        if (realScreenCount <= MIN_PAGE || cellLayout.getShortcutsAndWidgets().getChildCount() > 0) {
            Log.i(TAG, "Some where called deleteCellLayout() in wrong condition!");
            return;
        }
        /* @} */
        /* SPRD: fix bug258692 @ { */
       /* for (int i = index + 1; i <= getChildCount(); i++) {
            mLauncher.getModel().reduceItemsScreen(getContext(), i);
        }*/
        /* @} */
        mWorkspaceScreens.remove(mScreenOrder.get(index));
        mLauncher.getModel().removeEmptyScreen(mLauncher, mScreenOrder.get(index));
        mScreenOrder.remove(index);
        /* SPRD: fix bug 267176 @ { */
        mLauncher.getModel().updateWorkspaceScreenOrder(mLauncher, mScreenOrder);
        /* @} */
        removeView(cellLayout);
        updateHomeScreenNum();
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {

        switch (ev.getAction() & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_POINTER_UP:
                mMultiTouchStop = spacing(ev);
                // show previews
                multiTouch();
                mMultiTouchStart = 0;
                break;
        }
        //if two finger moving, not scroll page
        if (mMultiTouchStart != 0) {
            return true;
        }
        return super.onTouchEvent(ev);
    }

    private void multiTouch() {
        // SPRD: add for bug263711
        if ((mMultiTouchStart - mMultiTouchStop) > 35.0f && !mLauncher.isWorkspacePreviewsVisible() && !mLauncher.isEditingMode()) {
            mLauncher.showPreviews();
        }
    }

    /** Determine the length between the first two fingers */
    protected float spacing(MotionEvent event) {
        float x = event.getX(0) - event.getX(1);
        float y = event.getY(0) - event.getY(1);
        return FloatMath.sqrt(x * x + y * y);
    }

    private void initDefaultPage(){
        if (mCustomizePreference == null) {
            mCustomizePreference = (SharedPreferences) mLauncher
                    .getSharedPreferences(SprdLauncher.CUSTOMIZEPREF,
                            Context.MODE_PRIVATE);
        }
        mDefaultPage = mCustomizePreference.getInt(SprdLauncher.HOME_DEFAULT_SCREEN_TAG, mDefaultPage);
    }
    /* SPRD: Feature 251656, Remove the application drawer view @{ */
    /* SPRD: Fix bug 321525, there is no need to set the visibility of dragSourceView.
    public void setDragSourceViewVisible(boolean visible) {
        mDragSourceView.setVisibility(visible ? View.VISIBLE : View.GONE);
    }
    /* @} */

    /* SPRD: bug258558 2014-01-23 new requirement to allow delete in edit mode @{ */
    /**
     * NOTE: we keep at least two CellLayout not be deleted in workspace.
     *
     * @param allow boolean indicator whether this child can be delete or not.
     * if allow is true, then child(CellLayout) will show a delete image in the
     * right corner.
     */
    public void updateChildrenDeletable(boolean allow) {
        if (!mLauncher.isEditingMode()) {
            // currently we always keep invisible if we not in EditMode
            return;
        }

        // we want at least two CellLayout, keep the same logic with Previews
        if ((getChildCount() - EDIT_VIEW_COUNT) <= 2) {
            allow = false;
        }

        CellLayout cl = null;
        for(int i = 0; i < getChildCount(); i++) {
            cl = (CellLayout)getPageAt(i);

            if (cl.mEditAdd) {
                continue;
            }

            if (!allow) {
                cl.setDeleteViewVisibility(false);
            } else {
                cl.setDeleteViewVisibility(
                    (((ViewGroup)cl.getChildAt(0)).getChildCount() == 0));
            }
        }
    }

    public boolean hasPlusCellLayouts() {
        return ((CellLayout)getChildAt(0)).mEditAdd;
    }

    public void addPlusCellLayoutsIfNeeded() {
        if (!mLauncher.isEditingMode()) {
            // currently we always keep invisible if we not in EditMode
            return;
        }

        int childCount = getChildCount();

        // if there are no plus-CellLayout at first or last and the total count
        // less than (MAX_PAGE + 1) than we need to add two plus-CellLayout
        // at the first and the last postion.
        // SPRD : fix bug286062
        if (!hasPlusCellLayouts() && childCount < (MAX_PAGE)) {
            addEditViews(false);

            // TODO we set background alpha here may not be the best implementation

            // we need to set the new added CellLayout's background alpha to 1.0 to let not
            // show transparent.
            ((CellLayout)getChildAt(0)).setBackgroundAlpha(1.0f);
            // after we add two plus-CellLayout, curent count of Workspace is (childCount + 2)
            // so the last CellLayout is (childCount + 1)
            ((CellLayout)getChildAt(childCount + 1)).setBackgroundAlpha(1.0f);
        }
    }

    public void deleteCellLayoutInEditMode(CellLayout cell) {

        /* SPRD: Fix bug 280246 @{ */
        int realScreenCount = 0;
        int screenCount = mScreenOrder.size();
        for (int i = 0; i < screenCount; i++) {
            long screenId = mScreenOrder.get(i);
            if (EXTRA_EMPTY_SCREEN_ID != screenId && CUSTOM_CONTENT_SCREEN_ID != screenId) {
                realScreenCount++;
            }
        }
        if (realScreenCount <= MIN_PAGE || cell.getShortcutsAndWidgets().getChildCount() > 0) {
            Log.i(TAG, "Some where called deleteCellLayoutInEditMode() in wrong condition!");
            return;
        }
        /* @} */

        // here we need the real index of CellLayout in ViewGroup
        // real index may not the same as index in Workspace.mScreenOrder if there has plus-CellLayout
        int index = indexOfChild(cell);

        if (hasPlusCellLayouts()) {
            index--;
        }

        mWorkspaceScreens.remove(mScreenOrder.get(index));
        mLauncher.getModel().removeEmptyScreen(mLauncher, mScreenOrder.get(index));
        mScreenOrder.remove(index);
        mLauncher.getModel().updateWorkspaceScreenOrder(mLauncher, mScreenOrder);
        removeView(cell);
        /* SPRD: Fix bug 274300 @{ */
        if (index < mDefaultPage) {
            updateDefaultScreenNum(mDefaultPage - 1);
        } else if (index == mDefaultPage) {
            updateDefaultScreenNum(0);
        }
        /* @} */

        // TODO we don't need to update the mCurrentPage field because onEditModeEnd will do it for us
        // TODO we currently keep the right CellLayout centered to screen even the right CellLayout is the plus-CellLayout
    }
    /* SPRD: bug258558 2014-01-23 new requirement to allow delete in edit mode @} */

    /* SPRD: Feature 254917,add new view in edit mode@{ */
    public void onEditModeStart() {
        addEditViews(false);
    }

    private void addEditViews(boolean small) {
        if (getChildCount() >= MAX_PAGE) {
            return;
        }
        final LayoutInflater mInflater = mLauncher.getLayoutInflater();
        child = (CellLayout) mInflater.inflate(R.layout.workspace_screen, null);
        child2 = (CellLayout) mInflater.inflate(R.layout.workspace_screen, null);
        child.mEditAdd = true;
        child2.mEditAdd = true;
        if (small) {
            float finalScaleFactor = mSpringLoadedShrinkFactor;
            child.setScaleX(finalScaleFactor);
            child.setScaleY(finalScaleFactor);
            child.setAlpha(1.0f);
            child.setBackgroundAlpha(1.0f);
            child2.setScaleX(finalScaleFactor);
            child2.setScaleY(finalScaleFactor);
            child2.setAlpha(1.0f);
            child2.setBackgroundAlpha(1.0f);
        }
        /* SPRD: fix bug325902, NullPointerException. @{ */
        int measuredWidth = 0;
        for (int i = 0; i < getChildCount(); i++) {
            View view = getChildAt(i);
            if (view != null) {
                measuredWidth = view.getMeasuredWidth();
                break;
            } else {
                Log.w(TAG, "Workspace childCount=" + getChildCount() + " index=" + i);
            }
        }
        final int childWidthMeasureSpec =
                MeasureSpec.makeMeasureSpec(measuredWidth, MeasureSpec.EXACTLY);
        final int childHeightMeasureSpec =
                MeasureSpec.makeMeasureSpec(measuredWidth, MeasureSpec.EXACTLY);
        /* @} */

        child2.measure(childWidthMeasureSpec, childHeightMeasureSpec);
        child.measure(childWidthMeasureSpec, childHeightMeasureSpec);
        addView(child, getChildCount());
        addView(child2, 0);
        child.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                /* SPRD: Feature 255025,add new view at right side@{ */
                addCellLayoutAt(getChildCount() - 1, false);
                /* @} */
            }
        });
        child2.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                /* SPRD: Feature 255177,add new view at left side@{ */
                addCellLayoutAt(1, true);
                /* @} */
            }
        });
        updateCurrentScreen(mCurrentPage + 1);
        mAddedEditPage = true;

    }
    /* @} */

    /* SPRD: Feature 254953,remove edit view when exit edit mode@{ */
    public void onEditModeEnd() {
        removeEditViews();
    }

    private void removeEditViews() {
        if (!mAddedEditPage) {
            return;
        }
        int nextCurrentPage;
        if (mCurrentPage == getChildCount() - 1) {
            nextCurrentPage = mCurrentPage - EDIT_VIEW_COUNT;
        } else if (mCurrentPage == 0) {
            nextCurrentPage = mCurrentPage;
        } else {
            nextCurrentPage = mCurrentPage - 1;
        }
        removeView(child2);
        removeView(child);
        updateCurrentScreen(nextCurrentPage);
        mAddedEditPage = false;
    }
    /* @} */

    /* SPRD: Feature 255025,add new view at right side@{ */
    private void addCellLayoutAt(int index, boolean needPlus) {
        final LayoutInflater mInflater = mLauncher.getLayoutInflater();
        /*SPRD: fix bug258692 @{*/
        long newId = LauncherAppState.getLauncherProvider().generateNewScreenId();
        if (mWorkspaceScreens.containsKey(newId)) {
            throw new RuntimeException("Screen id " + newId + " already exists!");
        }

        CellLayout childLayout = (CellLayout) mInflater.inflate(R.layout.workspace_screen, null);
        childLayout.setAlpha(1.0f);
        childLayout.setBackgroundAlpha(1.0f);
        mWorkspaceScreens.put(newId, childLayout);
        if (needPlus) {
            mScreenOrder.add(0, newId);
        }else {
            mScreenOrder.add( newId);
        }

        addView(childLayout, index);
        childLayout.setOnLongClickListener(mLongClickListener);

        if (needPlus) {//lwhen add view at eft, will update screenRank of workspaceScreens
            mLauncher.getModel().updateWorkspaceScreenOrder(mLauncher, mScreenOrder);
            /* SPRD: Fix bug 268541, home_default_screen should plus 1. @{ */
            updateDefaultScreenNum(mDefaultPage + 1);
            /* @} */
        } else {
            mLauncher.insertEmptyScreen(newId,  mScreenOrder.size() - 1);
        }

        if (mCustomizePreference == null) {
            mCustomizePreference = (SharedPreferences) mLauncher
                    .getSharedPreferences(SprdLauncher.CUSTOMIZEPREF,
                            Context.MODE_PRIVATE);
        }
        Editor editor = mCustomizePreference.edit();
        editor.putInt(SprdLauncher.HOME_SCREEN_NUM_TAG, getChildCount() - EDIT_VIEW_COUNT);
        editor.commit();
        updateCurrentScreen(mCurrentPage);
        if (getChildCount() >= MAX_PAGE + EDIT_VIEW_COUNT) {
            removeEditViews();
        } else {
            if (needPlus) {
                snapToPage(1);
            }
        }
        /* @} */

        /* SPRD: bug258558 2014-01-23 new requirement to allow delete in edit mode @{ */
        updateChildrenDeletable(true /* allow show delete image */);
        /* SPRD: bug258558 2014-01-23 new requirement to allow delete in edit mode @} */
    }

    /* @} */

    /* SPRD: Feature 254917,add new view in edit mode@{ */
    private boolean mAddedEditPage = false;
    private static final int EDIT_VIEW_COUNT = 2;
    private CellLayout child;
    private CellLayout child2;
    /* @} */

    /**
     * SPRD: Feature 255891,unregisterPreferenceListenerForWorkspaceStyle(). @{
     *
     */
    private void unregisterPreferenceListenerForWorkspaceStyle(){
        // unregitster when detach
        SharedPreferences mAnimPreferences = mLauncher.getSharedPreferences(WorkspaceStyleSettings.WORKSPACE_STYLE, Context.MODE_PRIVATE);
        mAnimPreferences.unregisterOnSharedPreferenceChangeListener(mAnimationStyleChangeListener);
    }
    /** @} */

    /**
     * SPRD: Feature 255891,registerPreferenceListenerForWorkspaceStyle(). @{
     *
     */
    private void registerPreferenceListenerForWorkspaceStyle(){
        // regitster when attach , Remember that we should unregister when detach.
        SharedPreferences mAnimPreferences = mLauncher.getSharedPreferences(WorkspaceStyleSettings.WORKSPACE_STYLE, Context.MODE_PRIVATE);
        mAnimPreferences.registerOnSharedPreferenceChangeListener(mAnimationStyleChangeListener);
    }
    /** @} */

    /* SPRD: Feature 255891,New a Workspace setting PreferenceListener. @{ */
    private OnSharedPreferenceChangeListener mAnimationStyleChangeListener = new OnSharedPreferenceChangeListener(){
        public void onSharedPreferenceChanged( SharedPreferences sharedPreferences, String key) {
            if(WorkspaceStyleSettings.KEY_ANIMATION_STYLE.equalsIgnoreCase(key)){
                initAnimationStyle();

            }
        }
    };
    /* @} */

    /**
     * SPRD: Feature 255891,Init slide Animation. @{
     *
     */
    private void initAnimationStyle(){
        SharedPreferences mAnimPreferences = mLauncher.getSharedPreferences(WorkspaceStyleSettings.WORKSPACE_STYLE, Context.MODE_PRIVATE);
        int type = mAnimPreferences.getInt(WorkspaceStyleSettings.KEY_ANIMATION_STYLE, ANIMATION_DEFAULT);
        mCurentAnimInfo = EffectFactory.getEffect(type);
    }
    /** @} */

    /**
     * SPRD: Feature 255891,Porting from SprdLauncher(Android4.1). @{
     *
     * @param child, t
     * @return boolean
     */
    @Override
    protected boolean getChildStaticTransformation(View child, Transformation t) {
        return false;
    }
    /** @} */

    /**
     * SPRD: Feature 255891,Porting from SprdLauncher(Android4.1). @{
     *
     * @param screenCenter
     */
    protected void screenScrolledStandardUI(int screenCenter) {

        if(mState == State.SPRING_LOADED){
            return;
        }

        if(mCurentAnimInfo == null){
            return;
        }

        /* SPRD: Fix bug258437, Fix bug331782 reset view position properly. @{*/
        final boolean isRtl = isLayoutRtl();
        for (int i = 0; i < getChildCount(); i++) {
            View v = getPageAt(i);
            if (v != null) {

                int mIndex = indexOfChild(v);
                float offset = getScrollProgress(screenCenter,v,mIndex);
                boolean isOverscrollingFirstPage = isRtl ? offset > 0 : offset < 0;
                boolean isOverscrollingLastPage = isRtl ? offset < 0 : offset > 0;

                v.setCameraDistance(mDensity * CAMERA_DISTANCE);
                int pageWidth = getScaledMeasuredWidth(v);
                int pageHeight = v.getMeasuredHeight();
                if (android.os.Build.VERSION.SDK_INT > 15 && offset != 0 && offset != 1 && offset != -1) {
                    //mCurentAnimInfo.getTransformationMatrix(v, offset,pageWidth,pageHeight,mDensity * CAMERA_DISTANCE);
                    if (isOverscrollingFirstPage && mOverScrollX < 0) {
                        mCurentAnimInfo.getTransformationMatrix(v, offset,pageWidth,pageHeight,mDensity * CAMERA_DISTANCE,true,true);
                    } else if (isOverscrollingLastPage && mOverScrollX > mMaxScrollX) {
                        mCurentAnimInfo.getTransformationMatrix(v, offset,pageWidth,pageHeight,mDensity * CAMERA_DISTANCE,true,false);
                    } else {
                        mCurentAnimInfo.getTransformationMatrix(v, offset,pageWidth,pageHeight,mDensity * CAMERA_DISTANCE,false,false);
                    }
                } else {
                    if ((offset == 1.0 && i == (getChildCount()-1))
                            || (offset == -1.0 && i == 0)) {
                        // overscroll last page or first page
                        continue;
                    }
        /* @} */
                    v.setPivotY(pageHeight / 2.0f);
                    v.setPivotX(pageWidth / 2.0f);
                    v.setRotationY(0f);
                    v.setTranslationX(0f);
                    v.setRotationX(0f);
                    v.setRotation(0f);
                    v.setScaleX(1.0f);
                    v.setScaleY(1.0f);
                    v.setAlpha(1f);
                }
            }
        }
    }
    /** @} */

    /**
     * SPRD: Feature 255891,Porting from SprdLauncher(Android4.1). @{
     *
     * @param screenCenter
     */
    protected void screenScrolledLargeUI(int screenCenter) {
        if (isSwitchingState()) return;
        boolean isInOverscroll = false;
        for (int i = 0; i < getChildCount(); i++) {
            CellLayout cl = (CellLayout) getChildAt(i);
            if (cl != null) {
                float scrollProgress = getScrollProgress(screenCenter, cl, i);
                float rotation = WORKSPACE_ROTATION * scrollProgress;
                float translationX = getOffsetXForRotation(rotation, cl.getWidth(), cl.getHeight());

                // If the current page (i) is being over scrolled, we use a different
                // set of rules for setting the background alpha multiplier.
                if (!isSmall()) {
                    if ((mOverScrollX < 0 && i == 0) || (mOverScrollX > mMaxScrollX &&
                            i == getChildCount() -1)) {
                        isInOverscroll = true;
                        rotation *= -1;
                        cl.setBackgroundAlphaMultiplier(
                                overScrollBackgroundAlphaInterpolator(Math.abs(scrollProgress)));
                        mOverScrollPageIndex = i;
                        cl.setOverScrollAmount(Math.abs(scrollProgress), i == 0);
                        cl.setPivotX(cl.getMeasuredWidth() * (i == 0 ? 0.75f : 0.25f));
                        cl.setPivotY(cl.getMeasuredHeight() * 0.5f);
                        cl.setOverscrollTransformsDirty(true);
                    } else if (mOverScrollPageIndex != i) {
                        cl.setBackgroundAlphaMultiplier(
                                backgroundAlphaInterpolator(Math.abs(scrollProgress)));
                    }
                }
                cl.setTranslationX(translationX);
                cl.setRotationY(rotation);
                if (mFadeInAdjacentScreens && !isSmall()) {
                    float alpha = 1 - Math.abs(scrollProgress);
                    cl.setFastAlpha(alpha);
                }
                cl.invalidate();
            }
        }
        if (!isSwitchingState() && !isInOverscroll) {
            ((CellLayout) getChildAt(0)).resetOverscrollTransforms();
            ((CellLayout) getChildAt(getChildCount() - 1)).resetOverscrollTransforms();
        }
        invalidate();
    }
    /** @} */

    /**
     * SPRD: Feature 255891,Porting from SprdLauncher(Android4.1). @{
     *
     * Due to 3D transformations, if two CellLayouts are theoretically touching each other,
     * on the xy plane, when one is rotated along the y-axis, the gap between them is perceived
     * as being larger. This method computes what offset the rotated view should be translated
     * in order to minimize this perceived gap.
     *
     * @param degrees Angle of the view
     * @param width Width of the view
     * @param height Height of the view
     * @return float, Offset to be used in a View.setTranslationX() call
     *
     */
    private float getOffsetXForRotation(float degrees, int width, int height) {
        mMatrix.reset();
        mCamera.save();
        mCamera.rotateY(Math.abs(degrees));
        mCamera.getMatrix(mMatrix);
        mCamera.restore();

        mMatrix.preTranslate(-width * 0.5f, -height * 0.5f);
        mMatrix.postTranslate(width * 0.5f, height * 0.5f);
        mTempFloat2[0] = width;
        mTempFloat2[1] = height;
        mMatrix.mapPoints(mTempFloat2);
        return (width - mTempFloat2[0]) * (degrees > 0.0f ? 1.0f : -1.0f);
    }
    /** @} */

    /**
     * SPRD: Feature 255891,Porting from SprdLauncher(Android4.1). @{
     *
     * @param r
     *
     * @return float
     */
    float overScrollBackgroundAlphaInterpolator(float r) {
        float threshold = 0.08f;

        if (r > mOverScrollMaxBackgroundAlpha) {
            mOverScrollMaxBackgroundAlpha = r;
        } else if (r < mOverScrollMaxBackgroundAlpha) {
            r = mOverScrollMaxBackgroundAlpha;
        }

        return Math.min(r / threshold, 1.0f);
    }
    /** @} */

    /**
     * SPRD: Feature 259193,updateComponentUnreadChanged(), modify from Android.4.1 @{
     *
     */
    public void updateComponentUnreadChanged(ComponentName component, int unreadNum) {
        SharedPreferences mUnreadInfoPreference = (SharedPreferences) mContext
                .getSharedPreferences(SprdLauncher.UNREADINFO, Context.MODE_PRIVATE);
        unreadNum = mUnreadInfoPreference.getInt(component.getPackageName(),
                unreadNum);

        ArrayList<ShortcutAndWidgetContainer> childrenLayouts = getAllShortcutAndWidgetContainers();
        for (ShortcutAndWidgetContainer layout: childrenLayouts) {
            int childCount = layout.getChildCount();
            for (int j = 0; j < childCount; j++) {
                final View view = layout.getChildAt(j);
                Object tag = view.getTag();
                if (tag instanceof ShortcutInfo) {
                    ShortcutInfo info = (ShortcutInfo) tag;
                    // We need to check for ACTION_MAIN otherwise getComponent() might
                    // return null for some shortcuts (for instance, for shortcuts to
                    // web pages.)
                    final Intent intent = info.intent;
                    final ComponentName name = intent.getComponent();
                    if (info.itemType == LauncherSettings.Favorites.ITEM_TYPE_APPLICATION &&
                            Intent.ACTION_MAIN.equals(intent.getAction()) && name != null) {
                            if (component.equals(name)) {
                                BubbleTextView shortcut = (BubbleTextView) view;
                                info.unreadNum = unreadNum;
                                shortcut.applyFromShortcutInfo(info, mIconCache);
                                Bitmap origin = mIconCache.getIcon(info.intent);
                                if(info.unreadNum == 0){
                                    ((BubbleTextView) shortcut).setCompoundDrawablesWithIntrinsicBounds(null,
                                            new FastBitmapDrawable(origin), null,
                                            null);
                                }else{
                                    Bitmap bitmapWithNum = UnreadLoader.getBitmapWithNum(mContext, info, origin);
                                   ((BubbleTextView) shortcut).setCompoundDrawablesWithIntrinsicBounds(null,
                                          new FastBitmapDrawable(bitmapWithNum), null,
                                          null);
                                }
                            }
                        }
                    }
                if (tag instanceof FolderInfo) {
                    FolderIcon folderIcon = (FolderIcon) view;
                    ArrayList<View> folderItem = folderIcon.mFolder.getItemsInReadingOrder();
                    for (View item : folderItem) {
                        ShortcutInfo info = (ShortcutInfo) item.getTag();
                        final Intent intent = info.intent;
                        final ComponentName name = intent.getComponent();
                        if (info.itemType == LauncherSettings.Favorites.ITEM_TYPE_APPLICATION &&
                                Intent.ACTION_MAIN.equals(intent.getAction()) && name != null) {
                            if (component.equals(name)) {
                                info.setIcon(mIconCache.getIcon(info.intent));
                                info.unreadNum = unreadNum;
                                ((BubbleTextView) item).applyFromShortcutInfo(info, mIconCache);
                                Bitmap origin = mIconCache.getIcon(info.intent);
                                if(info.unreadNum == 0){
                                    ((BubbleTextView) item).setCompoundDrawablesWithIntrinsicBounds(null,
                                            new FastBitmapDrawable(origin), null,
                                            null);
                                }else{
                                    Bitmap bitmapWithNum = UnreadLoader.getBitmapWithNum(mContext, info, origin);
                                    ((BubbleTextView) item).setCompoundDrawablesWithIntrinsicBounds(null,
                                           new FastBitmapDrawable(bitmapWithNum), null,
                                           null);
                                }
                                folderIcon.invalidate();
                            }
                        }
                    }
                }
            }
        }
    }
    /** @} */

    /* SPRD: Feature 261947,Update OWNER_WIDGET after onResume. @{ */
    public void updateOwnerWidgetData() {
        int cellCount = getChildCount();
        for (int i = 0; i < cellCount; i++) {
            CellLayout mCellLayout = (CellLayout) getChildAt(i);
            ShortcutAndWidgetContainer mCellChildren = mCellLayout.getShortcutsAndWidgets();
            int count = mCellChildren.getChildCount();
            for (int j = 0; j < count; j++) {
                View view = mCellChildren.getChildAt(j);
                if (view.getTag() instanceof OwnerWidgetInfo){
                    ((WidgetUpdate) view).upData();
                }
            }
        }
    }
    /* @} */
}
