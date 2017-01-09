
package com.dream.camera.dreambasemodules;

import android.view.ViewGroup;

import com.android.camera.settings.SettingsManager;

public interface DreamInterface {

    // Generate a view to fit top panel.
    public void fitTopPanel(ViewGroup topPanelParent);

    // Update visibilities of state icons on side panel.
    public void updateSidePanel();

    // Generate views to fit extend panel.
    public void fitExtendPanel(ViewGroup extendPanelParent);

    // Update icons on bottom panel.
    public void updateBottomPanel();

    // Update item on slide panel.
    public void updateSlidePanel();

    public int getSidePanelMask();

    public void onSettingChanged(SettingsManager settingsManager, String key);
}
