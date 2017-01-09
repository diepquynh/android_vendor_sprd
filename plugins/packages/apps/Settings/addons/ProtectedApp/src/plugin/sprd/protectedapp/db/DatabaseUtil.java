package plugin.sprd.protectedapp.db;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

import plugin.sprd.protectedapp.model.AppInfoModel;
import plugin.sprd.protectedapp.util.ApplicationsState;

import android.content.ContentValues;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.util.Log;
import android.content.SharedPreferences;


public class DatabaseUtil extends DatabaseHelper {
    private static final String TAG = DatabaseUtil.class.getSimpleName();
    private static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);
    private Context mContext;
    private static final String ENABLE = "1";
    private static final String DISABLE = "0";

    public DatabaseUtil(Context context) {
        super(context);
        if (DEBUG) Log.d(TAG, "DatabaseUtil constructor ");
        this.mContext = context;
    }

    /**
     * instance for sqlite database operation
     * @param context
     * @return
     */
    private SQLiteDatabase getDatabase() {
        return DatabaseUtil.getInstance(mContext).getWritableDatabase();
    }

    public synchronized void update(String pkgName, Boolean state) {
        if (DEBUG) Log.d(TAG, "update pkgName: " + pkgName + " state: " + state);
        SQLiteDatabase db = getDatabase();
        try {
            ContentValues contentValues = new ContentValues();
            contentValues.put(IndexColumns.ENABLED, state == true ? 1 : 0);
            String[] args = { pkgName };
            db.update(Tables.TABLE_SECURITY_INDEX, contentValues, IndexColumns.PKGNAME + "=?", args);
        } catch (Exception e) {
            Log.e(TAG, "update error: ", e);
        } finally {
            try {
                db.close();
            } catch (Exception ex) {
                Log.e(TAG, "update db close error: ", ex);
            }
        }
    }

    public synchronized void updateAll(Boolean state) {
        if (DEBUG) Log.d(TAG, "updateAll: " + " state: " + state);
        SQLiteDatabase db = getDatabase();
        try {
            ContentValues contentValues = new ContentValues();
            contentValues.put(IndexColumns.ENABLED, state ? 1 : 0);
            db.update(Tables.TABLE_SECURITY_INDEX, contentValues, null, null);
        } catch (Exception e) {
            Log.e(TAG, "updateAll error: ", e);
        } finally {
            try {
                db.close();
            } catch (Exception ex) {
                Log.e(TAG, "updateAll db close error: ", ex);
            }
        }
    }

    public synchronized void insert(String pkgName) {
        if (DEBUG) Log.d(TAG, "insert pkgName: " + pkgName);
        if(pkgName == null || "".equals(pkgName)){
            if (DEBUG) Log.d(TAG, "Insert error: package name is null");
            return;
        }
        if (mContext.getPackageName().equalsIgnoreCase(pkgName)) {
            if (DEBUG) Log.d(TAG, "Insert error: package equals Ignore Case");
            return;
        }
        SQLiteDatabase db = getDatabase();
        try {
            ContentValues contentValues = new ContentValues();
            contentValues.put(IndexColumns.PKGNAME, pkgName);
            contentValues.put(IndexColumns.ENABLED, String.valueOf(0));
            db.insert(Tables.TABLE_SECURITY_INDEX, "", contentValues);
        } catch (Exception e) {
            Log.e(TAG, "Insert error: ", e);
        } finally {
            try {
                 db.close();
            } catch (Exception ex) {
            }
        }
    }

    public synchronized void delete(String pkgName) {
        if (DEBUG) Log.d(TAG, "delete pkgName: " + pkgName);
        SQLiteDatabase db = getDatabase();
        try {
            String[] args = { pkgName };
            db.delete(Tables.TABLE_SECURITY_INDEX, IndexColumns.PKGNAME + "=?", args);
        } catch (Exception e) {
            Log.e(TAG, "Delete error: ", e);
        } finally {
            try {
                db.close();
            } catch (Exception ex) {
                Log.e(TAG, "Delete db close error: ", ex);
            }
        }
    }

    public synchronized boolean hasNoDirectValue(int states) {
        if (DEBUG)
            Log.d(TAG, "hasNoDirectValue : " + states);
        SQLiteDatabase db = getDatabase();
        Cursor cursor = null;
        try {
            cursor = db.query(Tables.TABLE_SECURITY_INDEX,
                    new String[]{IndexColumns.ENABLED},
                    IndexColumns.ENABLED + " = ? ",
                    new String[] { String.valueOf(states) },
                    null,
                    null,
                    null);
            return !cursor.moveToNext();
        } catch (Exception e) {
            Log.e(TAG, "Delete error: ", e);
        } finally {
            try {
                if (db != null) {
                    db.close();
                }
                if (cursor != null) {
                    cursor.close();
                }
            } catch (Exception ex) {
                Log.e(TAG, "Delete db close error: ", ex);
            }
        }
        return false;
    }

    public synchronized ArrayList<AppInfoModel> initialize(ArrayList<ApplicationsState.AppEntry> list) {
        if (DEBUG) Log.d(TAG, "initialize");
        if (list == null) {
            if (DEBUG) Log.d(TAG, "initialize. list is null ");
            return null;
        }
        Cursor cursor = null;
        SQLiteDatabase db = getDatabase();
        try {
            cursor = db.query(Tables.TABLE_SECURITY_INDEX, null, null, null, null, null, null);

            // We need initialize database that be accessed first
            if (cursor.getCount() <= 0) {
                ArrayList<AppInfoModel> models = getInitializeModelList(list);
                for (AppInfoModel clearModel : models) {
                    insert(clearModel.getPackageName());
                }
                return models;
            }

            ArrayList<AppInfoModel> cursorModels = new ArrayList<AppInfoModel>();

            // If has installed new applications , Should added to database.
            for (ApplicationsState.AppEntry appEntry : list) {
                boolean isInserted = true;
                cursor.moveToFirst();
                while (cursor.moveToNext()) {
                    if (appEntry.info.packageName != null
                            && appEntry.info.packageName.equalsIgnoreCase(cursor.getString(cursor
                                    .getColumnIndex(IndexColumns.PKGNAME)))) {
                        isInserted = false;
                    }

                }
                if (isInserted) {
                    insert(appEntry.info.packageName);
                }
            }

            /* modify for bug 615790 */
            SharedPreferences preferences = mContext.getSharedPreferences("ProtectedApp", mContext.MODE_PRIVATE);
            String load_entries_complete = preferences.getString("load_entries_complete", "false");

            // remove record while application has been uninstalled.
           if (load_entries_complete.equals("true")) {
                cursor.moveToFirst();
                while (cursor.moveToNext()) {
                    String pkgName = cursor.getString(cursor.getColumnIndex(IndexColumns.PKGNAME));
                    boolean isUninstalled = true;
                    for (ApplicationsState.AppEntry appEntry : list) {
                        if (pkgName != null && pkgName.equalsIgnoreCase(appEntry.info.packageName)) {
                            isUninstalled = false;
                        }
                    }
                    if (isUninstalled) {
                        delete(pkgName);
                    }
                }
            }

            //Normal times. should rebuild list mode for adapter of fragment
            cursor.moveToFirst();
            if (cursor.getCount() > 0) {
                while (cursor.moveToNext()) {
                    AppInfoModel securityClearModel = new AppInfoModel();
                    String enable = cursor.getString(cursor.getColumnIndex(IndexColumns.ENABLED));
                    String pkgName = cursor.getString(cursor.getColumnIndex(IndexColumns.PKGNAME));
                    securityClearModel.setEnable(enable);
                    securityClearModel.setPackageName(pkgName);
                    cursorModels.add(securityClearModel);
                }
                return cursorModels;
            }
        } catch (Exception e) {
            Log.e(TAG, " initialize. query Count error: ", e);
        } finally {
            if (cursor != null) {
                try {
                    cursor.close();
                    db.close();
                } catch (Exception ex) {
                    Log.e(TAG, " initialize. close error: ", ex);
                }
            }
        }
        return null;
    }

    /**
     * when receiver the PRESENT action. Prepare appinfo list for clear recent task.
     */
    public synchronized ArrayList<AppInfoModel> queryEnableList() {
        Cursor cursor = null;
        SQLiteDatabase db = getDatabase();
        try {
            cursor = db.query(Tables.TABLE_SECURITY_INDEX, null, null, null, null, null, null);
            ArrayList<AppInfoModel> cursorModels = new ArrayList<AppInfoModel>();
            if (cursor.getCount() > 0) {
                while (cursor.moveToNext()) {
                    AppInfoModel securityClearModel = new AppInfoModel();
                    String isEnable = cursor.getString(cursor.getColumnIndex(IndexColumns.ENABLED));
                    String pkgName = cursor.getString(cursor.getColumnIndex(IndexColumns.PKGNAME));
                    securityClearModel.setEnable(isEnable);
                    securityClearModel.setPackageName(pkgName);
                    if(ENABLE.equals(isEnable)){
                        cursorModels.add(securityClearModel);
                    }
                }
                return cursorModels;
            }
        } catch (Exception e) {
            Log.e(TAG, " queryEnableList. Count error: ", e);
        } finally {
            if (cursor != null) {
                try {
                    cursor.close();
                    db.close();
                } catch (Exception ex) {
                    Log.e(TAG, " queryEnableList. close error: ", ex);
                }
            }
        }
        return null;
    }

    /**
     * Constructor initialize record for this feature be used first.
     * @param list
     */
    private ArrayList<AppInfoModel> getInitializeModelList(ArrayList<ApplicationsState.AppEntry> list) {
        ArrayList<AppInfoModel> modelList = new ArrayList<AppInfoModel>();
        for (ApplicationsState.AppEntry appEntry : list) {
            AppInfoModel securityClearModel = new AppInfoModel();
            ApplicationInfo appInfo = appEntry.info;
            securityClearModel.setPackageName(appInfo.packageName);
            securityClearModel.setUserID(String.valueOf(appInfo.uid));
            securityClearModel.setEnable(DISABLE);
            modelList.add(securityClearModel);
        }
        return modelList;
    }
}
