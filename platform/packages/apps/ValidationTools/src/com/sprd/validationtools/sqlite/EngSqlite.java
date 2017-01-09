
package com.sprd.validationtools.sqlite;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;


import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteDatabase.CursorFactory;
import android.database.sqlite.SQLiteOpenHelper;
import android.provider.BaseColumns;
import android.util.Log;
import android.widget.Toast;
import android.os.Debug;

import com.sprd.validationtools.Const;
import com.sprd.validationtools.TestItem;

public class EngSqlite {
    private static final String TAG = "EngSqlite";
    private Context mContext;
    private SQLiteDatabase mSqLiteDatabase = null;

    private static EngSqlite mEngSqlite;

    public static synchronized EngSqlite getInstance(Context context) {
        if (mEngSqlite == null) {
            mEngSqlite = new EngSqlite(context);
        }
        return mEngSqlite;
    }

    private EngSqlite(Context context) {
        mContext = context;
        /*sprd: add for bug 411514 @{ */
        String filePath = new String(Const.ENG_ENGTEST_DB);
        File file = new File(filePath);
        /* @} */
        Process p = null;
        DataOutputStream os = null;
        try {
            p = Runtime.getRuntime().exec("chmod 777 productinfo");
            os = new DataOutputStream(p.getOutputStream());
            BufferedInputStream err = new BufferedInputStream(p.getErrorStream());
            BufferedReader br = new BufferedReader(new InputStreamReader(err));
            Log.v("Vtools", "os= " + br.readLine());
            Runtime.getRuntime().exec("chmod 777 " + file.getAbsolutePath());
            int status = p.waitFor();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            if (os != null) {
                try {
                    os.close();
                    p.destroy();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        ValidationToolsDatabaseHelper databaseHelper = new ValidationToolsDatabaseHelper(mContext);
        mSqLiteDatabase = databaseHelper.getWritableDatabase();
    }

    public ArrayList<TestItem> queryData(ArrayList<TestItem> queryListitem) {
        ArrayList<TestItem> resultListItem = queryListitem;
        for (int i = 0; i < resultListItem.size(); i++) {
            TestItem item = resultListItem.get(i);
            item.setResult(getTestListItemStatus(item.getTestname()));
        }
        return resultListItem;
    }

    public int getTestListItemStatus(String name) {
        Cursor cursor = mSqLiteDatabase.query(Const.ENG_STRING2INT_TABLE,
                new String[] {
                    "value"
                }, "name=" + "\'" + name + "\'", null, null,
                null, null);
        Log.d(TAG, "name" + name);
        Log.d(TAG, "cursor.count" + cursor.getCount());
        try {
            if (cursor.getCount() != 0) {
                cursor.moveToNext();
                if (cursor.getInt(0) == Const.FAIL) {
                    return Const.FAIL;
                } else if (cursor.getInt(0) == Const.SUCCESS) {
                    return Const.SUCCESS;
                } else {
                    return Const.DEFAULT;
                }
            } else {
                Log.d(TAG, "cursor.count");
                return Const.DEFAULT;
            }
        } catch (Exception ex) {
            Log.d(TAG, "exception");
            return Const.DEFAULT;
        } finally {
            Log.d(TAG, "fianlly");
            if (cursor != null) {
                cursor.close();
            }
        }
    }

    public void inSertData(String name, String value) {
        ContentValues cv = new ContentValues();
        cv.put(Const.ENG_STRING2INT_NAME, name);
        cv.put(Const.ENG_STRING2INT_VALUE, value);
        long returnValue = mSqLiteDatabase.insert(Const.ENG_STRING2INT_TABLE, null, cv);
        if (returnValue == -1) {
            Log.e(TAG, "insert DB error!");
        }
        mSqLiteDatabase.close();
    }

    public void updateData(String name, int value) {
        ContentValues cv = new ContentValues();
        cv.put(Const.ENG_STRING2INT_NAME, name);
        cv.put(Const.ENG_STRING2INT_VALUE, value);
        mSqLiteDatabase.update(Const.ENG_STRING2INT_TABLE, cv,
                Const.ENG_STRING2INT_NAME + "= \'" + name + "\'", null);
    }

    public void updateDB(String name, int value) {
        if (queryData(name)) {
            updateData(name, value);
        } else {
            Log.d(TAG, "Error:unqueryData");
        }
    }

    private void inSertData(String name, int value) {
        ContentValues cv = new ContentValues();
        cv.put(Const.ENG_STRING2INT_NAME, name);
        cv.put(Const.ENG_STRING2INT_VALUE, value);

        Log.d(TAG, "name" + name + "value:" + value);

        long returnValue = mSqLiteDatabase.insert(Const.ENG_STRING2INT_TABLE, null, cv);
        Log.e(TAG, "returnValue" + returnValue);
        if (returnValue == -1) {
            Log.e(TAG, "insert DB error!");
        }
    }

    public boolean queryData(String name) {
        try {
            Cursor c = mSqLiteDatabase.query(Const.ENG_STRING2INT_TABLE,
                    new String[] {
                            Const.ENG_STRING2INT_NAME, Const.ENG_STRING2INT_VALUE
                    },
                    Const.ENG_STRING2INT_NAME + "= \'" + name + "\'",
                    null, null, null, null);
            if (c != null) {
                if (c.getCount() > 0) {
                    c.close();
                    return true;
                }
                c.close();
            }
        } catch (Exception e) {
            return false;
        }
        return false;
    }

    public int queryNotTestCount() {
        int bln = 0;
        if (mSqLiteDatabase == null)
            return bln;
        Cursor cur = mSqLiteDatabase.query(Const.ENG_STRING2INT_TABLE, new String[] {
                "name", "value"
        },
                "value=?", new String[] {
                    "2"
                },
                null, null, null);
        if (cur != null && cur.getCount() >= 1) {
            bln = cur.getCount();
        }
        cur.close();
        return bln;
    }

    public int queryFailCount() {
        int bln = 0;
        if (mSqLiteDatabase == null)
            return bln;
        Cursor cur = mSqLiteDatabase.query(Const.ENG_STRING2INT_TABLE, new String[] {
                "name", "value"
        },
                "value!=?", new String[] {
                    "1"
                },
                null, null, null);
        if (cur != null && cur.getCount() >= 1) {
            bln = cur.getCount();
        }
        cur.close();
        return bln;
    }
    public int querySystemFailCount() {                           
        ArrayList<TestItem> supportList = Const.getSupportSystemTestList(mContext);
        int count = 0;
        for (int i = 0; i < supportList.size(); i++) {
            if (Const.SUCCESS != getTestListItemStatus(supportList.get(i).getTestname())) {
                count++;
            } 
        }
        return count;
    }

    private static class ValidationToolsDatabaseHelper extends SQLiteOpenHelper {

        Context mContext = null;
        public ValidationToolsDatabaseHelper(Context context) {
            super(context, Const.ENG_ENGTEST_DB, null, Const.ENG_ENGTEST_VERSION);
            mContext = context;
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL("DROP TABLE IF EXISTS " + Const.ENG_STRING2INT_TABLE + ";");
            db.execSQL("CREATE TABLE " + Const.ENG_STRING2INT_TABLE + " (" + BaseColumns._ID
                    + " INTEGER PRIMARY KEY AUTOINCREMENT," + Const.ENG_GROUPID_VALUE
                    + " INTEGER NOT NULL DEFAULT 0,"
                    + Const.ENG_STRING2INT_NAME + " TEXT," + Const.ENG_STRING2INT_VALUE
                    + " INTEGER NOT NULL DEFAULT 0" + ");");

            ArrayList<TestItem> supportArray = Const.getSupportList(false, mContext);

            for (int index = 0; index < supportArray.size(); index++) {
                ContentValues cv = new ContentValues();
                cv.put(Const.ENG_STRING2INT_NAME, supportArray.get(index).getTestname());
                cv.put(Const.ENG_STRING2INT_VALUE, String.valueOf(Const.DEFAULT));
                long returnValue = db.insert(Const.ENG_STRING2INT_TABLE, null, cv);
                if (returnValue == -1) {
                    Log.e(TAG, "insert DB error!");
                    break;
                }
            }
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            if (newVersion > oldVersion) {
                db.execSQL("DROP TABLE IF EXISTS " + Const.ENG_STRING2INT_TABLE + ";");
                onCreate(db);
            }
        }

    }
}
