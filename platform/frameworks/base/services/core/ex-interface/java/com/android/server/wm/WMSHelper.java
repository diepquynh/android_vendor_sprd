package com.android.server.wm;

public class WMSHelper {

  /*
  * SPRD: Add for bug 605052 avoid pressing the home key or the menu key to exit camera,
  * the screen will light when enter the camera again.{@
  */
  boolean forceNotSaveSurface(WindowState ws) {
    return false;
  }
  /* @{ */
}
