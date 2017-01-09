
package com.dream.camera.modules.intervalphoto;

import java.util.ArrayList;

import com.android.camera.CameraActivity;
import com.android.camera.PhotoUI;
import com.android.camera.app.AppController;
import com.android.camera.debug.Log;
import com.dream.camera.DreamModule;
import com.dream.camera.dreambasemodules.DreamPhotoModule;
import android.net.Uri;
import android.view.View;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.dream.camera.dreambasemodules.DreamPhotoUI;
import android.graphics.Bitmap;
public class IntervalPhotoModule extends DreamPhotoModule {

    private static final Log.Tag TAG = new Log.Tag("IntervalPhotoModule");

    public static int INTERVAL_NUM = 4;
    public static int mNum = INTERVAL_NUM;
    private IntervalPhotoUI ui;
    protected ArrayList<Uri> mDreamIntervalDisplayList = new ArrayList<Uri>();

    public IntervalPhotoModule(AppController app) {
        super(app);
    }

    @Override
    public PhotoUI createUI(CameraActivity activity) {
        ui = new IntervalPhotoUI(activity, this, activity.getModuleLayoutRoot());
        return ui;
    }

    @Override
    public boolean isSupportTouchAFAE() {
        return true;
    }

    @Override
    public boolean isSupportManualMetering() {
        return false;
    }

    @Override
    public boolean shutterAgain() {
        Log.d(TAG, "shutterAgain mNum=" + mNum);
        if (mNum > 0 && mNum < INTERVAL_NUM) {
            return true;
        }
        else {
            return false;
        }
    }

    @Override
    public boolean isInShutter() {
        Log.d(TAG, "isInShutter mNum=" + mNum);
        if (mNum >= 0 && mNum < INTERVAL_NUM) {
            if (mNum == 0) {
                mNum = INTERVAL_NUM;
            }
            return true;
        }
        else {
            return false;
        }
    }

    @Override
    public void onShutterButtonClick() {
        canShutter = false;
        setCaptureCount(0);
        super.onShutterButtonClick();
        if (mNum == INTERVAL_NUM) {
            ui.showFourList(true);
        }
        mNum--;
    }

    public Uri getUri(int i) {
        return mDreamIntervalDisplayList.get(i);
    }

    public int getSize() {
        return mDreamIntervalDisplayList.size();
    }

    public void clear() {
        mDreamIntervalDisplayList.clear();
    }

    @Override
    protected void mediaSaved(final Uri uri) {
        if (isInShutter()) {
            mDreamIntervalDisplayList.add(uri);
            final int index = mDreamIntervalDisplayList.size() - 1;
            mActivity.getDreamHandler().post(new Runnable() {
                @Override
                public void run() {
                    if (ui != null) {
                        ui.prepareFreezeFrame(index, uri);
                    }
                }
            });
        }

        if (shutterAgain()) {
            onShutterButtonClick();
        }
    }

    public void showIntervalFreezeFrame() {
        if (mDreamIntervalDisplayList.size() > 0) {
            ui.showIntevalFreezeFrame(mDreamIntervalDisplayList);
            // ui check 72
            ui.updateIntervalFreezeFrameUI(View.GONE);
        }
    }
    /* dream test 50 @{ */
    protected void doSomethingWhenonPictureTaken() {
    }
    /* @} */
    /* dream test 84 @{ */
    protected void dosomethingWhenPause() {
        mNum = INTERVAL_NUM;

        if (!ui.isFreezeFrameShow())
            clear();

        ui.updateIntervalFreezeFrameUI(View.VISIBLE);
        ui.showFourList(false);
        updateMakeLevel();
    }

    /* @} */
    /* nj dream camera test 84 */
    @Override
    public boolean onBackPressed() {
        if (isInShutter() || mUI.isCountingDown()) {
            mNum = INTERVAL_NUM;
            if (mUI.isCountingDown()) {
                cancelCountDown();
            }
            if (mDreamIntervalDisplayList.size() > 0) {
                mActivity.getDreamHandler().post(new Runnable() {
                    @Override
                    public void run() {
                        mActivity.getMainHandler().post(new Runnable() {
                            @Override
                            public void run() {
                                if (ui != null) {
                                    showIntervalFreezeFrame();
                                    ui.showFourList(false);
                                }
                            }
                        });

                    }
                });
            } else {
                ui.updateIntervalFreezeFrameUI(View.VISIBLE);
                ui.showFourList(false);
                updateMakeLevel();
            }
            return true;
        }
        return super.onBackPressed();
    }
    /* @} */

    @Override
    public void onSingleTapUp(View view, int x, int y) {
        if (mCameraState == PREVIEW_STOPPED) {
            return;
        }
        /* Dream Camera test ui check 20 @{ */
        if (mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_TOUCHING_PHOTOGRAPH)) {
            if (!canShutter()) {//SPRD:fix bug600767
                return;
            }
            onShutterButtonClick();
            /* @} */
        }
    }

    /* SPRD: Fix bug 592600, InterValPhotoModule Thumbnail display abnormal. @{ */
    @Override
    public void updateIntervalThumbnail(final Bitmap indicator){
        mActivity.getMainHandler().post(new Runnable() {
            @Override
            public void run() {
                mActivity.getCameraAppUI().onThumbnail(indicator);
            }
        });
    }
    /* @} */

    @Override
    public int getModuleTpye() {
        return DreamModule.INTERVAL_MODULE;
    }

    private boolean canShutter = true;

    @Override
    public boolean canShutter(){
        if(canShutter && !ui.isFreezeFrameShow()){
            return true;
        }

        return false;
    }

    @Override
    public void resume(){
        super.resume();
        canShutter = true;
    }

    public void setCanShutter(boolean shutter){
        canShutter = shutter;
    }

    public boolean isFreezeFrameDisplayShow() {
        return ui.isFreezeFrameShow();
    }
}
