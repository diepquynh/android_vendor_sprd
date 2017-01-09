/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.ucamera.ucam.modules;

import com.android.camera.CameraActivity;
import com.android.camera.CameraModule;
import android.view.KeyEvent;
import com.android.camera.PhotoController;
import com.android.camera.app.AppController;
import com.android.camera.app.CameraAppUI;
import com.android.camera.debug.Log;
import com.android.camera.hardware.HardwareSpec;
import android.view.View;
import com.android.camera.module.ModuleController;
import com.android.camera.settings.SettingsManager;
import com.android.camera.ui.TouchCoordinate;
import com.android.ex.camera2.portability.CameraAgent.CameraProxy;
import com.ucamera.ucam.modules.ui.BasicUI;


public abstract class BasicModule extends CameraModule
        implements ModuleController {

    /**
     * Constructs a new photo module.
     */
    public BasicModule(AppController app) {
        super(app);
    }

    public void init(CameraActivity activity, boolean isSecureCamera, boolean isCaptureIntent) {}
    public void resume() {}
    public void destroy() {}
    public void pause() {}
    public boolean onBackPressed() {return false;}
    public void onCameraAvailable(CameraProxy cameraProxy) {}
    public CameraAppUI.BottomBarUISpec getBottomBarSpec() {return null;}
    public void hardResetSettings(SettingsManager settingsManager) {}
    public boolean isUsingBottomBar() {return false;}
    public void onPreviewVisibilityChanged(int visibility) {}
    public void onShutterCoordinate(TouchCoordinate coord) {}
    public HardwareSpec getHardwareSpec() {return null;}
    public void onLayoutOrientationChanged(boolean isLandscape) {}
    public void initializeModuleControls(){}
    public void onShutterButtonFocus(boolean pressed){}
    public void onShutterButtonClick(){}
    public void onShutterButtonLongPressed(){}
    public boolean onKeyDown(int keyCode, KeyEvent event){return false;}
    public boolean onKeyUp(int keyCode, KeyEvent event){return false;}
    public String getPeekAccessibilityString(){return null;}
    public void onStopRecordVoiceClicked(View v) {
    }
    public BasicUI getUcamUI(){return null;}
}
