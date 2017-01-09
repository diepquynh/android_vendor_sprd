/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.android.server.wm;

import static com.android.server.wm.WindowManagerDebugConfig.DEBUG_STARTING_WINDOW;
import static com.android.server.wm.WindowManagerDebugConfig.DEBUG_WINDOW_MOVEMENT;
import static com.android.server.wm.WindowManagerDebugConfig.DEBUG_ADD_REMOVE;
import static com.android.server.wm.WindowManagerDebugConfig.DEBUG_SCREENSHOT;
import static com.android.server.wm.WindowManagerDebugConfig.TAG_WM;
import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION;

import android.Manifest;
import android.content.Context;
import android.content.res.CompatibilityInfo;
import android.content.Intent;
import android.os.IBinder;
import android.os.Binder;
import android.os.Message;
import android.os.Trace;
import android.util.Slog;
import android.view.WindowManager.LayoutParams;
import android.view.WindowManager;
import android.view.Surface;
import android.view.SurfaceControl;
import android.view.Display;
import android.view.DisplayInfo;
import android.graphics.Rect;
import android.graphics.Bitmap;

import com.android.server.input.InputManagerService;
import com.android.server.wm.WindowManagerService;
import com.android.server.DebugController;
import com.android.server.AttributeCache;
import com.android.server.power.ShutdownThread;

import java.io.FileDescriptor;
import java.io.PrintWriter;

/** {@hide} */
public class SprdWindowManagerService extends WindowManagerService {
    SprdWindowManagerService(Context context, InputManagerService inputManager,
            boolean haveInputMethods, boolean showBootMsgs, boolean onlyCore) {
        super(context, inputManager, haveInputMethods, showBootMsgs, onlyCore);
        Slog.i(TAG, "init SprdWindowManagerService");
    }
    @Override
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        WMSLog mWMSLog = WMSFactory.getLogInstance();
        if (mWMSLog.changeDebugFlag(args)) {
            return;
        }
        super.dump(fd, pw, args);
    }

    //add for Bug#618149  begin
    public boolean setAppStartingWindow(IBinder token, String pkg,
            int theme, CompatibilityInfo compatInfo,
            CharSequence nonLocalizedLabel, int labelRes, int icon, int logo,
            int windowFlags, IBinder transferFrom, boolean createIfNeeded, Intent intent, boolean fullscreen){
        synchronized(mWindowMap) {
            if (DEBUG_STARTING_WINDOW) Slog.v(
                    TAG_WM, "setAppStartingWindow: token=" + token + " pkg=" + pkg
                    + " transferFrom=" + transferFrom);

            AppWindowToken wtoken = findAppWindowToken(token);
            if (wtoken == null) {
                Slog.w(TAG_WM, "Attempted to set icon of non-existing app token: " + token);
                return false;
            }

            // If the display is frozen, we won't do anything until the
            // actual window is displayed so there is no reason to put in
            // the starting window.
            if (!okToDisplay()) {
                return false;
            }

            if (wtoken.startingData != null) {
                return false;
            }

            // If this is a translucent window, then don't
            // show a starting window -- the current effect (a full-screen
            // opaque starting window that fades away to the real contents
            // when it is ready) does not work for this.
            if (DEBUG_STARTING_WINDOW) Slog.v(TAG_WM, "Checking theme of starting window: 0x"
                    + Integer.toHexString(theme));
            if (theme != 0) {
                AttributeCache.Entry ent = AttributeCache.instance().get(pkg, theme,
                        com.android.internal.R.styleable.Window, mCurrentUserId);
                if (ent == null) {
                    // Whoops!  App doesn't exist.  Um.  Okay.  We'll just
                    // pretend like we didn't see that.
                    return false;
                }
                final boolean windowIsTranslucent = ent.array.getBoolean(
                        com.android.internal.R.styleable.Window_windowIsTranslucent, false);
                final boolean windowIsFloating = ent.array.getBoolean(
                        com.android.internal.R.styleable.Window_windowIsFloating, false);
                final boolean windowShowWallpaper = ent.array.getBoolean(
                        com.android.internal.R.styleable.Window_windowShowWallpaper, false);
                final boolean windowDisableStarting = ent.array.getBoolean(
                        com.android.internal.R.styleable.Window_windowDisablePreview, false);
                if (DEBUG_STARTING_WINDOW) Slog.v(TAG_WM, "Translucent=" + windowIsTranslucent
                        + " Floating=" + windowIsFloating
                        + " ShowWallpaper=" + windowShowWallpaper);
                if (windowIsTranslucent) {
                    return false;
                }
                if (windowIsFloating || windowDisableStarting) {
                    return false;
                }
                if (windowShowWallpaper) {
                    if (mWallpaperControllerLocked.getWallpaperTarget() == null) {
                        // If this theme is requesting a wallpaper, and the wallpaper
                        // is not currently visible, then this effectively serves as
                        // an opaque window and our starting window transition animation
                        // can still work.  We just need to make sure the starting window
                        // is also showing the wallpaper.
                        windowFlags |= WindowManager.LayoutParams.FLAG_SHOW_WALLPAPER;
                    } else {
                        return false;
                    }
                }
            }
            if (transferStartingWindow(transferFrom, wtoken, intent)) {
                return true;
            }

            // There is no existing starting window, and the caller doesn't
            // want us to create one, so that's it!
            if (!createIfNeeded) {
                return false;
            }

            if (DEBUG_STARTING_WINDOW) Slog.v(TAG_WM, "Creating StartingData");
            wtoken.startingData = new StartingData(pkg, theme, compatInfo, nonLocalizedLabel,
                    labelRes, icon, logo, windowFlags);
            wtoken.startingIntent = intent;
            wtoken.fullscreen = fullscreen;
            Message m = mH.obtainMessage(H.ADD_STARTING, wtoken);
            // Note: we really want to do sendMessageAtFrontOfQueue() because we
            // want to process the message ASAP, before any other queued
            // messages.
            if (DEBUG_STARTING_WINDOW) Slog.v(TAG_WM, "Enqueueing ADD_STARTING");

            mH.sendMessageAtFrontOfQueue(m);

        }
        return true;
    }
    private boolean transferStartingWindow(IBinder transferFrom, AppWindowToken wtoken, Intent intent) {
        if (transferFrom == null) {
            return false;
        }
        AppWindowToken ttoken = findAppWindowToken(transferFrom);
        if (ttoken == null) {
            return false;
        }
        WindowState startingWindow = ttoken.startingWindow;
        if (startingWindow != null && ttoken.startingView != null) {
            // In this case, the starting icon has already been displayed, so start
            // letting windows get shown immediately without any more transitions.
            mSkipAppTransitionAnimation = true;

            if (DEBUG_STARTING_WINDOW) Slog.v(TAG_WM,
                    "Moving existing starting " + startingWindow + " from " + ttoken
                            + " to " + wtoken);
            final long origId = Binder.clearCallingIdentity();

            // Transfer the starting window over to the new token.
            wtoken.startingData = ttoken.startingData;
            wtoken.startingView = ttoken.startingView;
            wtoken.startingDisplayed = ttoken.startingDisplayed;
            ttoken.startingDisplayed = false;
            wtoken.startingWindow = startingWindow;
            wtoken.reportedVisible = ttoken.reportedVisible;
            ttoken.startingData = null;
            ttoken.startingView = null;
            ttoken.startingWindow = null;
            ttoken.startingMoved = true;
            startingWindow.mToken = wtoken;
            startingWindow.mRootToken = wtoken;
            startingWindow.mAppToken = wtoken;

            if (DEBUG_WINDOW_MOVEMENT || DEBUG_ADD_REMOVE || DEBUG_STARTING_WINDOW) {
                Slog.v(TAG_WM, "Removing starting window: " + startingWindow);
            }
            startingWindow.getWindowList().remove(startingWindow);
            mWindowsChanged = true;
            if (DEBUG_ADD_REMOVE) Slog.v(TAG_WM,
                    "Removing starting " + startingWindow + " from " + ttoken);
            ttoken.windows.remove(startingWindow);
            ttoken.allAppWindows.remove(startingWindow);
            addWindowToListInOrderLocked(startingWindow, true);

            // Propagate other interesting state between the
            // tokens.  If the old token is displayed, we should
            // immediately force the new one to be displayed.  If
            // it is animating, we need to move that animation to
            // the new one.
            if (ttoken.allDrawn) {
                wtoken.allDrawn = true;
                wtoken.deferClearAllDrawn = ttoken.deferClearAllDrawn;
            }
            if (ttoken.firstWindowDrawn) {
                wtoken.firstWindowDrawn = true;
            }
            if (!ttoken.hidden) {
                wtoken.hidden = false;
                wtoken.hiddenRequested = false;
            }
            if (wtoken.clientHidden != ttoken.clientHidden) {
                wtoken.clientHidden = ttoken.clientHidden;
                wtoken.sendAppVisibilityToClients();
            }
            ttoken.mAppAnimator.transferCurrentAnimation(
                    wtoken.mAppAnimator, startingWindow.mWinAnimator);

            updateFocusedWindowLocked(UPDATE_FOCUS_WILL_PLACE_SURFACES,
                    true /*updateInputWindows*/);
            getDefaultDisplayContentLocked().layoutNeeded = true;
            mWindowPlacerLocked.performSurfacePlacement();
            Binder.restoreCallingIdentity(origId);

            return true;
        } else if (ttoken.startingData != null) {
            // The previous app was getting ready to show a
            // starting window, but hasn't yet done so.  Steal it!
            if (DEBUG_STARTING_WINDOW) Slog.v(TAG_WM, "Moving pending starting from " + ttoken
                    + " to " + wtoken);
            wtoken.startingData = ttoken.startingData;
            ttoken.startingData = null;
            ttoken.startingMoved = true;
            wtoken.startingIntent = intent;
            Message m = mH.obtainMessage(H.ADD_STARTING, wtoken);
            // Note: we really want to do sendMessageAtFrontOfQueue() because we
            // want to process the message ASAP, before any other queued
            // messages.
            mH.sendMessageAtFrontOfQueue(m);
            return true;
        }
        final AppWindowAnimator tAppAnimator = ttoken.mAppAnimator;
        final AppWindowAnimator wAppAnimator = wtoken.mAppAnimator;
        if (tAppAnimator.thumbnail != null) {
            // The old token is animating with a thumbnail, transfer that to the new token.
            if (wAppAnimator.thumbnail != null) {
                wAppAnimator.thumbnail.destroy();
            }
            wAppAnimator.thumbnail = tAppAnimator.thumbnail;
            wAppAnimator.thumbnailLayer = tAppAnimator.thumbnailLayer;
            wAppAnimator.thumbnailAnimation = tAppAnimator.thumbnailAnimation;
            tAppAnimator.thumbnail = null;
        }
        return false;
    }

    /** {@hide} */
    public Bitmap screenshotApplicationsEx(IBinder appToken, int displayId, int width, int height,
            float frameScale) {
        try {
            Trace.traceBegin(Trace.TRACE_TAG_WINDOW_MANAGER, "screenshotApplicationsEx");
            return screenshotApplicationsInnerEx(appToken, displayId, width, height, false,
                    frameScale, Bitmap.Config.RGB_565);
        } finally {
            Trace.traceEnd(Trace.TRACE_TAG_WINDOW_MANAGER);
        }
    }

    /** {@hide} */
    Bitmap screenshotApplicationsInnerEx(IBinder appToken, int displayId, int width, int height,
            boolean includeFullDisplay, float frameScale, Bitmap.Config config) {
        final DisplayContent displayContent;
        synchronized(mWindowMap) {
            displayContent = getDisplayContentLocked(displayId);
            if (displayContent == null) {
                if (DEBUG_SCREENSHOT) Slog.i(TAG_WM, "Screenshot of " + appToken
                        + ": returning null. No Display for displayId=" + displayId);
                return null;
            }
        }
        final DisplayInfo displayInfo = displayContent.getDisplayInfo();
        int dw = displayInfo.logicalWidth;
        int dh = displayInfo.logicalHeight;
        if (dw == 0 || dh == 0) {
            if (DEBUG_SCREENSHOT) Slog.i(TAG_WM, "Screenshot of " + appToken
                    + ": returning null. logical widthxheight=" + dw + "x" + dh);
            return null;
        }

        Bitmap bm = null;

        int maxLayer = 0;
        final Rect frame = new Rect();
        final Rect stackBounds = new Rect();

        boolean screenshotReady;
        int minLayer;
        if (appToken == null) {
            screenshotReady = true;
            minLayer = 0;
        } else {
            screenshotReady = false;
            minLayer = Integer.MAX_VALUE;
        }

        WindowState appWin = null;

        boolean includeImeInScreenshot;
        synchronized(mWindowMap) {
            final AppWindowToken imeTargetAppToken =
                    mInputMethodTarget != null ? mInputMethodTarget.mAppToken : null;
            // We only include the Ime in the screenshot if the app we are screenshoting is the IME
            // target and isn't in multi-window mode. We don't screenshot the IME in multi-window
            // mode because the frame of the IME might not overlap with that of the app.
            // E.g. IME target app at the top in split-screen mode and the IME at the bottom
            // overlapping with the bottom app.
            includeImeInScreenshot = imeTargetAppToken != null
                    && imeTargetAppToken.appToken != null
                    && imeTargetAppToken.appToken.asBinder() == appToken
                    && !mInputMethodTarget.isInMultiWindowMode();
        }

        final int aboveAppLayer = (mPolicy.windowTypeToLayerLw(TYPE_APPLICATION) + 1)
                * TYPE_LAYER_MULTIPLIER + TYPE_LAYER_OFFSET;

        synchronized(mWindowMap) {
            // Figure out the part of the screen that is actually the app.
            appWin = null;
            final WindowList windows = displayContent.getWindowList();
            for (int i = windows.size() - 1; i >= 0; i--) {
                WindowState ws = windows.get(i);
                if (!ws.mHasSurface) {
                    continue;
                }
                if (ws.mLayer >= aboveAppLayer) {
                    continue;
                }
                if (ws.mIsImWindow) {
                    if (!includeImeInScreenshot) {
                        continue;
                    }
                } else if (ws.mIsWallpaper) {
                    if (appWin == null) {
                        // We have not ran across the target window yet, so it is probably
                        // behind the wallpaper. This can happen when the keyguard is up and
                        // all windows are moved behind the wallpaper. We don't want to
                        // include the wallpaper layer in the screenshot as it will coverup
                        // the layer of the target window.
                        continue;
                    }
                    // Fall through. The target window is in front of the wallpaper. For this
                    // case we want to include the wallpaper layer in the screenshot because
                    // the target window might have some transparent areas.
                } else if (appToken != null) {
                    if (ws.mAppToken == null || ws.mAppToken.token != appToken) {
                        // This app window is of no interest if it is not associated with the
                        // screenshot app.
                        continue;
                    }
                    appWin = ws;
                }

                // Include this window.

                final WindowStateAnimator winAnim = ws.mWinAnimator;
                int layer = winAnim.mSurfaceController.getLayer();
                if (maxLayer < layer) {
                    maxLayer = layer;
                }
                if (minLayer > layer) {
                    minLayer = layer;
                }

                // Don't include wallpaper in bounds calculation
                if (!includeFullDisplay && !ws.mIsWallpaper) {
                    final Rect wf = ws.mFrame;
                    final Rect cr = ws.mContentInsets;
                    int left = wf.left + cr.left;
                    int top = wf.top + cr.top;
                    int right = wf.right - cr.right;
                    int bottom = wf.bottom - cr.bottom;
                    frame.union(left, top, right, bottom);
                    ws.getVisibleBounds(stackBounds);
                    if (!Rect.intersects(frame, stackBounds)) {
                        // Set frame empty if there's no intersection.
                        frame.setEmpty();
                    }
                }

                if (ws.mAppToken != null && ws.mAppToken.token == appToken &&
                        ws.isDisplayedLw() && winAnim.getShown()) {
                    screenshotReady = true;
                }

                if (ws.isObscuringFullscreen(displayInfo)){
                    break;
                }
            }

            if (appToken != null && appWin == null) {
                // Can't find a window to snapshot.
                if (DEBUG_SCREENSHOT) Slog.i(TAG_WM,
                        "Screenshot: Couldn't find a surface matching " + appToken);
                return null;
            }

            if (!screenshotReady) {
                Slog.i(TAG_WM, "Failed to capture screenshot of " + appToken +
                        " appWin=" + (appWin == null ? "null" : (appWin + " drawState=" +
                        appWin.mWinAnimator.mDrawState)));
                return null;
            }

            // Screenshot is ready to be taken. Everything from here below will continue
            // through the bottom of the loop and return a value. We only stay in the loop
            // because we don't want to release the mWindowMap lock until the screenshot is
            // taken.

            if (maxLayer == 0) {
                if (DEBUG_SCREENSHOT) Slog.i(TAG_WM, "Screenshot of " + appToken
                        + ": returning null maxLayer=" + maxLayer);
                return null;
            }

            if (!includeFullDisplay) {
                // Constrain frame to the screen size.
                if (!frame.intersect(0, 0, dw, dh)) {
                    frame.setEmpty();
                }
            } else {
                // Caller just wants entire display.
                frame.set(0, 0, dw, dh);
            }
            if (frame.isEmpty()) {
                return null;
            }

            if (width < 0) {
                width = (int) (frame.width() * frameScale);
            }
            if (height < 0) {
                height = (int) (frame.height() * frameScale);
            }

            // Tell surface flinger what part of the image to crop. Take the top
            // right part of the application, and crop the larger dimension to fit.
            Rect crop = new Rect(frame);
            //modify forBug#618149 begin
            if (appWin != null && appWin.mAppToken != null && appWin.mAppToken.startingIntent !=null) {
                Slog.d(TAG_WM,"skep crop...");
            } else {
                if (width / (float) frame.width() < height / (float) frame.height()) {
                    int cropWidth = (int)((float)width / (float)height * frame.height());
                    crop.right = crop.left + cropWidth;
                } else {
                    int cropHeight = (int)((float)height / (float)width * frame.width());
                    crop.bottom = crop.top + cropHeight;
                }
            }
            //modify for Bug#618149 end

            // The screenshot API does not apply the current screen rotation.
            int rot = getDefaultDisplayContentLocked().getDisplay().getRotation();

            if (rot == Surface.ROTATION_90 || rot == Surface.ROTATION_270) {
                rot = (rot == Surface.ROTATION_90) ? Surface.ROTATION_270 : Surface.ROTATION_90;
            }

            // Surfaceflinger is not aware of orientation, so convert our logical
            // crop to surfaceflinger's portrait orientation.
            convertCropForSurfaceFlinger(crop, rot, dw, dh);

            if (DEBUG_SCREENSHOT) {
                Slog.i(TAG_WM, "Screenshot: " + dw + "x" + dh + " from " + minLayer + " to "
                        + maxLayer + " appToken=" + appToken);
                for (int i = 0; i < windows.size(); i++) {
                    WindowState win = windows.get(i);
                    WindowSurfaceController controller = win.mWinAnimator.mSurfaceController;
                    Slog.i(TAG_WM, win + ": " + win.mLayer
                            + " animLayer=" + win.mWinAnimator.mAnimLayer
                            + " surfaceLayer=" + ((controller == null)
                                ? "null" : controller.getLayer()));
                }
            }

            ScreenRotationAnimation screenRotationAnimation =
                    mAnimator.getScreenRotationAnimationLocked(Display.DEFAULT_DISPLAY);
            final boolean inRotation = screenRotationAnimation != null &&
                    screenRotationAnimation.isAnimating();
            if (DEBUG_SCREENSHOT && inRotation) Slog.v(TAG_WM,
                    "Taking screenshot while rotating");

            // We force pending transactions to flush before taking
            // the screenshot by pushing an empty synchronous transaction.
            SurfaceControl.openTransaction();
            SurfaceControl.closeTransactionSync();

            bm = SurfaceControl.screenshot(crop, width, height, minLayer, (appWin != null && appWin.mAppToken != null && appWin.mAppToken.startingIntent !=null)? minLayer + 1: maxLayer, //modify for Bug#618149
                    inRotation, rot);
            if (bm == null) {
                Slog.w(TAG_WM, "Screenshot failure taking screenshot for (" + dw + "x" + dh
                        + ") to layer " + maxLayer);
                return null;
            }
        }

        if (DEBUG_SCREENSHOT) {
            // TEST IF IT's ALL BLACK
            int[] buffer = new int[bm.getWidth() * bm.getHeight()];
            bm.getPixels(buffer, 0, bm.getWidth(), 0, 0, bm.getWidth(), bm.getHeight());
            boolean allBlack = true;
            final int firstColor = buffer[0];
            for (int i = 0; i < buffer.length; i++) {
                if (buffer[i] != firstColor) {
                    allBlack = false;
                    break;
                }
            }
            if (allBlack) {
                Slog.i(TAG_WM, "Screenshot " + appWin + " was monochrome(" +
                        Integer.toHexString(firstColor) + ")! mSurfaceLayer=" +
                        (appWin != null ?
                                appWin.mWinAnimator.mSurfaceController.getLayer() : "null") +
                        " minLayer=" + minLayer + " maxLayer=" + maxLayer);
            }
        }

        // Create a copy of the screenshot that is immutable and backed in ashmem.
        // This greatly reduces the overhead of passing the bitmap between processes.
        Bitmap ret = bm.createAshmemBitmap(config);
        bm.recycle();
        return ret;
    }

    //add for Bug#618149  end

    /* SPRD: add shutdown reason for PhoneInfo feature @{ */
    @Override
    public void shutdown(boolean confirm,String reason) {
        ShutdownThread.shutdown(mContext, reason, confirm);
    }
    /* @} */

    /* SPRD: add shutdown reason for PhoneInfo feature @{ */
    @Override
    public void rebootSafeMode(boolean confirm,String reason) {
        ShutdownThread.rebootSafeMode(mContext, confirm,reason);
    }
    /* @} */
}
