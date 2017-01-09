package com.dream.camera.modules.manualphoto;

import java.util.HashMap;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.SeekBar;
import android.widget.TextView;
import android.graphics.Color;
import android.widget.ImageView;

import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;
import com.android.camera.app.AppController;

import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;

import com.android.camera2.R;

import com.dream.camera.dreambasemodules.DreamPhotoUI;
import com.dream.camera.settings.DataModuleBasic;
import com.dream.camera.settings.DataModuleBasic.DreamSettingChangeListener;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.settings.DataModulePhoto;
import com.dream.util.SettingsList;

public class ManualPhotoUI extends DreamPhotoUI implements
        View.OnClickListener, SeekBar.OnSeekBarChangeListener {

    // top panel
    private View topPanel;

    private ImageButton tSettingsButton;
    private ImageButton tFlashToggleButton;
    private ImageButton tCountDownToggleButton;
    private ImageButton tMeteringToggleButton;
    private ImageButton tCameraToggleButton;

    // extend panel
    private View extendPanel;

    private View eExposureBtn;
    private ImageView eExposureIcon;
    private TextView eExposureTv;
    private View eExposurePanel;
    private SeekBar eExposureSeekBar;

    private View eIsoBtn;
    private ImageView eIsoIcon;
    private TextView eIsoTv;
    private View eIsoPanel;
    private SeekBar eIsoSeekBar;

    private View eWbBtn;
    private ImageView eWbIcon;
    private TextView eWbTv;
    private View eWbPanel;
    private SeekBar eWbSeekBar;

    private View eStyleBtn;
    private ImageView eStyleIcon;
    private TextView eStyleTv;
    private View eStylePanel;

    private SeekBar eContrastSeekBar;
    private SeekBar eSaturationSeekBar;
    private SeekBar eBrightnessSeekBar;

    public ManualPhotoUI(CameraActivity activity, PhotoController controller,
            View parent) {
        super(activity, controller, parent);
    }

    @Override
    public void fitTopPanel(ViewGroup topPanelParent) {

        if (topPanel == null) {
            LayoutInflater lf = LayoutInflater.from(mActivity);
            topPanel = lf.inflate(R.layout.manualphoto_top_panel,
                    topPanelParent);
        }

        bindTopButtons();
    }

    private void bindTopButtons() {
        mActivity.getButtonManager().load(topPanel);

        tSettingsButton = (ImageButton) topPanel
                .findViewById(R.id.settings_button_dream);

        bindSettingsButton(tSettingsButton);
        bindFlashButton();
        bindCountDownButton();
        bindMeteringButton();
        bindCameraButton();
        bindMeteringButton();
    }

    @Override
    public void updateSidePanel() {
        super.updateSidePanel();
    }

    /*SPRD:fix bug 622818 add for exposure to adapter auto @{*/
    @Override
    protected void updateExposureUI() {
        int exposure = DataModuleManager.getInstance(mActivity)
                .getDataModulePhoto().getInt(Keys.KEY_EXPOSURE);
        //int exposureIndex = exposure + 3;
        int exposureIndex = convertExposureValueToIndex(exposure);

        eExposureTv.setText(mActivity.getAndroidContext().getString(
                SettingsList.EXPOSURE_DISPLAY[exposureIndex]));
        eExposureSeekBar.setProgress(exposureIndex);
    }
    /* @} */

    @Override
    public void fitExtendPanel(ViewGroup extendPanelParent) {
        if (extendPanel == null) {
            LayoutInflater lf = LayoutInflater.from(mActivity);
            extendPanel = lf.inflate(R.layout.manualphoto_extend_panel,
                    extendPanelParent);
        }

        // EXPOSURE
        eExposureBtn = extendPanelParent.findViewById(R.id.e_exposure_btn);
        eExposureBtn.setOnClickListener(this);

        eExposureIcon = (ImageView) extendPanelParent
                .findViewById(R.id.e_exposure_icon);
        eExposureTv = (TextView) extendPanelParent
                .findViewById(R.id.e_exposure_tv);
        //eExposureTv.setText(mActivity.getAndroidContext().getString(
        //        SettingsList.EXPOSURE_DISPLAY[exposureIndex]));

        eExposurePanel = extendPanelParent.findViewById(R.id.e_exposure_panel);

        eExposureSeekBar = (SeekBar) extendPanelParent
                .findViewById(R.id.e_exposure_seekbar);
        eExposureSeekBar.setMax(SettingsList.EXPOSURE_DISPLAY.length - 1);
        //eExposureSeekBar.setProgress(exposureIndex);
        eExposureSeekBar.setOnSeekBarChangeListener(this);
        updateExposureUI();

        // ISO
        eIsoBtn = extendPanelParent.findViewById(R.id.e_iso_btn);
        eIsoBtn.setOnClickListener(this);

        String iso = DataModuleManager.getInstance(mActivity)
                .getDataModulePhoto().getString(Keys.KEY_CAMERA_ISO);
        int isoIndex = SettingsList.indexOf(iso, SettingsList.ISO,
                SettingsList.ISO_DEFAULT);

        eIsoIcon = (ImageView) extendPanelParent.findViewById(R.id.e_iso_icon);
        eIsoTv = (TextView) extendPanelParent.findViewById(R.id.e_iso_tv);
        eIsoTv.setText(mActivity.getAndroidContext().getString(
                SettingsList.ISO_DISPLAY[isoIndex]));

        eIsoPanel = extendPanelParent.findViewById(R.id.e_iso_panel);

        eIsoSeekBar = (SeekBar) extendPanelParent
                .findViewById(R.id.e_iso_seekbar);
        eIsoSeekBar.setMax(SettingsList.ISO_DISPLAY.length - 1);
        eIsoSeekBar.setProgress(isoIndex);
        eIsoSeekBar.setOnSeekBarChangeListener(this);

        // WHITE_BALANCE
        eWbBtn = extendPanelParent.findViewById(R.id.e_wb_btn);
        eWbBtn.setOnClickListener(this);

        String wb = DataModuleManager.getInstance(mActivity)
                .getDataModulePhoto().getString(Keys.KEY_WHITE_BALANCE);
        int wbIndex = SettingsList.indexOf(wb, SettingsList.WHITE_BALANCE,
                SettingsList.WHITE_BALANCE_DEFAULT);

        eWbIcon = (ImageView) extendPanelParent.findViewById(R.id.e_wb_icon);
        eWbTv = (TextView) extendPanelParent.findViewById(R.id.e_wb_tv);
        eWbTv.setText(mActivity.getAndroidContext().getString(
                SettingsList.WHITE_BALANCE_DISPLAY[wbIndex]));

        eWbPanel = extendPanelParent.findViewById(R.id.e_wb_panel);

        eWbSeekBar = (SeekBar) extendPanelParent
                .findViewById(R.id.e_wb_seekbar);
        eWbSeekBar.setMax(SettingsList.WHITE_BALANCE_DISPLAY.length - 1);
        eWbSeekBar.setProgress(wbIndex);
        eWbSeekBar.setOnSeekBarChangeListener(this);

        // Style
        eStyleBtn = extendPanelParent.findViewById(R.id.e_style_btn);
        eStyleBtn.setOnClickListener(this);

        eStyleIcon = (ImageView) extendPanelParent
                .findViewById(R.id.e_style_icon);
        eStyleTv = (TextView) extendPanelParent.findViewById(R.id.e_style_tv);

        eStylePanel = extendPanelParent.findViewById(R.id.e_style_panel);

        // CONTRAST
        String contrast = DataModuleManager.getInstance(mActivity)
                .getDataModulePhoto().getString(Keys.KEY_CAMERA_CONTRAST);
        int contrastIndex = SettingsList.indexOf(contrast,
                SettingsList.CONTRAST, SettingsList.CONTRAST_DEFAULT);

        eContrastSeekBar = (SeekBar) extendPanelParent
                .findViewById(R.id.e_contrast_seekbar);
        eContrastSeekBar.setMax(SettingsList.CONTRAST_DISPLAY.length - 1);
        eContrastSeekBar.setProgress(contrastIndex);
        eContrastSeekBar.setOnSeekBarChangeListener(this);

        // SATURATION
        String saturation = DataModuleManager.getInstance(mActivity)
                .getDataModulePhoto().getString(Keys.KEY_CAMERA_SATURATION);
        int saturatioIndex = SettingsList.indexOf(saturation,
                SettingsList.SATURATION, SettingsList.SATURATION_DEFAULT);

        eSaturationSeekBar = (SeekBar) extendPanelParent
                .findViewById(R.id.e_saturation_seekbar);
        eSaturationSeekBar.setMax(SettingsList.SATURATION_DISPLAY.length - 1);
        eSaturationSeekBar.setProgress(saturatioIndex);
        eSaturationSeekBar.setOnSeekBarChangeListener(this);

        // BRIGHTNESS
        String brightness = DataModuleManager.getInstance(mActivity)
                .getDataModulePhoto().getString(Keys.KEY_CAMERA_BRIGHTNESS);
        int brightnessIndex = SettingsList.indexOf(brightness,
                SettingsList.BRIGHTNESS, SettingsList.BRIGHTNESS_DEFAULT);

        eBrightnessSeekBar = (SeekBar) extendPanelParent
                .findViewById(R.id.e_brightness_seekbar);
        eBrightnessSeekBar.setMax(SettingsList.BRIGHTNESS_DISPLAY.length - 1);
        eBrightnessSeekBar.setProgress(brightnessIndex);
        eBrightnessSeekBar.setOnSeekBarChangeListener(this);

        changeColorStyleDescription();
    }

    @Override
    public void updateBottomPanel() {
        super.updateBottomPanel();
    }

    @Override
    public void updateSlidePanel() {
        super.updateSlidePanel();
    }

    private void hideAllSeekbar() {
        if (eExposurePanel != null) {
            eExposurePanel.setVisibility(View.GONE);
        }
        if (eIsoPanel != null) {
            eIsoPanel.setVisibility(View.GONE);
        }
        if (eExposurePanel != null) {
            eExposurePanel.setVisibility(View.GONE);
        }
        if (eWbPanel != null) {
            eWbPanel.setVisibility(View.GONE);
        }
        if (eStylePanel != null) {
            eStylePanel.setVisibility(View.GONE);
        }
    }

    private void changeIconUI(View view, int visible) {
        boolean showOrHide = false;
        if (eExposureBtn != null) {
            showOrHide = view == eExposureBtn && visible == View.VISIBLE;
            eExposureIcon
                    .setImageResource(showOrHide ? R.drawable.ic_operate_ev_sprd_selected
                            : R.drawable.ic_operate_ev_sprd_unselected);
            // UI CHECK 105
            eExposureTv.setTextColor(showOrHide ? ((AppController) mActivity)
                    .getAndroidContext().getResources()
                    .getColor(R.color.dream_yellow) : Color.WHITE);
        }
        if (eIsoBtn != null) {
            showOrHide = view == eIsoBtn && visible == View.VISIBLE;
            eIsoIcon.setImageResource(showOrHide ? R.drawable.ic_operate_iso_sprd_selected
                    : R.drawable.ic_operate_iso_sprd_unselected);
            // UI CHECK 105
            eIsoTv.setTextColor(showOrHide ? ((AppController) mActivity)
                    .getAndroidContext().getResources()
                    .getColor(R.color.dream_yellow) : Color.WHITE);
        }
        if (eWbBtn != null) {
            showOrHide = view == eWbBtn && visible == View.VISIBLE;
            eWbIcon.setImageResource(showOrHide ? R.drawable.ic_operate_wb_auto_sprd_selected
                    : R.drawable.ic_operate_wb_auto_sprd_unselected);
            // UI CHECK 105
            eWbTv.setTextColor(showOrHide ? ((AppController) mActivity)
                    .getAndroidContext().getResources()
                    .getColor(R.color.dream_yellow) : Color.WHITE);
        }
        if (eStyleBtn != null) {
            showOrHide = view == eStyleBtn && visible == View.VISIBLE;
            eStyleIcon
                    .setImageResource(showOrHide ? R.drawable.ic_operate_art_style_sprd_selected
                            : R.drawable.ic_operate_art_style_sprd_unselected);
            // UI CHECK 105
            eStyleTv.setTextColor(showOrHide ? ((AppController) mActivity)
                    .getAndroidContext().getResources()
                    .getColor(R.color.dream_yellow) : Color.WHITE);
        }
    }

    @Override
    public void onClick(View view) {

        int visible = View.VISIBLE;
        if (eExposureBtn != null && view == eExposureBtn) {
            if (eExposurePanel.getVisibility() == View.GONE) {
                hideAllSeekbar();
                visible = View.VISIBLE;

            } else if (eExposurePanel.getVisibility() == View.VISIBLE) {
                visible = View.GONE;
            }
            eExposurePanel.setVisibility(visible);
            changeIconUI(view, visible);
            return;
        }

        if (eIsoBtn != null && view == eIsoBtn) {
            if (eIsoPanel.getVisibility() == View.GONE) {
                hideAllSeekbar();
                visible = View.VISIBLE;
            } else if (eIsoPanel.getVisibility() == View.VISIBLE) {
                visible = View.GONE;
            }
            eIsoPanel.setVisibility(visible);
            changeIconUI(view, visible);
            return;
        }

        if (eWbBtn != null && view == eWbBtn) {
            if (eWbPanel.getVisibility() == View.GONE) {
                hideAllSeekbar();
                visible = View.VISIBLE;
            } else if (eWbPanel.getVisibility() == View.VISIBLE) {
                visible = View.GONE;

            }
            eWbPanel.setVisibility(visible);
            changeIconUI(view, visible);
            return;
        }

        if (eStyleBtn != null && view == eStyleBtn) {
            if (eStylePanel.getVisibility() == View.GONE) {
                hideAllSeekbar();
                visible = View.VISIBLE;
            } else if (eStylePanel.getVisibility() == View.VISIBLE) {
                visible = View.GONE;

            }
            eStylePanel.setVisibility(visible);
            changeIconUI(view, visible);
            return;
        }
    }

    @Override
    public void onSettingChanged(SettingsManager settingsManager, String key) {
        //
        // // EXPOSURE
        // if (Keys.KEY_EXPOSURE.equals(key)) {
        // int exposure = settingsManager.getInteger(
        // mActivity.getCameraScope(), Keys.KEY_EXPOSURE);
        // int exposureIndex = exposure + 3;
        //
        // eExposureTv.setText(mActivity.getAndroidContext().getString(
        // SettingsList.EXPOSURE_DISPLAY[exposureIndex]));
        //
        // eExposureSeekBar.setProgress(exposureIndex);
        // return;
        // }
        //
        // // ISO
        // if (Keys.KEY_CAMERA_ISO.equals(key)) {
        // String iso = settingsManager.getString(
        // SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_ISO);
        // int isoIndex = SettingsList.indexOf(iso, SettingsList.ISO,
        // SettingsList.ISO_DEFAULT);
        //
        // eIsoTv.setText(mActivity.getAndroidContext().getString(
        // SettingsList.ISO_DISPLAY[isoIndex]));
        //
        // eIsoSeekBar.setProgress(isoIndex);
        // return;
        // }
        //
        // // WHITE_BALANCE
        // if (Keys.KEY_WHITE_BALANCE.equals(key)) {
        // String wb = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
        // Keys.KEY_WHITE_BALANCE);
        // int wbIndex = SettingsList.indexOf(wb, SettingsList.WHITE_BALANCE,
        // SettingsList.WHITE_BALANCE_DEFAULT);
        //
        // eWbTv.setText(mActivity.getAndroidContext().getString(
        // SettingsList.WHITE_BALANCE_DISPLAY[wbIndex]));
        //
        // eWbSeekBar.setProgress(wbIndex);
        // return;
        // }
        //
        // // CONTRAST
        // if (Keys.KEY_CAMERA_CONTRAST.equals(key)) {
        // String contrast = settingsManager.getString(
        // SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_CONTRAST);
        // int contrastIndex = SettingsList.indexOf(contrast,
        // SettingsList.CONTRAST, SettingsList.CONTRAST_DEFAULT);
        //
        // eContrastSeekBar.setProgress(contrastIndex);
        // return;
        // }
        //
        // // SATURATION
        // if (Keys.KEY_CAMERA_SATURATION.equals(key)) {
        // String saturation = settingsManager.getString(
        // SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_SATURATION);
        // int saturatioIndex = SettingsList.indexOf(saturation,
        // SettingsList.SATURATION, SettingsList.SATURATION_DEFAULT);
        //
        // eSaturationSeekBar.setProgress(saturatioIndex);
        // return;
        // }
        //
        // // BRIGHTNESS
        // if (Keys.KEY_CAMERA_BRIGHTNESS.equals(key)) {
        // String brightness = settingsManager.getString(
        // SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_BRIGHTNESS);
        // int brightnessIndex = SettingsList.indexOf(brightness,
        // SettingsList.BRIGHTNESS, SettingsList.BRIGHTNESS_DEFAULT);
        //
        // eBrightnessSeekBar.setProgress(brightnessIndex);
        // return;
        // }
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress,
            boolean fromUser) {

        if (!fromUser)
            return;

        DataModuleBasic currentDataModule = DataModuleManager.getInstance(
                mActivity).getDataModulePhoto();

        if (eExposureSeekBar != null && seekBar == eExposureSeekBar) {
            currentDataModule.set(Keys.KEY_EXPOSURE_COMPENSATION_ENABLED, true);
            /*SPRD:fix bug 622818 add for exposure to adapter auto @{*/
            if (mExposureCompensationStep != 0.0f) {
                int exposureValue = Math.round((progress - 3) / mExposureCompensationStep);
                mBasicModule.setExposureCompensation(exposureValue);
                mBasicModule.applySettings();
                // nj dream test 124
                eExposureTv.setText(mActivity.getAndroidContext().getString(
                        SettingsList.EXPOSURE_DISPLAY[progress]));
            }
            /* @} */
        }

        if (eIsoSeekBar != null && seekBar == eIsoSeekBar) {
            currentDataModule.set(Keys.KEY_CAMERA_ISO,
                    SettingsList.ISO[progress]);
            mBasicModule.updateParametersISO();
            mBasicModule.applySettings();
            // nj dream test 124
            eIsoTv.setText(mActivity.getAndroidContext().getString(
                    SettingsList.ISO_DISPLAY[progress]));
        }

        if (eWbSeekBar != null && seekBar == eWbSeekBar) {
            currentDataModule.set(Keys.KEY_WHITE_BALANCE,
                    SettingsList.WHITE_BALANCE[progress]);
            mBasicModule.updateParametersWhiteBalance();
            mBasicModule.applySettings();
            // nj dream test 124
            eWbTv.setText(mActivity.getAndroidContext().getString(
                    SettingsList.WHITE_BALANCE_DISPLAY[progress]));
        }

        if (eContrastSeekBar != null && seekBar == eContrastSeekBar) {
            currentDataModule.set(Keys.KEY_CAMERA_CONTRAST,
                    SettingsList.CONTRAST[progress]);
            mBasicModule.updateParametersContrast();
            mBasicModule.applySettings();
        }

        if (eSaturationSeekBar != null && seekBar == eSaturationSeekBar) {
            currentDataModule.set(Keys.KEY_CAMERA_SATURATION,
                    SettingsList.SATURATION[progress]);
            mBasicModule.updateParametersSaturation();
            mBasicModule.applySettings();
        }

        if (eBrightnessSeekBar != null && seekBar == eBrightnessSeekBar) {
            currentDataModule.set(Keys.KEY_CAMERA_BRIGHTNESS,
                    SettingsList.BRIGHTNESS[progress]);
            mBasicModule.updateParametersBrightness();
            mBasicModule.applySettings();
        }
        changeColorStyleDescription();
    }

    private void changeColorStyleDescription() {
        DataModuleBasic currentDataModule = DataModuleManager.getInstance(
                mActivity).getDataModulePhoto();
        // Monkey crash happened on 6-23 020
        // currentDataModule.getString(Keys.KEY_CAMERA_CONTRAST) may be null
        if (SettingsList.CONTRAST[SettingsList.CONTRAST_DEFAULT].equals(currentDataModule
                .getString(Keys.KEY_CAMERA_CONTRAST))
                && SettingsList.SATURATION[SettingsList.SATURATION_DEFAULT]
                        .equals(currentDataModule
                                .getString(Keys.KEY_CAMERA_SATURATION))
                && SettingsList.BRIGHTNESS[SettingsList.BRIGHTNESS_DEFAULT]
                        .equals(currentDataModule
                                .getString(Keys.KEY_CAMERA_BRIGHTNESS))) {
            eStyleTv.setText(R.string.extend_panel_style_default);
        } else {
            eStyleTv.setText(R.string.extend_panel_style_customize);
        }

    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
    }

    @Override
    public void onSettingReset() {
        /*
        if (eExposureSeekBar != null) {
            updateExposureUI();//SPRD:fix bug622818
            //mBasicModule.updateParametersExposureCompensation();
        }

        if (eIsoSeekBar != null) {
            String iso = DataModuleManager.getInstance(mActivity)
                    .getDataModulePhoto().getString(Keys.KEY_CAMERA_ISO);
            int isoIndex = SettingsList.indexOf(iso, SettingsList.ISO,
                    SettingsList.ISO_DEFAULT);

            eIsoTv.setText(mActivity.getAndroidContext().getString(
                    SettingsList.ISO_DISPLAY[isoIndex]));

            eIsoSeekBar.setProgress(isoIndex);
            //mBasicModule.updateParametersISO();
        }

        if (eWbSeekBar != null) {
            String wb = DataModuleManager.getInstance(mActivity)
                    .getDataModulePhoto().getString(Keys.KEY_WHITE_BALANCE);
            int wbIndex = SettingsList.indexOf(wb, SettingsList.WHITE_BALANCE,
                    SettingsList.WHITE_BALANCE_DEFAULT);

            eWbTv.setText(mActivity.getAndroidContext().getString(
                    SettingsList.WHITE_BALANCE_DISPLAY[wbIndex]));

            eWbSeekBar.setProgress(wbIndex);
            //mBasicModule.updateParametersWhiteBalance();
        }

        if (eContrastSeekBar != null) {
            String contrast = DataModuleManager.getInstance(mActivity)
                    .getDataModulePhoto().getString(Keys.KEY_CAMERA_CONTRAST);
            int contrastIndex = SettingsList.indexOf(contrast,
                    SettingsList.CONTRAST, SettingsList.CONTRAST_DEFAULT);

            eContrastSeekBar.setProgress(contrastIndex);
            //mBasicModule.updateParametersContrast();
        }

        if (eSaturationSeekBar != null) {
            String saturation = DataModuleManager.getInstance(mActivity)
                    .getDataModulePhoto().getString(Keys.KEY_CAMERA_SATURATION);
            int saturatioIndex = SettingsList.indexOf(saturation,
                    SettingsList.SATURATION, SettingsList.SATURATION_DEFAULT);

            eSaturationSeekBar.setProgress(saturatioIndex);
            //mBasicModule.updateParametersSaturation();
        }

        if (eBrightnessSeekBar != null) {
            String brightness = DataModuleManager.getInstance(mActivity)
                    .getDataModulePhoto().getString(Keys.KEY_CAMERA_BRIGHTNESS);
            int brightnessIndex = SettingsList.indexOf(brightness,
                    SettingsList.BRIGHTNESS, SettingsList.BRIGHTNESS_DEFAULT);

            eBrightnessSeekBar.setProgress(brightnessIndex);
            //mBasicModule.updateParametersBrightness();
        }
        //mBasicModule.applySettings();
        eStyleTv.setText(R.string.extend_panel_style_default);
        */
    }

    private int convertExposureValueToIndex(int exposure) {
        if (mExposureCompensationStep == 0.0f) {
            return 0;
        }
        return (int)(exposure * mExposureCompensationStep) + 3;
    }
}
