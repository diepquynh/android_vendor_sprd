/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.android.gallery3d.app;

import android.Manifest;
import android.app.Activity;
import android.app.Dialog;
import android.appwidget.AppWidgetManager;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.pm.PackageManager;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Images.ImageColumns;
import android.provider.MediaStore.Video;
import android.util.Log;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;

import com.android.gallery3d.R;
import com.android.gallery3d.common.Utils;
import com.android.gallery3d.data.DataManager;
import com.android.gallery3d.data.MediaItem;
import com.android.gallery3d.data.MediaSet;
import com.android.gallery3d.data.Path;
import com.android.gallery3d.gadget.PhotoAppWidgetProvider;
import com.android.gallery3d.picasasource.PicasaSource;
import com.android.gallery3d.util.GalleryUtils;
import com.sprd.gallery3d.app.GalleryPermissionsActivity;
import com.sprd.gallery3d.app.PermissionsActivity;
import com.sprd.gallery3d.app.PickPhotosPermissionsActivity;
import com.sprd.gallery3d.drm.GalleryActivityUtils;
import com.sprd.gallery3d.tools.LargeImageProcessingUtils;

public final class GalleryActivity extends AbstractGalleryActivity implements OnCancelListener {
    public static final String EXTRA_SLIDESHOW = "slideshow";
    public static final String EXTRA_DREAM = "dream";
    public static final String EXTRA_CROP = "crop";

    public static final String ACTION_REVIEW = "com.android.camera.action.REVIEW";
    public static final String KEY_GET_CONTENT = "get-content";
    public static final String KEY_GET_ALBUM = "get-album";
    public static final String KEY_TYPE_BITS = "type-bits";
    public static final String KEY_MEDIA_TYPES = "mediaTypes";
    public static final String KEY_DISMISS_KEYGUARD = "dismiss-keyguard";
    public static final String MIME_TYPE = "mime_type";
    public static final String DATA = "_data";

    private static final String TAG = "GalleryActivity";
    private Dialog mVersionCheckDialog;
    private static GalleryActivity sLastActivity;
    private boolean mHasCriticalPermissions;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        /* SPRD: Modify 20150609 Spreadst of bug444059, avoid ANR in monkey test cause by too many threads run @{ */
        if (GalleryUtils.isMonkey()) {
            if (sLastActivity != null) {
                Log.e(TAG, "GalleryActivity in monkey test -> last activity is not finished! ");
                sLastActivity.finish();
                if (sLastActivity.getGLRoot() != null && sLastActivity.getGLRoot().isFreezed()) {
                    Log.d(TAG, "GLRootView for last activity is unfreezed");
                    sLastActivity.getGLRoot().unfreeze();
                }
            }
        }
        /* @} */
        requestWindowFeature(Window.FEATURE_ACTION_BAR);
        requestWindowFeature(Window.FEATURE_ACTION_BAR_OVERLAY);

        if (getIntent().getBooleanExtra(KEY_DISMISS_KEYGUARD, false)) {
            getWindow().addFlags(
                    WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD);
        }

        setContentView(R.layout.main);
        /* SPRD: bug 631765,in MultiWindowMode,not to check permissions @} */
        if (this.isInMultiWindowMode()) {
            Log.d(TAG, " onCreate , MultiWindowMode .");
            GalleryUtils.killActivityInMultiWindow(this, getToastFlag());
            return;
        }
        /* @} */
        /* SPRD: add check gallery permissions @{*/
        checkPermissions();
        if (!mHasCriticalPermissions) {
            Log.v(TAG, "onCreate: Missing critical permissions.");
            finish();
            return;
        }
        /* @} */
        if (savedInstanceState != null) {
            getStateManager().restoreFromState(savedInstanceState);
        } else {
            initializeByIntent();
        }
        // SPRD:bug575864,open Gallery check widget,need update or not
        checkUpdateWidget();
    }

    private void initializeByIntent() {
        Intent intent = getIntent();
        String action = intent.getAction();

        if (Intent.ACTION_GET_CONTENT.equalsIgnoreCase(action)) {
            startGetContent(intent);
        } else if (Intent.ACTION_PICK.equalsIgnoreCase(action)) {
            // We do NOT really support the PICK intent. Handle it as
            // the GET_CONTENT. However, we need to translate the type
            // in the intent here.
            Log.w(TAG, "action PICK is not supported");
            String type = Utils.ensureNotNull(intent.getType());
            if (type.startsWith("vnd.android.cursor.dir/")) {
                if (type.endsWith("/image")) intent.setType("image/*");
                if (type.endsWith("/video")) intent.setType("video/*");
            }
            startGetContent(intent);
        } else if (Intent.ACTION_VIEW.equalsIgnoreCase(action)
                || ACTION_REVIEW.equalsIgnoreCase(action)){
            startViewAction(intent);
        } else {
            startDefaultPage();
        }
    }

    public void startDefaultPage() {
        PicasaSource.showSignInReminder(this);
        Bundle data = new Bundle();
        data.putString(AlbumSetPage.KEY_MEDIA_PATH,
                getDataManager().getTopSetPath(DataManager.INCLUDE_ALL));
        getStateManager().startState(AlbumSetPage.class, data);
        mVersionCheckDialog = PicasaSource.getVersionCheckDialog(this);
        if (mVersionCheckDialog != null) {
            mVersionCheckDialog.setOnCancelListener(this);
        }
    }

    private void startGetContent(Intent intent) {
        Bundle data = intent.getExtras() != null
                ? new Bundle(intent.getExtras())
                : new Bundle();
        /* SPRD: Drm feature start @{ */
        GalleryActivityUtils.getInstance().startGetContentSetAs(intent, data);
        /* SPRD: Drm feature end @} */
        data.putBoolean(KEY_GET_CONTENT, true);
        int typeBits = GalleryUtils.determineTypeBits(this, intent);
        data.putInt(KEY_TYPE_BITS, typeBits);
        data.putString(AlbumSetPage.KEY_MEDIA_PATH,
                getDataManager().getTopSetPath(typeBits));
        getStateManager().startState(AlbumSetPage.class, data);
    }

    private String getContentType(Intent intent) {
        String type = intent.getType();
        if (type != null) {
            return GalleryUtils.MIME_TYPE_PANORAMA360.equals(type)
                ? MediaItem.MIME_TYPE_JPEG : type;
        }

        Uri uri = intent.getData();
        try {
            return LargeImageProcessingUtils.getType(uri, this);
            //SPRD: remove for bug 500764
//            return getContentResolver().getType(uri);
        } catch (Throwable t) {
            Log.w(TAG, "get type fail", t);
            return null;
        }
    }

    private void startViewAction(Intent intent) {
        Boolean slideshow = intent.getBooleanExtra(EXTRA_SLIDESHOW, false);
        if (slideshow) {
            getActionBar().hide();
            DataManager manager = getDataManager();
            Path path = manager.findPathByUri(intent.getData(), intent.getType());
            if (path == null || manager.getMediaObject(path)
                    instanceof MediaItem) {
                path = Path.fromString(
                        manager.getTopSetPath(DataManager.INCLUDE_IMAGE));
            }
            Bundle data = new Bundle();
            data.putString(SlideshowPage.KEY_SET_PATH, path.toString());
            data.putBoolean(SlideshowPage.KEY_RANDOM_ORDER, true);
            data.putBoolean(SlideshowPage.KEY_REPEAT, true);
            if (intent.getBooleanExtra(EXTRA_DREAM, false)) {
                data.putBoolean(SlideshowPage.KEY_DREAM, true);
            }
            getStateManager().startState(SlideshowPage.class, data);
        } else {
            Bundle data = new Bundle();
            DataManager dm = getDataManager();
            Uri uri = intent.getData();
            String contentType = getContentType(intent);
            if (contentType == null) {
                Toast.makeText(this,
                        R.string.no_such_item, Toast.LENGTH_LONG).show();
                finish();
                return;
            }
            if (uri == null) {
                int typeBits = GalleryUtils.determineTypeBits(this, intent);
                data.putInt(KEY_TYPE_BITS, typeBits);
                data.putString(AlbumSetPage.KEY_MEDIA_PATH,
                        getDataManager().getTopSetPath(typeBits));
                getStateManager().startState(AlbumSetPage.class, data);
            } else if (contentType.startsWith(
                    ContentResolver.CURSOR_DIR_BASE_TYPE)) {
                int mediaType = intent.getIntExtra(KEY_MEDIA_TYPES, 0);
                if (mediaType != 0) {
                    uri = uri.buildUpon().appendQueryParameter(
                            KEY_MEDIA_TYPES, String.valueOf(mediaType))
                            .build();
                }
                Path setPath = dm.findPathByUri(uri, null);
                MediaSet mediaSet = null;
                if (setPath != null) {
                    mediaSet = (MediaSet) dm.getMediaObject(setPath);
                }
                if (mediaSet != null) {
                    if (mediaSet.isLeafAlbum()) {
                        data.putString(AlbumPage.KEY_MEDIA_PATH, setPath.toString());
                        data.putString(AlbumPage.KEY_PARENT_MEDIA_PATH,
                                dm.getTopSetPath(DataManager.INCLUDE_ALL));
                        getStateManager().startState(AlbumPage.class, data);
                    } else {
                        data.putString(AlbumSetPage.KEY_MEDIA_PATH, setPath.toString());
                        getStateManager().startState(AlbumSetPage.class, data);
                    }
                } else {
                    startDefaultPage();
                }
            } else {
                /* SPRD: Drm feature start @{ */
                if(neededChangetoContent(uri)) {
                    uri = changeToContent(uri);
                }
                Path itemPath = dm.findPathByUri(uri, contentType);
                /**SPRD:473267 M porting add video entrance & related bug-fix
                Modify 20150106 of bug 390428,video miss after crop @{ */
                Path albumPath = dm.getDefaultSetOf(false,itemPath);
                /**@}*/
                /* SPRD: Drm feature end @} */
                data.putString(PhotoPage.KEY_MEDIA_ITEM_PATH, itemPath.toString());
                /* SPRD: Modify 562234 for support edit image when open from other applications @{
                 * data.putBoolean(PhotoPage.KEY_READONLY, true);
                 */
                data.putBoolean(PhotoPage.KEY_READONLY,
                        intent.getBooleanExtra(PhotoPage.KEY_READONLY, true));
                /* @} */
                // TODO: Make the parameter "SingleItemOnly" public so other
                //       activities can reference it.
                boolean singleItemOnly = (albumPath == null)
                        || intent.getBooleanExtra("SingleItemOnly", false);
                if (!singleItemOnly) {
                    data.putString(PhotoPage.KEY_MEDIA_SET_PATH, albumPath.toString());
                    // when FLAG_ACTIVITY_NEW_TASK is set, (e.g. when intent is fired
                    // from notification), back button should behave the same as up button
                    // rather than taking users back to the home screen
                    if (intent.getBooleanExtra(PhotoPage.KEY_TREAT_BACK_AS_UP, false)
                            || ((intent.getFlags() & Intent.FLAG_ACTIVITY_NEW_TASK) != 0)) {
                        data.putBoolean(PhotoPage.KEY_TREAT_BACK_AS_UP, true);
                    }
                }
                /* SPRD: fix bug 488355,crashed when we saw some specific picture's info rmation @{ */
                try {
                    getStateManager().startState(SinglePhotoPage.class, data);
                } catch (Exception e) {
                    Toast.makeText(this,
                            R.string.fail_to_load, Toast.LENGTH_LONG).show();
                    finish();
                    return;
                }
                /* @} */
            }
        }
    }

    @Override
    protected void onResume() {
        //Utils.assertTrue(getStateManager().getStateCount() > 0);
        super.onResume();
        /* SPRD:bug 543815 if no critical permissions,ActivityState count is 0,when fixed screen @{ */
        if (getStateManager().getStateCount() <= 0) {
            finish();
            return;
        }
        /* @} */
        /* SPRD: add check gallery permissions @{*/
        checkPermissions();
        if (!mHasCriticalPermissions) {
            Log.v(TAG, "onResume: Missing critical permissions.");
            finish();
            return;
        }
        /* @} */
        if (mVersionCheckDialog != null) {
            mVersionCheckDialog.show();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (GalleryUtils.isMonkey()) {
            sLastActivity = this;
        }
        if (mVersionCheckDialog != null) {
            mVersionCheckDialog.dismiss();
        }
    }

    @Override
    public void onCancel(DialogInterface dialog) {
        if (dialog == mVersionCheckDialog) {
            mVersionCheckDialog = null;
        }
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        final boolean isTouchPad = (event.getSource()
                & InputDevice.SOURCE_CLASS_POSITION) != 0;
        if (isTouchPad) {
            float maxX = event.getDevice().getMotionRange(MotionEvent.AXIS_X).getMax();
            float maxY = event.getDevice().getMotionRange(MotionEvent.AXIS_Y).getMax();
            View decor = getWindow().getDecorView();
            float scaleX = decor.getWidth() / maxX;
            float scaleY = decor.getHeight() / maxY;
            float x = event.getX() * scaleX;
            //x = decor.getWidth() - x; // invert x
            float y = event.getY() * scaleY;
            //y = decor.getHeight() - y; // invert y
            MotionEvent touchEvent = MotionEvent.obtain(event.getDownTime(),
                    event.getEventTime(), event.getAction(), x, y, event.getMetaState());
            return dispatchTouchEvent(touchEvent);
        }
        return super.onGenericMotionEvent(event);
    }

    /**
     * SPRD: check intent URI scheme is content or not. @{
     *
     * @param uri URI content or file
     * @return true if URI scheme is content, else false
     */
    private boolean neededChangetoContent(Uri uri) {
        return uri.getScheme().compareTo("content") != 0;
    }

    /**
     * SPRD: get content URI by file path @{
     *
     * @param uri
     * @return URI of content
     */
    private Uri changeToContent(Uri uri) {
        Uri baseUri = Images.Media.EXTERNAL_CONTENT_URI;
        Uri query = baseUri.buildUpon().appendQueryParameter("limit", "1").build();
        String[] projection = new String[] {
                ImageColumns._ID
        };
        String selection = ImageColumns.DATA + '=' + "\"" + uri.getPath() + "\"";

        Cursor cursor = null;
        long id = -1;
        try {
            cursor = getContentResolver().query(query, projection, selection, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                id = cursor.getLong(0);
            }
        } catch (Exception e) {
            Log.w(TAG, "ChangetoContent error:" + e.toString());
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        if (id != -1) {
            uri = Images.Media.EXTERNAL_CONTENT_URI.buildUpon().appendPath(String.valueOf(id))
                    .build();
        }
        return uri;
    }
    /** @} */

    /* SPRD: add check gallery permissions @{*/
    private void checkPermissions() {
        boolean isPickIntent = Intent.ACTION_GET_CONTENT.equalsIgnoreCase(getIntent().getAction())
                || Intent.ACTION_PICK.equalsIgnoreCase(getIntent().getAction());
        if (isPickIntent && GalleryUtils.checkStoragePermissions(this)) {
            mHasCriticalPermissions = true;
        } else if (GalleryUtils.checkStoragePermissions(this) && GalleryUtils.checkLocationPermissions(this) ) { // SPRD: Modify for bug576760, also check access location permission
            mHasCriticalPermissions = true;
        } else {
            mHasCriticalPermissions = false;
        }

        if (!mHasCriticalPermissions) {
            Intent intent = null;
            if (isPickIntent) {
                intent = new Intent(this, PickPhotosPermissionsActivity.class);
            } else {
                intent = new Intent(this, GalleryPermissionsActivity.class);
            }

            if (Intent.isAccessUriMode(getIntent().getFlags())) {
                intent.setFlags(getIntent().getFlags());
            }
            if(getIntent().getAction() != null){
                intent.setAction(getIntent().getAction());
            }
            if (getIntent().getType() != null) {
                intent.setType(getIntent().getType());
            }
            if (getIntent().getData() != null) {
                intent.setData(getIntent().getData());
            }
            if (getIntent().getExtras() != null) {
                intent.putExtras(getIntent().getExtras());
            }
            intent.putExtra(PermissionsActivity.UI_START_BY, PermissionsActivity.START_FROM_GALLERY);
            if (!(sLastActivity != null && sLastActivity.isFinishing()) || !isFinishing()) {
                startActivity(intent);
                finish();
            }
        }
    }
    /* @} */

    /* SPRD:bug575864,open Gallery check widget,need update or not @{ */
    private void checkUpdateWidget() {
        final AppWidgetManager appWidgetManager = AppWidgetManager
                .getInstance(getApplicationContext());
        int[] conversationWidgetIds = null;
        if (appWidgetManager != null) {
            conversationWidgetIds = appWidgetManager.getAppWidgetIds(new ComponentName(
                    getApplicationContext(), PhotoAppWidgetProvider.class));
        }
        if (conversationWidgetIds != null && conversationWidgetIds.length > 0) {
            Log.d(TAG, "has " + conversationWidgetIds.length + " gallery widget,update widget ");
            Intent updateWidget = new Intent();
            updateWidget.setAction(AppWidgetManager.ACTION_APPWIDGET_UPDATE);
            updateWidget.putExtra(AppWidgetManager.EXTRA_APPWIDGET_IDS, conversationWidgetIds);
            sendBroadcast(updateWidget);
        } else {
            Log.d(TAG, " no gallery widget in Launcher");
        }
    }
    /* @} */
}
