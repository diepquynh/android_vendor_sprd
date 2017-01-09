
package com.dream.camera.modules.filterdream;

import com.android.camera.app.AppController;
import com.android.camera.debug.Log;
import com.android.camera.ui.MainActivityLayout;

public class DreamFilterModuleController {
    private static final Log.Tag TAG = new Log.Tag("DreamFilterModuleController");

    public DreamFilterModuleController(AppController controller, MainActivityLayout appRootView) {}

    public boolean switchMode(int swipeState) {
        return false;
    }

    public void checkFrameCount() {}

    public void resetFrameCount() {}
}
