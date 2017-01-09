
package com.dream.camera.modules.panoramadream;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.camera.CameraActivity;

import com.sprd.camera.panora.WideAnglePanoramaController;
import com.sprd.camera.panora.WideAnglePanoramaUI;

import com.android.camera.settings.SettingsManager;
import com.android.camera2.R;

import com.dream.camera.dreambasemodules.DreamInterface;
import com.dream.camera.DreamUI;
import com.dream.camera.SlidePanelManager;
import com.dream.camera.util.DreamUtil;

public class DreamPanoramaUI extends WideAnglePanoramaUI implements DreamInterface {

    public DreamPanoramaUI(CameraActivity activity, WideAnglePanoramaController controller,
            ViewGroup root) {
        super(activity, controller, root);
        activity.getCameraAppUI().setDreamInterface(this);
    }

    @Override
    public void initUI() {

        // Generate a view to fit top panel.
        ViewGroup topPanelParent = (ViewGroup) mActivity.getModuleLayoutRoot().findViewById(
                R.id.top_panel_parent);
        topPanelParent.removeAllViews();
        updateTopPanelValue(mActivity);
        fitTopPanel(topPanelParent);

        // Update visibilities of state icons on side panel.
        updateSidePanel();

        // Generate views to fit extend panel.
        ViewGroup extendPanelParent = (ViewGroup) mActivity.getModuleLayoutRoot().findViewById(
                R.id.extend_panel_parent);
        extendPanelParent.removeAllViews();
        fitExtendPanel(extendPanelParent);

        // Update icons on bottom panel.
        updateBottomPanel();

        // Update item on slide panel.
        updateSlidePanel();
    }

    @Override
    public void fitTopPanel(ViewGroup topPanelParent) {

    }

    @Override
    public void fitExtendPanel(ViewGroup extendPanelParent) {

    }

    @Override
    public void updateBottomPanel() {
        mActivity.getCameraAppUI().updateSwitchModeBtn(this);
    }

    @Override
    public void updateSlidePanel() {
        SlidePanelManager.getInstance(mActivity).udpateSlidePanelShow(
                SlidePanelManager.FILTER,View.INVISIBLE);
        SlidePanelManager.getInstance(mActivity).focusItem(SlidePanelManager.CAPTURE, false);
    }

    protected int sidePanelMask;

    @Override
    public void updateSidePanel() {
        sidePanelMask = DreamUtil.SP_EXTERNAL_STORAGE | DreamUtil.SP_INTERNAL_STORAGE
                | DreamUtil.SP_USB_STORAGE | DreamUtil.SP_LOCATE;
    }

    @Override
    public int getSidePanelMask() {
        return sidePanelMask;
    }

    @Override
    public void onSettingChanged(SettingsManager settingsManager, String key) {

    }
    /* Dream Camera ui check 41*/
    public void adjustUI(int orientation) {
        super.adjustUIDream(orientation);
    }

    public void changeOtherUIVisible(Boolean shuttering, int visible) {
        mActivity.getCameraAppUI().setBottomBarLeftAndRightUI(visible);
        mActivity.getCameraAppUI().updateSidePanelUI(visible);
        mActivity.getCameraAppUI().updateSlidePanelUI(visible);
    }

    public void changeSomethingUIVisible(Boolean shuttering, int visible) {
        mActivity.getCameraAppUI().updateSidePanelUI(visible);
        mActivity.getCameraAppUI().updateSlidePanelUI(visible);
        mActivity.getCameraAppUI().setBottomBarLeftAndRightUI(visible);
    }

    public void changeMosaicSideAndSlideVisible(Boolean shuttering, int visible) {
        mActivity.getCameraAppUI().updateSidePanelUI(visible);
        mActivity.getCameraAppUI().updateSlidePanelUI(visible);
    }

    @Override
    public int getUITpye() {
        return DreamUI.DREAM_WIDEANGLEPANORAMA_UI;
    }
}
