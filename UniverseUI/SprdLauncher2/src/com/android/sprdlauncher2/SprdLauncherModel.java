/**
 * added by Sprd; This Class is the extend of the google LauncherModel
 */
package com.android.sprdlauncher2;

import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.TreeMap;

import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.util.Log;

public class SprdLauncherModel extends LauncherModel {

    SprdLauncherModel(LauncherAppState app, IconCache iconCache, AppFilter appFilter) {
        super(app, iconCache, appFilter);
    }

    /* SPRD: bug309463 2014-05-08 reset wallpaper dimension if needed. @{ */
    public static final String RESET_WALLPAPER_DIM_ACTION = "com.android.launcher.resetwallpaperdimension";
    /* SPRD: bug309463 2014-05-08 reset wallpaper dimension if needed. @} */

    @Override
    public void onReceive(Context context, Intent intent) {

        final String action = intent.getAction();

        /* SPRD: bug309463 2014-05-08 reset wallpaper dimension if needed. @{ */
        if (Intent.ACTION_PACKAGE_CHANGED.equals(action)
                || Intent.ACTION_PACKAGE_REMOVED.equals(action)
                || Intent.ACTION_PACKAGE_ADDED.equals(action)) {
            final String packageName = intent.getData().getSchemeSpecificPart();

            if (packageName == null || packageName.length() == 0) {
                // they sent us a bad intent
                return;
            }

            /* SPRD: for STK bug275865 @{ */
            if (mIsLoaderTaskRunning) {
                if (packageName.equals(STK1_PACKAGE)) {
                    pendingAddStk(0);
                    return;
                }
                if (packageName.equals(STK2_PACKAGE)) {
                    pendingAddStk(1);
                    return;
                }
            }
        } else if (RESET_WALLPAPER_DIM_ACTION.equals(action)) {
            mHandler.post(new Runnable() {
                public void run() {
                    LauncherModel.Callbacks cb = mCallbacks != null ? mCallbacks.get() : null;
                    if (cb != null) {
                        cb.bindResetWallpaperDimensions();
                    }
                }
            });
        }
        /* @} */
        super.onReceive(context, intent);
        /* SPRD: bug309463 2014-05-08 reset wallpaper dimension if needed. @} */
    }

    /* SPRD: for STK bug275865 @{ */

    protected void pendingAddStk(int type) {
        if (type == 0) {
            pendingAddStk1 = true;
        }
        if (type == 1) {
            pendingAddStk2 = true;
        }
    }

    protected void addStk() {
        removeStk();
        if (pendingAddStk1) {
            addStkToWorkspace(0);
            pendingAddStk1 = false;
        }
        if (pendingAddStk2) {
            addStkToWorkspace(1);
            pendingAddStk2 = false;
        }
    }

    protected void addStkToWorkspace(int type){
        if(stkExist(type)){
            return;
        }
        String stkPackage = type == 0 ? STK1_PACKAGE : STK2_PACKAGE;
        String stkActivity = type == 0 ? STK1_ACTIVITY : STK2_ACTIVITY;

        List<ResolveInfo> matches = AllAppsList.findActivitiesForPackage(mApp.getContext(), stkPackage);
        if(matches.size() <= 0){
            Log.d(TAG, "stk not installed:" + stkPackage);
            return;
        }
        PackageManager mPackageManager = mApp.getContext().getPackageManager();
        ResolveInfo info = matches.get(0);
        String title = info.loadLabel(mPackageManager).toString();
        Resources resources;
        Drawable d;
        Bitmap icon;
        try {
            resources = mPackageManager.getResourcesForApplication(
                    info.activityInfo.applicationInfo);
        } catch (PackageManager.NameNotFoundException e) {
            resources = null;
        }
        /* SPRD: coverity 80288 @{*/
        Intent i = new Intent(InstallShortcutReceiver.ACTION_INSTALL_SHORTCUT);
        Intent shortcutIntent = new Intent();
        shortcutIntent.setComponent(new ComponentName(stkPackage,stkActivity));
        if(resources != null){
            icon = BitmapFactory.decodeResource(resources, info.getIconResource());
            i.putExtra(Intent.EXTRA_SHORTCUT_ICON, icon);
        }
        /* @} */
        i.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcutIntent);
        i.putExtra(Intent.EXTRA_SHORTCUT_NAME, title);

        // Do not allow duplicate items
        i.putExtra("duplicate", false);
        mApp.getContext().sendBroadcast(i);
    }

    protected boolean stkExist(int type){
        String stkName = type == 0 ? STK1_PACKAGE : STK2_PACKAGE;
        final ContentResolver contentResolver = mApp.getContext().getContentResolver();
        final Uri contentUri = LauncherSettings.Favorites.CONTENT_URI;
        /* SPRD: Fix bug 281450, @{ */
        Cursor c = contentResolver.query(contentUri, null, null, null, null);
        final int intentIndex = c.getColumnIndexOrThrow(LauncherSettings.Favorites.INTENT);
        final int itemTypeIndex = c.getColumnIndexOrThrow(LauncherSettings.Favorites.ITEM_TYPE);
        String intentDescription;
        Intent intent;
        int itemType = -1;
        boolean stkExist = false;
        while(c.moveToNext()){
            intentDescription = c.getString(intentIndex);
            itemType = c.getInt(itemTypeIndex);
            if(itemType == LauncherSettings.Favorites.ITEM_TYPE_APPLICATION || itemType == LauncherSettings.Favorites.ITEM_TYPE_SHORTCUT){
                try {
                    intent = Intent.parseUri(intentDescription, 0);
                    ComponentName cn = intent.getComponent();
                    // SPRD: fix bug 282633
                    if(cn != null && cn.getPackageName() != null && cn.getPackageName().equals(stkName)){
                        Log.d(TAG, "stk already exist:"+stkName);
                        stkExist = true;
                        return true;
                    }
                } catch (URISyntaxException e) {
                    e.printStackTrace();
                } finally {
                    if (stkExist && c != null) {
                        c.close();
                    }
                }
            }
        }
        if (c != null) {
            c.close();
        }
        /* @} */
        Log.d(TAG, "stk not exist:"+stkName);
        return false;
    }

    protected void filterStk(ArrayList<ItemInfo> added) {
        for (ItemInfo app : added) {
            if (app.getIntent().getComponent().getPackageName().equals(STK1_PACKAGE)
                    && stkExist(0)) {
                Log.d(TAG, "remove "+STK1_PACKAGE + " from added");
                added.remove(app);
            }
            if (app.getIntent().getComponent().getPackageName().equals(STK2_PACKAGE)
                    && stkExist(1)) {
                Log.d(TAG, "remove "+STK2_PACKAGE + " from added");
                added.remove(app);
            }
        }
    }

    private void removeStk(){
        if (DEBUG_LOADERS) Log.d(TAG, "start remove stk:");
        PackageManager mPackageManager = mApp.getContext().getPackageManager();
        if (!isValidStk(mPackageManager, new ComponentName(STK1_PACKAGE, STK1_ACTIVITY))) {
            ArrayList<ItemInfo> infos = getItemInfoForPackageName(STK1_PACKAGE);
            if (DEBUG_LOADERS) Log.d(TAG, "remove stk1 from db,infos1 count:"+infos.size());
            for (ItemInfo i : infos) {
                deleteItemFromDatabase(mApp.getContext(), i);
            }
            mHandler.post(new Runnable() {
                public void run() {
                    Callbacks cb = mCallbacks != null ? mCallbacks.get() : null;
                    if (cb != null) {
                        ArrayList<String> ss = new ArrayList<String>();
                        ss.add(STK1_PACKAGE);
                        /* SPRD: Bug 286066,The second parameter cannot be null, otherwise cause a null pointer exception. @{ */
                        cb.bindComponentsRemoved(ss, new ArrayList<AppInfo>(), true);
                        /* @} */
                    } else {
                        if (DEBUG_LOADERS) Log.d(TAG, "remove stk1 failed because callback is null");
                    }
                }
            });
        }
        if (!isValidStk(mPackageManager, new ComponentName(STK2_PACKAGE, STK2_ACTIVITY))) {
            ArrayList<ItemInfo> infos = getItemInfoForPackageName(STK2_PACKAGE);
            if (DEBUG_LOADERS) Log.d(TAG, "remove stk2 from db,infos2 count:"+infos.size());
            for (ItemInfo i : infos) {
                deleteItemFromDatabase(mApp.getContext(), i);
            }
            mHandler.post(new Runnable() {
                public void run() {
                    Callbacks cb = mCallbacks != null ? mCallbacks.get() : null;
                    if (cb != null) {
                        ArrayList<String> ss = new ArrayList<String>();
                        ss.add(STK2_PACKAGE);
                        /* SPRD: Bug 286066,The second parameter cannot be null, otherwise cause a null pointer exception. @{ */
                        cb.bindComponentsRemoved(ss, new ArrayList<AppInfo>(), true);
                        /* @} */
                    } else {
                        if (DEBUG_LOADERS) Log.d(TAG, "remove stk2 failed because callback is null");
                    }
                }
            });
        }
        if (DEBUG_LOADERS) Log.d(TAG, "end remove stk:");
    }

    private boolean isValidStk(PackageManager pm, ComponentName cn) {
        if (cn == null) {
            return false;
        }
        // Skip if the application is disabled
        if (DEBUG_LOADERS)
            Log.i(TAG, cn.getPackageName()
                            + " enabled:"
                            + !(pm.getComponentEnabledSetting(cn) == PackageManager.COMPONENT_ENABLED_STATE_DISABLED));
        if (pm.getComponentEnabledSetting(cn) == PackageManager.COMPONENT_ENABLED_STATE_DISABLED) {
            return false;
        }
        return true;
    }
    /* @} */

    /* SPRD: add for UUI previews @{*/
    protected void reduceItemsScreen(Context context, int screenNum) {
        final ContentResolver contentResolver = context.getContentResolver();
        final ContentValues values = new ContentValues();
        values.put(LauncherSettings.Favorites.SCREEN, screenNum - 1);
        /* Add 20130227 Spreadst of 130369 when delete workspace child start*/
        contentResolver.update(
                LauncherSettings.Favorites.CONTENT_URI_NO_NOTIFICATION, values,
                "screen=" + screenNum + " and container != "
                        + LauncherSettings.Favorites.CONTAINER_HOTSEAT, null);
        /* Add 20130227 Spreadst of 130369 when delete workspace child end*/
    }

    protected void sortPreviewItems(Context context, final ArrayList<Integer> newOrder) {
        final ContentResolver resolver = context.getContentResolver();
        Runnable sortPreviewRunnable = new Runnable() {
            @Override
            public void run() {
                // TODO Auto-generated method stub
                int newOrderSize = 0;
                if (newOrder != null) {
                    newOrderSize = newOrder.size();
                } else {
                    Log.w(TAG, "newOrder is NullPointerException");
                    return;
                }
                ArrayList<Integer> updateList = new ArrayList<Integer>();
                HashMap<Integer, ArrayList<ContentValues>> dataMap = new HashMap<Integer, ArrayList<ContentValues>>();
                for (int newIndex = 0; newIndex < newOrderSize; newIndex++) {
                    int oldIndex = newOrder.get(newIndex);
                    if (newIndex == oldIndex) {
                        continue;
                    }
                    updateList.add(newIndex);
                }
                final int _idColumnIndex = 0;
                final int titleColumnIndex = 1;
                final int intentColumnIndex = 2;
                final int containerColumnIndex = 3;
                final int screenColumnIndex = 4;
                final int cellXColumnIndex = 5;
                final int cellYColumnIndex = 6;
                final int spanXColumnIndex = 7;
                final int spanYColumnIndex = 8;
                final int itemTypeColumnIndex = 9;
                final int appWidgetIdColumnIndex = 10;
                final int isShortcutColumnIndex = 11;
                final int iconTypeColumnIndex = 12;
                final int iconPackageColumnIndex = 13;
                final int iconResourceColumnIndex = 14;
                final int iconColumnIndex = 15;
                final int uriColumnIndex = 16;
                final int displayModeColumnIndex = 17;

                int updateSize = updateList.size();
                for (int i = 0; i < updateSize; i++) {
                    /* SPRD: Fix bug 267031,do not use "screen" to rank CellLayout,just use "screenRank" in Android4.4 Launcher @{ */
                    Cursor cursor1 = resolver.query(
                            LauncherSettings.WorkspaceScreens.CONTENT_URI, null,
                            "screenRank=" + updateList.get(i).intValue(),
                            null, null);
                    if (cursor1 == null || !cursor1.moveToFirst()) {
                        if (cursor1 != null) {
                            cursor1.close();
                            cursor1 = null;
                        }
                        continue;
                    }
                    int screen = cursor1.getInt(0);
                    cursor1.close();
                    cursor1 = null;
                    Cursor cursor = resolver.query(
                            LauncherSettings.Favorites.CONTENT_URI, null,
                            "screen=" + screen,
                            null, null);
                    /* @} */
                    ArrayList<ContentValues> tempArrayList = new ArrayList<ContentValues>();
                    if (cursor == null || !cursor.moveToFirst()) {
                        if (cursor != null) {
                            cursor.close();
                            cursor = null;
                        }
                        dataMap.put(updateList.get(i).intValue(), tempArrayList);
                        continue;
                    }
                    do {
                        ContentValues tempValues = new ContentValues();
                        tempValues.put(LauncherSettings.Favorites._ID,
                                cursor.getInt(_idColumnIndex));
                        tempValues.put(LauncherSettings.Favorites.TITLE,
                                cursor.getString(titleColumnIndex));
                        tempValues.put(LauncherSettings.Favorites.INTENT,
                                cursor.getString(intentColumnIndex));
                        tempValues.put(LauncherSettings.Favorites.CONTAINER,
                                cursor.getInt(containerColumnIndex));
                        // tempValues.put(LauncherSettings.Favorites.SCREEN,
                        // cursor.getInt(screenColumnIndex));
                        tempValues.put(LauncherSettings.Favorites.CELLX,
                                cursor.getInt(cellXColumnIndex));
                        tempValues.put(LauncherSettings.Favorites.CELLY,
                                cursor.getInt(cellYColumnIndex));
                        tempValues.put(LauncherSettings.Favorites.SPANX,
                                cursor.getInt(spanXColumnIndex));
                        tempValues.put(LauncherSettings.Favorites.SPANY,
                                cursor.getInt(spanYColumnIndex));
                        tempValues.put(LauncherSettings.Favorites.ITEM_TYPE,
                                cursor.getInt(itemTypeColumnIndex));
                        tempValues.put(LauncherSettings.Favorites.APPWIDGET_ID,
                                cursor.getInt(appWidgetIdColumnIndex));
                        tempValues.put(LauncherSettings.Favorites.IS_SHORTCUT,
                                cursor.getInt(isShortcutColumnIndex));
                        tempValues.put(LauncherSettings.Favorites.ICON_TYPE,
                                cursor.getInt(iconTypeColumnIndex));
                        tempValues.put(LauncherSettings.Favorites.ICON_PACKAGE,
                                cursor.getString(iconPackageColumnIndex));
                        tempValues.put(
                                LauncherSettings.Favorites.ICON_RESOURCE,
                                cursor.getString(iconResourceColumnIndex));
                        tempValues.put(LauncherSettings.Favorites.ICON,
                                cursor.getBlob(iconColumnIndex));
                        tempValues.put(LauncherSettings.Favorites.URI,
                                cursor.getString(uriColumnIndex));
                        tempValues.put(LauncherSettings.Favorites.DISPLAY_MODE,
                                cursor.getInt(displayModeColumnIndex));
                        tempArrayList.add(tempValues);
                    } while (cursor.moveToNext());
                    if (cursor != null) {
                        cursor.close();
                        cursor = null;
                    }
                    dataMap.put(updateList.get(i), tempArrayList);
                    /* SPRD: Fix bug 267019,do not use "screen" to rank CellLayout,just use "screenRank" in Android4.4 Launcher @{ */
                    resolver.delete(
                            LauncherSettings.Favorites.CONTENT_URI_NO_NOTIFICATION,
                            "screen=" + screen
                                    + " and container != "
                                    + LauncherSettings.Favorites.CONTAINER_HOTSEAT,
                            null);
                    /* @} */
                }

                for (int newIndex = 0; newIndex < newOrderSize; newIndex++) {
                    int oldIndex = newOrder.get(newIndex);
                    if (newIndex == oldIndex) {
                        continue;
                    }
                    int listSize = dataMap.get(oldIndex).size();
                    for (int i = 0; i < listSize; i++) {
                        /* SPRD: Fix bug 267019,do not use "screen" to rank CellLayout,just use "screenRank" in Android4.4 Launcher @{ */
                        Cursor cursor1 = resolver.query(
                                LauncherSettings.WorkspaceScreens.CONTENT_URI, null,
                                "screenRank=" + newIndex,
                                null, null);
                        if (cursor1 == null || !cursor1.moveToFirst()) {
                            if (cursor1 != null) {
                                cursor1.close();
                                cursor1 = null;
                            }
                            continue;
                        }
                        int screen = cursor1.getInt(0);
                        cursor1.close();
                        cursor1 = null;
                        dataMap.get(oldIndex).get(i)
                                .put(LauncherSettings.Favorites.SCREEN, screen);
                        /* @} */
                        resolver.insert(
                                LauncherSettings.Favorites.CONTENT_URI_NO_NOTIFICATION,
                                dataMap.get(oldIndex).get(i));
                    }
                }
            }

        };
        new Thread(sortPreviewRunnable).start();

    }

    public void insertEmptyScreen(Context context, final Long screenId,
            final int screenRank) {
        final ContentResolver cr = context.getContentResolver();
        final Uri uri = LauncherSettings.WorkspaceScreens.CONTENT_URI;

        Runnable r = new Runnable() {
            @Override
            public void run() {
                ContentValues values = new ContentValues();
                values.put(LauncherSettings.WorkspaceScreens._ID, screenId);
                values.put(LauncherSettings.WorkspaceScreens.SCREEN_RANK,
                        screenRank);
                cr.insert(uri, values);

                synchronized (sBgLock) {
                    sBgWorkspaceScreens.add(screenId);
                }
            }
        };
        runOnWorkerThread(r);
    }

    public long removeEmptyScreen(Context context, final Long screenId) {
        final ContentResolver cr = context.getContentResolver();
        final Uri uri = LauncherSettings.WorkspaceScreens.CONTENT_URI;
        Runnable r = new Runnable() {
            @Override
            public void run() {
                cr.delete(uri, LauncherSettings.WorkspaceScreens._ID + "="
                        + screenId, null);
                synchronized (sBgLock) {
                    sBgWorkspaceScreens.remove(screenId);
                }
            }
        };
        runOnWorkerThread(r);
        return screenId;
    }
    /* @} */
    /* SPRD: Feature 255177,add new view at left side@{ */
    protected void plusItemsScreen(Context context, int screenNum) {
        final ContentResolver contentResolver = context.getContentResolver();
        final ContentValues values = new ContentValues();
        values.put(LauncherSettings.Favorites.SCREEN, screenNum + 1);
        contentResolver.update(
                LauncherSettings.Favorites.CONTENT_URI_NO_NOTIFICATION, values,
                "screen=" + screenNum + " and container != "
                        + LauncherSettings.Favorites.CONTAINER_HOTSEAT, null);
    }
    /* @} */

}
