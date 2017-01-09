package com.android.server.wm;

import static com.android.server.wm.WindowManagerDebugConfig.TAG_WM;
import android.util.Slog;

public class WMSHelperEx extends WMSHelper {

  /*
  * SPRD: Add for bug 605052 avoid pressing the home key or the menu key to exit camera,
  * the screen will light when enter the camera again.{@
  */
  boolean forceNotSaveSurface(WindowState ws) {
    String wmname = ws.getWindowTag().toString();
    for (String apkname: NOT_SAVE_SURFACE_APPS) {
      if (wmname != null && wmname.contains(apkname)) {
         Slog.i(TAG_WM, "force not save surface for : " + wmname);
         return true;
      }
    }
    return false;
  }

  public static final String[] NOT_SAVE_SURFACE_APPS = {
     "com.android.camera2",
     "com.android.gallery3d"
  };
  /* @{ */
}
