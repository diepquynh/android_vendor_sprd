
package com.sprd.note.data;

import java.util.List;

import com.sprd.note.utils.LogUtils;
import com.sprd.note.R;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.database.Cursor;
import android.net.Uri;
import android.support.v4.app.NotificationCompat;
import android.util.Log;
import android.widget.Toast;
import android.os.Debug;


public class NoteDataManagerImpl implements NoteDataManager {

    private static final String TAG = "NoteDataManagerImpl";
    private NoteList mNoteList = new NoteList();// List<NoteItem>

    private static NoteDataManager mDataManager = null;
    private DBOpenHelper mhelper = null;
    private Context mContext;
    private Cursor mCursor;

    private volatile boolean mNeedUpdate = false;
    private static final Uri NOTE_URI = NoteProvider.NOTE_CONTENT_URI;
    private static final boolean DEBUG = LogUtils.DEBUG;
    //SPRD : bug 520197 sqlitefull
    private static final String KEY_SQLITE_TAG = "SQLiteFull";
    private NoteDataManagerImpl(Context context) {
        mhelper = DBOpenHelper.getInstance(context);
        mContext = context;
    }

    public static NoteDataManager getNoteDataManger(Context context) {
        if (mDataManager == null) {
            synchronized (NoteDataManager.class) {
                if (mDataManager == null) {
                    mDataManager = new NoteDataManagerImpl(context);
                }
            }
        }
        return mDataManager;
    }

    @Override
    public void initData(Context context) {
        mNoteList.clear();
        if (mCursor == null) {
            Log.d(TAG, "updateFromDB---cursor == null");
            return;
        }
        if (!mCursor.moveToFirst()) {
            Log.d(TAG, "updateFromDB---cursor.close()");
            mCursor.close();
            return;
        }
        try {
            do {
                NoteItem noteItem = buildNoteItem(mCursor, context);
                mNoteList.add(noteItem);
            } while (mCursor.moveToNext());

        } finally {
            if (mCursor != null) {
                mCursor.close();
                mCursor = null;
            }
        }
        mNeedUpdate = false;
    }

    NoteItem buildNoteItem(Cursor cursor, Context mContext) {
        NoteItem noteItem = new NoteItem();
        noteItem._id = cursor.getInt(DBOpenHelper.ColumnsIndex.ID);
        noteItem.content = cursor.getString(DBOpenHelper.ColumnsIndex.NOTE_CONTENT);
        noteItem.title = cursor.getString(DBOpenHelper.ColumnsIndex.NOTE_TITLE);
        int isFileFolder = cursor.getInt(DBOpenHelper.ColumnsIndex.NOTE_IS_FOLDER);
        if (isFileFolder == 1) {
            noteItem.isFileFolder = true;
        } else if (isFileFolder == 0) {
            noteItem.isFileFolder = false;
        }
        noteItem.parentFolderId = cursor.getInt(DBOpenHelper.ColumnsIndex.NOTE_PARENT_FOLDER);
        noteItem.date = cursor.getLong(DBOpenHelper.ColumnsIndex.NOTE_UPDATE_DATE);

        return noteItem;
    }

    @Override
    public List<NoteItem> getFolderAndAllItems() {
        updateFromDB();
        return mNoteList.getList();
    }

    @Override
    public NoteItem getNoteItem(int id) {
        updateFromDB();
        if (DEBUG) {
            Log.v(TAG, "getNoteItem, id-->" + id);
        }
        NoteItem item = mNoteList.getNoteItemByID(id);
        if (item == null && id > 0) {
            item = getNoteItemFromDB(id);
            initData(mContext);
        }
        return item;
    }

    @Override
    public int insertItem(NoteItem item) {
        if (DEBUG) {
            Log.d(TAG, "Enter insertItem");
        }
        ContentValues cv = buildValuesNoID(item);
        Uri uri = mContext.getApplicationContext().getContentResolver().insert(NOTE_URI, cv);
        /* SPRD: bug 520197 sqlitefull  @{ */
        if (uri == null) {
            showSqliteFullNotification();
            return -1;
        } else {
            Log.d(TAG, "---uri.LastPath=" + uri.getLastPathSegment());
            item._id = Integer.valueOf(uri.getLastPathSegment());
            if (DEBUG) {
                Log.d(TAG, "---item._id=" + item._id);
            }
            mNoteList.addOneItem(item);
            return (int) item._id;
        }
        /* @} */
    }
    /* SPRD: bug 520197 sqlitefull @{ */
    private void showSqliteFullNotification() {
        NotificationManager mNotificationManager = (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        NotificationCompat.Builder mBuilder = new NotificationCompat.Builder(mContext);
        Resources mResources = mContext.getResources();
        String title = mResources.getString(R.string.sqlite_full);
        String mTag = KEY_SQLITE_TAG;
        Log.d(TAG, "showSqliteFullNotification");
        mNotificationManager.notify(
                mTag,
                0,
                mBuilder.setAutoCancel(true)
                        .setContentTitle(title)
                        .setContentText(title)
                        .setTicker(title)
                        .setOngoing(false)
                        .setProgress(0, 0, false)
                        .setSmallIcon(android.R.drawable.stat_sys_warning)
                        .setContentIntent(
                                PendingIntent.getActivity(mContext, mTag.hashCode(), new Intent(),
                                        PendingIntent.FLAG_CANCEL_CURRENT)).build());
        Toast.makeText(mContext, title, Toast.LENGTH_LONG).show();
    }
    /* @} */
    ContentValues buildValuesNoID(NoteItem item) {
        ContentValues cv = new ContentValues();
        cv.put(DBOpenHelper.NOTE_CONTENT, item.content);
        cv.put(DBOpenHelper.NOTE_IS_FOLDER, item.isFileFolder ? 1 : 0);
        cv.put(DBOpenHelper.NOTE_PARENT_FOLDER, item.parentFolderId);
        cv.put(DBOpenHelper.NOTE_UPDATE_DATE, item.date);
        cv.put(DBOpenHelper.NOTE_TITLE, item.title == null ? "" : item.title);

        return cv;
    }

    @Override
    public int updateItem(NoteItem item) {
        if (DEBUG) {
            Log.d(TAG, "Enter updateItem");
        }
        ContentValues cv = buildValuesNoID(item);
        int id = mContext.getApplicationContext().getContentResolver().update(NoteProvider.NOTE_CONTENT_URI,
                        cv, DBOpenHelper.ID + "=?", new String[] {item._id + ""});
        mNoteList.updateOneItem(item);
        return id;
    }

    @Override
    public void deleteNoteItem(NoteItem item) {
        if (DEBUG) {
            Log.d(TAG, "Enter deleteNoteItem");
        }
        if (item == null) {
            return;
        }
        String noteWhereClause = " _id = ? or " + DBOpenHelper.NOTE_PARENT_FOLDER + " = ? ";
        mContext.getApplicationContext().getContentResolver().delete(NoteProvider.NOTE_CONTENT_URI,
                noteWhereClause, new String[] {item._id + "", item._id + ""});
        mNoteList.deleteNoteItemOrFolder(item);
    }

    @Override
    public void deleteAllNotes() {
        mContext.getApplicationContext().getContentResolver().delete(NoteProvider.NOTE_CONTENT_URI, 
                null, null);
        mNoteList.clear();
    }

    public List<NoteItem> getNotes() {
        updateFromDB();
        return mNoteList.getNotes();
    }

    @Override
    public List<NoteItem> getFolders() {
        updateFromDB();
        return mNoteList.getFolderList();
    }

    @Override
    public List<NoteItem> getNotesFromFolder(int folderID) {
        updateFromDB();
        return mNoteList.getNotesFromFolder(folderID);
    }

    @Override
    public List<NoteItem> getRootFoldersAndNotes() {
        updateFromDB();
        return mNoteList.getRootFoldersAndNotes();
    }

    @Override
    public List<NoteItem> getRootNotes() {
        updateFromDB();
        return mNoteList.getRootNotes();
    }

    private void updateFromDB() {
        if (mNeedUpdate) {
            Log.d(TAG, "updateFromDB");
            initData(mContext);
        }
    }

    public void updateCacheFromDB() {
        mNeedUpdate = true;
    }

    @Override
    public NoteItem getNoteItemFromDB(int id) {
        Cursor cursor = null;
        NoteItem noteItem = null;
        try {
            cursor = mContext.getContentResolver().query(NOTE_URI, DBOpenHelper.NOTE_ALL_COLUMS, "_id=?", new String[] {id + ""}, null);
            if (cursor == null) {
                return null;
            }
            while (cursor.moveToNext()) {
                noteItem = buildNoteItem(cursor, mContext);
            }
        } finally {
            if (cursor != null) {
                cursor.close();
                cursor = null;
            }
        }
        return noteItem;
    }

    @Override
    public List<NoteItem> getNotesIncludeContent(String content) {
        updateFromDB();
        return mNoteList.getNotesIncludeContent(content);
    }

    @Override
    public Cursor query(String[] columns, String selection, String[] selectionArgs, String groupBy,
            String having, String sortOrder, String limit) {
        return mContext.getContentResolver().query(NOTE_URI, columns, selection, selectionArgs, sortOrder);
    }

    @Override
    public void setCursor(Cursor cursor) {
        mCursor = cursor;
        initData(mContext);
    }
}
