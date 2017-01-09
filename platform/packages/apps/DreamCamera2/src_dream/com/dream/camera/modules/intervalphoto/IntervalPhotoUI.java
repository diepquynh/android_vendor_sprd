package com.dream.camera.modules.intervalphoto;

import java.util.ArrayList;
import java.util.HashMap;

import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;
import com.android.camera.debug.Log;
import com.dream.camera.dreambasemodules.DreamPhotoUI;
import android.graphics.Bitmap;
import android.net.Uri;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.ImageView;
import com.android.camera.PhotoModule;
import com.android.camera.settings.Keys;
import com.android.camera.ui.RotateTextView;
import com.android.camera.widget.ModeOptionsOverlay;
import com.android.camera2.R;
import com.dream.camera.ButtonManagerDream;
import com.dream.camera.dreambasemodules.DreamPhotoUI;
import com.dream.camera.util.DreamUtil;
import com.dream.camera.settings.DataModuleBasic.DreamSettingChangeListener;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.settings.DataModuleManager.ResetListener;

public class IntervalPhotoUI extends DreamPhotoUI implements
        DreamFreezeFrameDisplayView.DreamProxyFreezeFrameClick, DreamSettingChangeListener {

    private static final Log.Tag TAG = new Log.Tag("IntvalPhUI");

    private ImageButton mSettingsButton;
    private View topPanel;
    private DreamFreezeFrameDisplayView mFreezeFrame;

    public IntervalPhotoUI(CameraActivity activity, PhotoController controller,
            View parent) {
        super(activity, controller, parent);

        if (mFreezeFrame == null) {
            mActivity.getLayoutInflater().inflate(
                    R.layout.interval_freeze_frame_display, getModuleRoot(), true);

            mFreezeFrame = (DreamFreezeFrameDisplayView) getModuleRoot()
                    .findViewById(R.id.preview_camera_freeze_frame_display);
            mFreezeFrame.setListener(this);
        }
    }

    @Override
    public void fitTopPanel(ViewGroup topPanelParent) {

        if (topPanel == null) {
            LayoutInflater lf = LayoutInflater.from(mActivity);
            topPanel = lf.inflate(R.layout.intervalphoto_top_panel,
                    topPanelParent);
        }

        mActivity.getButtonManager().load(topPanel);
        mSettingsButton = (ImageButton) topPanel
                .findViewById(R.id.settings_button_dream);

        bindSettingsButton(mSettingsButton);

        bindCountDownButton();

        bindMakeupButton();

        bindCameraButton();
    }

    @Override
    public void bindCountDownButton() {
        ButtonManagerDream buttonManager = (ButtonManagerDream) mActivity
                .getButtonManager();
        buttonManager.initializeButton(
                ButtonManagerDream.BUTTON_COUNTDOWN_DREAM, null,
                R.array.dream_countdown_duration_icons_without_off);
    }

    @Override
    public void updateSidePanel() {
        super.updateSidePanel();
    }

    @Override
    public void fitExtendPanel(ViewGroup extendPanelParent) {
        LayoutInflater lf = LayoutInflater.from(mActivity);
        // mFreezeFrame = extendPanelParent;
        View extendPanel = lf.inflate(R.layout.intervalphoto_extend_panel,
                extendPanelParent);

        initMakeupControl(extendPanelParent);
    }

    @Override
    public void updateBottomPanel() {
        super.updateBottomPanel();
    }

    @Override
    public void updateSlidePanel() {
        super.updateSlidePanel();
    }

    public void showIntevalFreezeFrame(ArrayList<Uri> intervalDisplayList) {
        mFreezeFrame.init(intervalDisplayList);
        saved = false;
    }

    private boolean saved = false;

    public void dreamProxyDoneClicked(boolean save) {
        saved = save;

        if (saved && mFreezeFrame != null && mFreezeFrame.getGiveupNum() == 0) {
            mActivity.runSyncThumbnail();
        }

        Log.d(TAG, "dreamProxyDoneClicked");
        mFreezeFrame.reset();
        mFreezeFrame.setVisibility(View.GONE);
        ((IntervalPhotoModule) mController).clear();
        // ui check 72
        updateIntervalFreezeFrameUI(View.VISIBLE);
        // ui check 129,dream camera test 121
        // interval self own
        if (DataModuleManager.getInstance(mActivity).getCurrentDataModule()
                .getBoolean(Keys.KEY_CAMERA_BEAUTY_ENTERED)) {
            mMakeupController.resumeMakeupControllerView();
        }
    }

    @Override
    public void dreamProxyFinishDeleted(Uri uri) {
        Log.d(TAG, "dreamProxyFinishDeleted " + uri);
        mActivity.removeDataByUri(uri);

        if (saved == true) {
            mActivity.runSyncThumbnail();
        }
    }

    private int fourListState = 0;

    public void showFourList(boolean show) {

        View bottomBar = mActivity.findViewById(R.id.bottom_bar);

        View fourList = mActivity.findViewById(R.id.four_list);

        if (show) {
            bottomBar.setVisibility(View.GONE);
            fourList.setVisibility(View.VISIBLE);
            ((IntervalPhotoModule) mController).setCanShutter(false);
        } else {
            bottomBar.setVisibility(View.VISIBLE);
            fourList.setVisibility(View.GONE);
            ((IntervalPhotoModule) mController).setCanShutter(true);
        }

        if (previewOne == null || previewTwo == null || previewThree == null
                || previewFour == null) {
            previewOne = (ImageView) mActivity.findViewById(R.id.preview_1);
            previewTwo = (ImageView) mActivity.findViewById(R.id.preview_2);
            previewThree = (ImageView) mActivity.findViewById(R.id.preview_3);
            previewFour = (ImageView) mActivity.findViewById(R.id.preview_4);
        }

        if (previewOne == null || previewTwo == null || previewThree == null
                || previewFour == null) {
            return;
        }

        previewOne.setImageBitmap(null);
        previewTwo.setImageBitmap(null);
        previewThree.setImageBitmap(null);
        previewFour.setImageBitmap(null);

        fourListState = 0;

    }

    private ImageView previewOne;
    private ImageView previewTwo;
    private ImageView previewThree;
    private ImageView previewFour;

    public void onThumbnail(final Bitmap thumbmail) {
        if (previewOne == null || previewTwo == null || previewThree == null
                || previewFour == null) {
            previewOne = (ImageView) mActivity.findViewById(R.id.preview_1);
            previewTwo = (ImageView) mActivity.findViewById(R.id.preview_2);
            previewThree = (ImageView) mActivity.findViewById(R.id.preview_3);
            previewFour = (ImageView) mActivity.findViewById(R.id.preview_4);
        }

        if (previewOne == null || previewTwo == null || previewThree == null
                || previewFour == null) {
            return;
        }

        switch (fourListState) {
        case 0:
            previewOne.setImageBitmap(thumbmail);
            break;
        case 1:
            previewTwo.setImageBitmap(thumbmail);
            break;
        case 2:
            previewThree.setImageBitmap(thumbmail);
            break;
        case 3:
            previewFour.setImageBitmap(thumbmail);
            if (mActivity.getMainHandler() != null)
                mActivity.getMainHandler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        if (fourListState != 0) {//SPRD:fix bug600908 not on pause
                            showFourList(false);
                            ((IntervalPhotoModule) mController).showIntervalFreezeFrame();
                        }
                    }
                }, 1000);
            break;
        default:
            break;
        }
        fourListState++;
    }

    /*
     * ui check 8, 71
     * 
     * @{
     */
    public void updateStartCountDownUI() {
        // top panel hidden
        mActivity.getCameraAppUI().updateTopPanelUI(View.GONE);
        // side panel hidden
        mActivity.getCameraAppUI().updateSidePanelUI(View.GONE);
        // bottom bar left and right hidden
        mActivity.getCameraAppUI().setBottomBarLeftAndRightUI(View.GONE);
        // shutter button hidden
        mActivity.getCameraAppUI().updateShutterButtonUI(View.GONE);
        // slide panel hidden
        mActivity.getCameraAppUI().updateSlidePanelUI(View.GONE);

        // ui check 129
        // interval self own
        //mMakeupController.pauseMakeupControllerView();
        mMakeupController.pauseMakeupControllerView(false);
    }

    /* @} */

    /**
     * UI CHECK 72
     */
    public void updateIntervalFreezeFrameUI(int visibility) {
        mActivity.getCameraAppUI().updatePreviewUI(visibility);
    }

    @Override
    public void onResume(){
        if(isFreezeFrameShow()){
            updateStartCountDownUI();
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        DataModuleManager.getInstance(mActivity).getCurrentDataModule()
                .removeListener(this);
    }

    @Override
    public void onDreamSettingChangeListener(HashMap<String, String> keys) {
        for (String key : keys.keySet()) {
            switch (key) {
            case Keys.KEY_CAMERA_BEAUTY_ENTERED:
                mBasicModule.updateMakeLevel();
                break;

            default:
                break;
            }
        }

    }

    @Override
    public boolean onBackPressed(){
        if(mFreezeFrame != null && mFreezeFrame.isShown()){
            mFreezeFrame.showAlertDialog();
            return true;
        }
        return super.onBackPressed();
    }

    public boolean isFreezeFrameShow() {
        if (mFreezeFrame != null && mFreezeFrame.getVisibility() == View.VISIBLE) {
            return true;
        }

        return false;
    }

    public void prepareFreezeFrame(int index, Uri uri) {
        mFreezeFrame.prepare(index, uri);
    }

    /*SPRD:fix bug625571 change interval decode for OOM @{ */
    @Override
    protected void setPictureInfo(int width, int height, int orientation) {
        mFreezeFrame.setPictureInfo(width, height,orientation);
    }
    /* @} */
}
