/*
 * Copyright (C) 2008 The Android Open Source Project
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

package plugins.sprd.Folder;


import android.app.AddonManager;
import android.content.Context;


import android.app.SearchManager;
import android.appwidget.AppWidgetProviderInfo;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.Intent.ShortcutIconResource;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.pm.ProviderInfo;
import android.content.pm.ResolveInfo;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Parcelable;
import android.os.Process;
import android.os.SystemClock;
import android.os.TransactionTooLargeException;
import android.provider.BaseColumns;
import android.text.TextUtils;
import android.util.Log;
import android.util.LongSparseArray;
import android.util.Pair;

import com.android.launcher3.AppInfo;
import com.android.launcher3.FolderInfo;
import com.android.launcher3.ItemInfo;
import com.android.launcher3.Launcher;
import com.android.launcher3.LauncherAppState;
import com.android.launcher3.LauncherModel;
import com.android.launcher3.LauncherSettings;
import com.android.launcher3.ShortcutInfo;
import com.android.launcher3.VolteAppsProvider;
import com.android.launcher3.compat.AppWidgetManagerCompat;
import com.android.launcher3.compat.LauncherActivityInfoCompat;
import com.android.launcher3.compat.LauncherAppsCompat;
import com.android.launcher3.compat.PackageInstallerCompat;
import com.android.launcher3.compat.PackageInstallerCompat.PackageInstallInfo;
import com.android.launcher3.compat.UserHandleCompat;
import com.android.launcher3.compat.UserManagerCompat;
import com.android.launcher3.Folderplugins.*;
import com.android.launcher3.*;

import java.lang.Override;
import java.util.ArrayList;
import java.util.TreeMap;


/**
 * Maintains in-memory state of the Launcher. It is expected that there should be only one
 * LauncherModel object held in a static. Also provide APIs for updating the database state
 * for the Launcher.
 */
public class LauncherModelUtilsAddon extends LauncherModelUtils implements AddonManager.InitialCallback {

    static final String TAG = "AddonLauncherModelPlugin";


    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        android.util.Log.d(TAG,"onCreateAddon");

        return clazz;
    }

    public void loadAppFolderDbInfo(Context context, AppFolderDbInfo folderDbInfo) {

        android.util.Log.d(TAG,"loadAppFolderDbInfo");
        if (context == null || folderDbInfo == null) {
            Log.e(TAG, "context or folderDbInfo is null.");
            return;
        }

        Log.d(TAG, "loadAppFolderDbInfo called, folder: " + folderDbInfo.folderInfo.title);
        final ContentResolver contentResolver = context.getContentResolver();
        final Uri appsUri = VolteAppsProvider.Apps.CONTENT_URI;
        final long container = folderDbInfo.folderInfo.id;
        final Cursor ac = contentResolver.query(appsUri, null,
                VolteAppsProvider.Apps.CONTAINER + "=?", new String[]{String.valueOf(container)}, null);
        try {
            final int idIndex = ac.getColumnIndexOrThrow(VolteAppsProvider.Apps._ID);
            final int titleIndex = ac.getColumnIndexOrThrow(VolteAppsProvider.Apps.TITLE);
            final int pkgIndex = ac.getColumnIndexOrThrow(VolteAppsProvider.Apps.COMPONENT_PKG);
            final int clsIndex = ac.getColumnIndexOrThrow(VolteAppsProvider.Apps.COMPONENT_CLS);
            final int cellXIndex = ac.getColumnIndexOrThrow(VolteAppsProvider.Apps.CELLX);
            final int cellYIndex = ac.getColumnIndexOrThrow(VolteAppsProvider.Apps.CELLY);
            final int itemTypeIndex = ac.getColumnIndexOrThrow(VolteAppsProvider.Apps.ITEM_TYPE);
            while (ac.moveToNext()) {
                try {
                    long id = ac.getLong(idIndex);
                    String title = ac.getString(titleIndex);
                    String pkg = ac.getString(pkgIndex);
                    String cls = ac.getString(clsIndex);
                    int cellX = ac.getInt(cellXIndex);
                    int cellY = ac.getInt(cellYIndex);
                    int type = ac.getInt(itemTypeIndex);
                    switch (type) {
                        case VolteAppsProvider.Apps.ITEM_TYPE_APPLICATION:
                            AppDbInfo dbItem = new AppDbInfo();
                            dbItem.id = id;
                            dbItem.title = title;
                            dbItem.pkgName = pkg;
                            dbItem.clsName = cls;
                            dbItem.screenId = folderDbInfo.folderInfo.screenId;
                            dbItem.cellX = cellX;
                            dbItem.cellY = cellY;
                            dbItem.container = container;
                            dbItem.itemType = VolteAppsProvider.Apps.ITEM_TYPE_APPLICATION;
                            dbItem.fromAppList = true;
                            // TODO: generate AppInfo and add to folderInfo
                            folderDbInfo.addChild(dbItem);
                            break;
                        case VolteAppsProvider.Apps.ITEM_TYPE_FOLDER:
                            Log.w(TAG, "A folder can not contains folder, folder title: " + title);
                            break;
                        default:
                            Log.e(TAG, "Unknown type in app list database, type: " + type);
                            break;
                    }
                } catch (Exception e) {
                    Launcher.addDumpLog(TAG, "App List items loading interrupted - invalid screens: " + e, true);
                }
            }
        } finally {
            ac.close();
        }
        if (folderDbInfo.children.size() == 0) {
            // TODO: delete the folder record
            deleteItemFromAppsDatabase(context, folderDbInfo);
        }

        Log.d(TAG, "loadAppFolderDbInfo finished, child count: " + folderDbInfo.children.size());
    }

    public  void deleteItemFromAppsDatabase(Context context, final ItemInfo item) {
        android.util.Log.d("AddonLauncherModelPlugin","deleteItemFromAppsDatabase");
        ArrayList<ItemInfo> items = new ArrayList<ItemInfo>();
        items.add(item);
        deleteItemsFromAppsDatabase(context, items);
    }

    public  void deleteItemsFromAppsDatabase(Context context, final ArrayList<ItemInfo> items) {
        android.util.Log.d("AddonLauncherModelPlugin","deleteItemsFromAppsDatabase");
        final ContentResolver cr = context.getContentResolver();

        Runnable r = new Runnable() {
            public void run() {
                for (ItemInfo item : items) {
                    Log.d(TAG, String.format("deleteItemsFromAppsDatabase _id[%d]", item.id));
                    final Uri uri = VolteAppsProvider.Apps.getContentUri(item.id);
                    cr.delete(uri, null, null);
                }
            }
        };
        LauncherModel.runOnWorkerThread(r);
    }

    public void addOrMoveItemInAppsDatabase(Context context, ItemInfo item, long container,long screenId, int cellX, int cellY){
        android.util.Log.d("AddonLauncherModelPlugin","addOrMoveItemInAppsDatabase");
        if (item.container == ItemInfo.NO_ID) {
            addItemToAppsDatabase(context, item, container, screenId, cellX, cellY);
        } else {
            moveItemInAppsDatabase(context, item, container, screenId, cellX, cellY);
        }
    }
    public void addItemToAppsDatabase(Context context, final ItemInfo item, final long container,
                                      final long screenId, final int cellX, final int cellY) {
        android.util.Log.d("AddonLauncherModelPlugin","addItemToAppsDatabase");
        item.container = container;
        item.cellX = cellX;
        item.cellY = cellY;
        item.screenId = screenId;

        final ContentValues values = new ContentValues();
        final ContentResolver cr = context.getContentResolver();

        item.id = LauncherAppState.getLauncherAppsProvider().generateNewItemId();
        values.put(VolteAppsProvider.Apps._ID, item.id);

        Log.d(TAG, "addItemInAppsDatabase called, item: " + item);
        final StackTraceElement[] stackTrace = new Throwable().getStackTrace();
        Runnable r = new Runnable() {
            public void run() {
                cr.insert(VolteAppsProvider.Apps.CONTENT_URI, values);
            }
        };
        LauncherModel.runOnWorkerThread(r);
    }

    public void moveItemInAppsDatabase(Context context, final ItemInfo item, final long container,
                                       final long screenId, final int cellX, final int cellY) {
        android.util.Log.d("AddonLauncherModelPlugin","moveItemInAppsDatabase");
        item.container = container;
        item.cellX = cellX;
        item.cellY = cellY;
        item.screenId = screenId;

        Log.d(TAG, "moveItemInAppsDatabase called, item: " + item);
        final ContentValues values = new ContentValues();
        values.put(VolteAppsProvider.Apps.CONTAINER, item.container);
        values.put(VolteAppsProvider.Apps.CELLX, item.cellX);
        values.put(VolteAppsProvider.Apps.CELLY, item.cellY);
        values.put(VolteAppsProvider.Apps.SCREEN, item.screenId);

        updateItemInAppsDatabaseHelper(context, values, item, "moveItemInAppsDatabase");
    }
    public void updateItemInAppsDatabaseHelper(Context context, final ContentValues values,
                                               final ItemInfo item, final String callingFunction) {
        android.util.Log.d("AddonLauncherModelPlugin","updateItemInAppsDatabaseHelper");
        Log.d(TAG, "updateItemInAppsDatabaseHelper");
        final long itemId = item.id;
        final Uri uri = VolteAppsProvider.Apps.getContentUri(itemId);
        final ContentResolver cr = context.getContentResolver();
        final StackTraceElement[] stackTrace = new Throwable().getStackTrace();
        Runnable r = new Runnable() {

            @Override
            public void run() {
                cr.update(uri, values, null, null);
            }
        };
        LauncherModel.runOnWorkerThread(r);
    }

    public void updateItemsInAppsDatabaseHelper(Context context, final ArrayList<ContentValues> valuesList,
                                                final ArrayList<ItemInfo> items, final String callingFunction) {
        android.util.Log.d("AddonLauncherModelPlugin","updateItemsInAppsDatabaseHelper");
        final ContentResolver cr = context.getContentResolver();

        final StackTraceElement[] stackTrace = new Throwable().getStackTrace();
        Runnable r = new Runnable() {
            public void run() {
                ArrayList<ContentProviderOperation> ops =
                        new ArrayList<ContentProviderOperation>();
                int count = items.size();
                for (int i = 0; i < count; i++) {
                    ItemInfo item = items.get(i);
                    final long itemId = item.id;
                    final Uri uri = VolteAppsProvider.Apps.getContentUri(itemId);
                    ContentValues values = valuesList.get(i);
                    ops.add(ContentProviderOperation.newUpdate(uri).withValues(values).build());
                }
                try {
                    cr.applyBatch(VolteAppsProvider.AUTHORITY, ops);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        };
        LauncherModel.runOnWorkerThread(r);
    }

    public void moveItemsInAppsDatabase(Context context, final ArrayList<ItemInfo> items,
                                        final long container, final int screen) {
        android.util.Log.d("AddonLauncherModelPlugin","moveItemsInAppsDatabase");

        ArrayList<ContentValues> contentValues = new ArrayList<ContentValues>();
        int count = items.size();

        for (int i = 0; i < count; i++) {
            ItemInfo item = items.get(i);
            item.container = container;
            item.screenId = screen;

            final ContentValues values = new ContentValues();
            values.put(VolteAppsProvider.Apps.CONTAINER, item.container);
            values.put(VolteAppsProvider.Apps.CELLX, item.cellX);
            values.put(VolteAppsProvider.Apps.CELLY, item.cellY);
            values.put(VolteAppsProvider.Apps.SCREEN, item.screenId);

            contentValues.add(values);
        }
        updateItemsInAppsDatabaseHelper(context, contentValues, items, "moveItemsInAppsDatabase");
    }
    public void modifyItemInAppsDatabase(Context context, final ItemInfo item, final long container,
                                         final long screenId, final int cellX, final int cellY, final int spanX, final int spanY) {
        android.util.Log.d("AddonLauncherModelPlugin","modifyItemInAppsDatabase");
        item.container = container;
        item.cellX = cellX;
        item.cellY = cellY;
        item.spanX = spanX;
        item.spanY = spanY;
        item.screenId = screenId;

        final ContentValues values = new ContentValues();
        values.put(VolteAppsProvider.Apps.CONTAINER, item.container);
        values.put(VolteAppsProvider.Apps.CELLX, item.cellX);
        values.put(VolteAppsProvider.Apps.CELLY, item.cellY);
        values.put(VolteAppsProvider.Apps.SCREEN, item.screenId);

        updateItemInAppsDatabaseHelper(context, values, item, "modifyItemInAppsDatabase");
    }
    public void updateItemInAppsDatabase(Context context, final ItemInfo item) {
        android.util.Log.d("AddonLauncherModelPlugin","updateItemInAppsDatabase");
        final ContentValues values = new ContentValues();
        updateItemInAppsDatabaseHelper(context, values, item, "updateItemInAppsDatabase");
    }

    public AppInfo removeInInstalledApps(String pkg, String cls, ArrayList<AppInfo> installedApps) {
        android.util.Log.d("AddonLauncherModelPlugin","removeInInstalledApps");
        if (pkg == null) {
            return null;
        }
        int index = -1;
        if (cls != null) {
            // find exact
            for (int i=0; i<installedApps.size(); i++) {
                AppInfo info = installedApps.get(i);
                if (pkg.equals(info.componentName.getPackageName())
                        && cls.equals(info.componentName.getClassName())) {
                    index = i;
                    break;
                }
            }
        } else {
            // find by package name
            for (int i = 0; i < installedApps.size(); i++) {
                AppInfo info = installedApps.get(i);
                if (pkg.equals(info.componentName.getPackageName())) {
                    index = i;
                    break;
                }
            }
        }
        if (index != -1) {
            AppInfo info = installedApps.remove(index);
            if (info != null) {
                Log.d(TAG, "Found app info in installed list: " + info);
            }
            return info;
        }
        Log.d(TAG, String.format("Search not found with pkg=%s, cls=%s", pkg, cls));
        return null;
    }

    public ArrayList<ItemInfo> loadAppsByScreenId(Context context, long screen) {
        final ContentResolver contentResolver = context.getContentResolver();
        final Uri appsUri = VolteAppsProvider.Apps.CONTENT_URI;
        final Cursor ac = contentResolver.query(appsUri, null,
                "screen=?", new String[] { String.valueOf(screen) }, null);
        ArrayList<ItemInfo> items = new ArrayList<ItemInfo>();
        try {
            final int idIndex = ac.getColumnIndexOrThrow( VolteAppsProvider.Apps._ID);
            final int titleIndex = ac.getColumnIndexOrThrow( VolteAppsProvider.Apps.TITLE);
            final int pkgIndex = ac.getColumnIndexOrThrow( VolteAppsProvider.Apps.COMPONENT_PKG);
            final int clsIndex = ac.getColumnIndexOrThrow( VolteAppsProvider.Apps.COMPONENT_CLS);
            final int containerIndex = ac.getColumnIndexOrThrow( VolteAppsProvider.Apps.CONTAINER);
            final int cellXIndex = ac.getColumnIndexOrThrow( VolteAppsProvider.Apps.CELLX);
            final int cellYIndex = ac.getColumnIndexOrThrow( VolteAppsProvider.Apps.CELLY);
            final int itemTypeIndex = ac.getColumnIndexOrThrow( VolteAppsProvider.Apps.ITEM_TYPE);
            while (ac.moveToNext()) {
                ItemInfo item = null;
                try {
                    long id = ac.getLong(idIndex);
                    String title = ac.getString(titleIndex);
                    String pkg = ac.getString(pkgIndex);
                    String cls = ac.getString(clsIndex);
                    int cellX = ac.getInt(cellXIndex);
                    int cellY = ac.getInt(cellYIndex);
                    int container = ac.getInt(containerIndex);
                    int type = ac.getInt(itemTypeIndex);
                    switch (type) {
                        case VolteAppsProvider.Apps.ITEM_TYPE_APPLICATION:
                            AppDbInfo dbItem = new AppDbInfo();
                            dbItem.id = id;
                            dbItem.title = title;
                            dbItem.pkgName = pkg;
                            dbItem.clsName = cls;
                            dbItem.screenId = screen;
                            dbItem.cellX = cellX;
                            dbItem.cellY = cellY;
                            dbItem.container = container;
                            dbItem.itemType = VolteAppsProvider.Apps.ITEM_TYPE_APPLICATION;
                            dbItem.fromAppList = true;
                            // TODO: generate AppInfo
                            item = dbItem;
                            break;
                        case VolteAppsProvider.Apps.ITEM_TYPE_FOLDER:
                            FolderInfo folderItem = new FolderInfo();
                            folderItem.id = id;
                            folderItem.title = title;
                            folderItem.screenId = screen;
                            folderItem.cellX = cellX;
                            folderItem.cellY = cellY;
                            folderItem.container = container;
                            folderItem.itemType = VolteAppsProvider.Apps.ITEM_TYPE_FOLDER;
                            folderItem.fromAppList = true;

                            AppFolderDbInfo folderDbInfo = AppFolderDbInfo.getInstance();
                            folderDbInfo.id = id;
                            folderDbInfo.screenId = screen;
                            folderDbInfo.cellX = cellX;
                            folderDbInfo.cellY = cellY;
                            folderDbInfo.container = container;
                            folderDbInfo.itemType = VolteAppsProvider.Apps.ITEM_TYPE_FOLDER;
                            folderDbInfo.folderInfo = folderItem;
                            folderDbInfo.fromAppList = true;
                            loadAppFolderDbInfo(context, folderDbInfo);
                            item = folderDbInfo;
                            break;
                        default:
                            Log.e(TAG, "Unkown type in app list database, type: " + type);
                            break;
                    }
                } catch (Exception e) {
                    Launcher.addDumpLog(TAG, "App List items loading interrupted - invalid screens: " + e, true);
                    e.printStackTrace();
                }
                if (item != null) {
                    items.add(item);
                }
            }
        } finally {
            ac.close();
        }

        return items;
    }

    public boolean isAddItemToDatabase(ItemInfo item) {
        return (item.container == ItemInfo.NO_ID || item.container == LauncherSettings.Favorites.CONTAINER_APPLIST || item.fromAppList);
    }

    public ComponentName setComponentName(ItemInfo s) {
        ComponentName cn = null;
        if(s instanceof ShortcutInfo){
            cn = ((ShortcutInfo) s).getTargetComponent();
        } else if (s instanceof  AppInfo) {
            cn = ((AppInfo) s).componentName;
        }
        return cn;
    }
}
