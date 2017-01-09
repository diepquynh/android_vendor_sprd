package com.dream.camera.modules.scenephoto;

import java.util.HashMap;

import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;
import com.android.camera.app.AppController;
import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera.util.GcamHelper;
import com.dream.camera.dreambasemodules.DreamPhotoUI;

import com.dream.camera.settings.DataModuleManager;
import com.dream.util.SettingsList;

import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import com.android.camera2.R;
import com.android.ex.camera2.portability.CameraCapabilities;
import com.android.ex.camera2.portability.CameraSettings;

import android.graphics.Color;

public class ScenePhotoUI extends DreamPhotoUI implements View.OnClickListener{
    private static final Log.Tag TAG = new Log.Tag("ScenePUI");
    private ImageButton mSettingsButton;

    // extend panel
    private View extendPanel;

    private View autoScene;
    private TextView autoSceneTv;
    private ImageView autoSceneIcon;

    private View actionScene;
    private TextView actionSceneTv;
    private ImageView actionSceneIcon;

    private View nightScene;
    private TextView nightSceneTv;
    private ImageView nightSceneIcon;

    private View normalScene;
    private TextView normalSceneTv;
    private ImageView normalSceneIcon;

    private View portraitScene;
    private TextView portraitSceneTv;
    private ImageView portraitSceneIcon;

    private View landscapeScene;
    private TextView landscapeSceneTv;
    private ImageView landscapeSceneIcon;

    public ScenePhotoUI(CameraActivity activity, PhotoController controller,
            View parent) {
        super(activity, controller, parent);
    }

    @Override
    public void fitTopPanel(ViewGroup topPanelParent) {
        Log.d(TAG, "fitTopPanel");
        LayoutInflater lf = LayoutInflater.from(mActivity);
        View topPanel = lf.inflate(R.layout.scenephoto_top_panel,
                topPanelParent);

        mActivity.getButtonManager().load(topPanel);

        mSettingsButton = (ImageButton) topPanel
                .findViewById(R.id.settings_button_dream);

        bindSettingsButton(mSettingsButton);
        bindFlashButton();
        bindCountDownButton();
        bindCameraButton();
    }

    @Override
    public void updateSidePanel() {
        super.updateSidePanel();
    }

    @Override
    public void fitExtendPanel(ViewGroup extendPanelParent) {
        if (extendPanel == null) {
            LayoutInflater lf = LayoutInflater.from(mActivity);
            extendPanel = lf.inflate(R.layout.scenephoto_extend_panel,
                    extendPanelParent);
        }

        // AUTO SCENE
        autoScene = extendPanelParent.findViewById(R.id.auto_scene);
        autoScene.setOnClickListener(this);

        autoSceneIcon = (ImageView) extendPanelParent
                .findViewById(R.id.auto_scene_icon);

        autoSceneTv = (TextView) extendPanelParent
                .findViewById(R.id.auto_scene_tv);

        // ACTION SCENE
        actionScene = extendPanelParent.findViewById(R.id.action_scene);
        actionScene.setOnClickListener(this);

        actionSceneIcon = (ImageView) extendPanelParent
                .findViewById(R.id.action_scene_icon);

        actionSceneTv = (TextView) extendPanelParent
                .findViewById(R.id.action_scene_tv);

        // NIGHT SCENE
        nightScene = extendPanelParent.findViewById(R.id.night_scene);
        nightScene.setOnClickListener(this);

        nightSceneIcon = (ImageView) extendPanelParent
                .findViewById(R.id.night_scene_icon);

        nightSceneTv = (TextView) extendPanelParent
                .findViewById(R.id.night_scene_tv);

        // NORMAL SCENE
        normalScene = extendPanelParent.findViewById(R.id.normal_scene);
        normalScene.setOnClickListener(this);

        normalSceneIcon = (ImageView) extendPanelParent
                .findViewById(R.id.normal_scene_icon);

        normalSceneTv = (TextView) extendPanelParent
                .findViewById(R.id.normal_scene_tv);

        // portrait SCENE
        portraitScene = extendPanelParent.findViewById(R.id.portrait_scene);
        portraitScene.setOnClickListener(this);

        portraitSceneIcon = (ImageView) extendPanelParent
                .findViewById(R.id.portrait_scene_icon);

        portraitSceneTv = (TextView) extendPanelParent
                .findViewById(R.id.portrait_scene_tv);

        // landscape SCENE
        landscapeScene = extendPanelParent.findViewById(R.id.landscape_scene);
        landscapeScene.setOnClickListener(this);

        landscapeSceneIcon = (ImageView) extendPanelParent
                .findViewById(R.id.landscape_scene_icon);

        landscapeSceneTv = (TextView) extendPanelParent
                .findViewById(R.id.landscape_scene_tv);

    }

    @Override
    public void updateBottomPanel() {
        super.updateBottomPanel();
    }

    @Override
    public void updateSlidePanel() {
        super.updateSlidePanel();
    }

    @Override
    public void onClick(View view) {
        DataModuleManager
                .getInstance(mActivity)
                .getDataModulePhoto()
                .changeSettings(Keys.KEY_SCENE_MODE,
                        sceneModeViewToString(view));
    }

    public void initSceneModeUI(String sceneMode) {
        updateUIDisplay(sceneMode);
    }

    public String sceneModeViewToString(View view) {
        String sceneMode = "auto";

        if (view == actionScene) {
            sceneMode = "action";
        } else if (view == nightScene) {
            sceneMode = "night";
        } else if (view == normalScene) {
            sceneMode = "normal";
        } else if (view == portraitScene) {
            sceneMode = "portrait";
        } else if (view == landscapeScene) {
            sceneMode = "landscape";
        }
        return sceneMode;
    }

    public View updateUIDisplay(String sceneMode) {

        View view = autoScene;
        if ("action".equals(sceneMode)) {
            view = actionScene;
        } else if ("night".equals(sceneMode)) {
            view = nightScene;
        } else if ("normal".equals(sceneMode)) {
            view = normalScene;
        } else if ("portrait".equals(sceneMode)) {
            view = portraitScene;
        } else if ("landscape".equals(sceneMode)) {
            view = landscapeScene;
        } else {
            view = autoScene;
        }

        if (autoScene != null) {
            autoSceneIcon
                    .setImageResource(view == autoScene ? R.drawable.ic_operate_scene_auto_sprd_selected
                            : R.drawable.ic_operate_scene_auto_sprd_unselected);
            //UI CHECK 105
            autoSceneTv.setTextColor(view == autoScene ? ((AppController)mActivity).getAndroidContext().getResources()
                    .getColor(R.color.dream_yellow) : Color.WHITE);
        }
        if (nightScene != null) {
            nightSceneIcon
                    .setImageResource(view == nightScene ? R.drawable.ic_operate_scene_night_sprd_selected
                            : R.drawable.ic_operate_scene_night_sprd_unselected);
            //UI CHECK 105
            nightSceneTv.setTextColor(view == nightScene ? ((AppController)mActivity).getAndroidContext().getResources()
                    .getColor(R.color.dream_yellow) : Color.WHITE);
        }
        if (actionScene != null) {
            actionSceneIcon
                    .setImageResource(view == actionScene ? R.drawable.ic_operate_scene_action_sprd_selected
                            : R.drawable.ic_operate_scene_action_sprd_unselected);
            //UI CHECK 105
            actionSceneTv.setTextColor(view == actionScene ? ((AppController)mActivity).getAndroidContext().getResources()
                    .getColor(R.color.dream_yellow) : Color.WHITE);
        }
        if (normalScene != null) {
            normalSceneIcon
                    .setImageResource(view == normalScene ? R.drawable.ic_operate_scene_normal_sprd_selected
                            : R.drawable.ic_operate_scene_normal_sprd_unselected);
            //UI CHECK 105
            normalSceneTv.setTextColor(view == normalScene ? ((AppController)mActivity).getAndroidContext().getResources()
                    .getColor(R.color.dream_yellow) : Color.WHITE);
        }
        if (portraitScene != null) {
            portraitSceneIcon
                    .setImageResource(view == portraitScene ? R.drawable.ic_operate_scene_portrait_sprd_selected
                            : R.drawable.ic_operate_scene_portrait_sprd_unselected);
            //UI CHECK 105
            portraitSceneTv.setTextColor(view == portraitScene ? ((AppController)mActivity).getAndroidContext().getResources()
                    .getColor(R.color.dream_yellow) : Color.WHITE);
        }
        if (landscapeScene != null) {
            landscapeSceneIcon
                    .setImageResource(view == landscapeScene ? R.drawable.ic_operate_scene_landscape_sprd_selected
                            : R.drawable.ic_operate_scene_landscape_sprd_unselected);
            //UI CHECK 105
            landscapeSceneTv.setTextColor(view == landscapeScene ? ((AppController)mActivity).getAndroidContext().getResources()
                    .getColor(R.color.dream_yellow) : Color.WHITE);
        }

        return view;

    }
    //nj dream camera test 58
    @Override
    public void onCameraOpened(CameraCapabilities capabilities, CameraSettings settings) {
        super.onCameraOpened(capabilities,settings);
        String sceneMode = DataModuleManager.getInstance(mActivity)
                .getDataModulePhoto().getString(Keys.KEY_SCENE_MODE);
        initSceneModeUI(sceneMode);
    }
}
