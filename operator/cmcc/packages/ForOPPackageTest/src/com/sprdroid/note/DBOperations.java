package com.sprdroid.note;

/* 封装数据库增删改查操作

 */
import java.sql.Time;
import java.util.Calendar;
import java.sql.Date;
import java.text.ParseException;
import java.text.SimpleDateFormat;

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.CursorLoader;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteQueryBuilder;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;

public class DBOperations extends ContentProvider {

//    private Context mContext;
    DBOpenHelper helper;
    SQLiteDatabase db;

    private static final int EVENTS = 1;
    private static final int EVENTS_ID = 2;

    private static final int CLOCK = 3;
    private static final int CLOCK_ID = 4;

    private static final UriMatcher sURLMatcher = new UriMatcher(
            UriMatcher.NO_MATCH);

    static {
        sURLMatcher.addURI("com.sprdroid.note", "items", EVENTS);
        sURLMatcher.addURI("com.sprdroid.note", "items/#", EVENTS_ID);

        sURLMatcher.addURI("com.sprdroid.note", "clock", CLOCK);
        sURLMatcher.addURI("com.sprdroid.note", "clock/#", CLOCK_ID);
    }
    
    // 添加记录
    public int insert(Context context, String content, String cdate,
            String ctime, String alarm_enable, String bgColor, String isFile,
            String parentFile) {
//        mContext = context;
        //helper = new DBOpenHelper(mContext);
		helper = DBOpenHelper.getInstance(context);
        db = helper.getWritableDatabase();

        ContentValues cv = new ContentValues();
        cv.put(DBOpenHelper.CONTENT, content);
        cv.put(DBOpenHelper.UPDATE_DATE, cdate);
        cv.put(DBOpenHelper.UPDATE_TIME, ctime);
        cv.put(DBOpenHelper.ALARM_ENABLE, alarm_enable);
        cv.put(DBOpenHelper.BG_COLOR, bgColor);
        cv.put(DBOpenHelper.IS_FOLDER, isFile);
        cv.put(DBOpenHelper.PARENT_FOLDER, parentFile);
        long lid = db.insert(DBOpenHelper.TABLE_NAME, DBOpenHelper.CONTENT, cv);
        // 关闭数据库
        //this.close();
        int id = (int) lid;
        return id;
    }

    // 删除记录(如果被删记录是文件夹,则删除文件夹下的所有记录)
    public void delete(Context context, int _id) {
//        mContext = context;
        //helper = new DBOpenHelper(mContext);
		helper = DBOpenHelper.getInstance(context);
        db = helper.getWritableDatabase();

        String id = String.valueOf(_id);
        db.delete(DBOpenHelper.TABLE_NAME, " _id = ? or "
                + DBOpenHelper.PARENT_FOLDER + " = ? ", new String[] { id, id });

        // 如果对应的闹钟项存在，则删除之
        String where = DBOpenHelper.CLOCK_ID + "=?";
        String[] whereValues = { String.valueOf(id) };
        Cursor cursor = db.query(DBOpenHelper.CLOCK_TABLE_NAME, null, where,
                whereValues, null, null, null);
        Log.v("you", "delete Clock: cursor=" + cursor);
        if (cursor != null) {
            Log.v("you", "delete Clock: id=" + id);
            db.delete(DBOpenHelper.CLOCK_TABLE_NAME, " _id = ? ",
                    new String[] { id });
        }
        cursor.close();
        // 关闭数据库
        //this.close();
    }

    // 修改记录
    public void update(Context context, int _id, String content,
            int alarm_enable, String bgColor, String parentFile) {
//        mContext = context;
        //helper = new DBOpenHelper(mContext);
		helper = DBOpenHelper.getInstance(context);
        db = helper.getWritableDatabase();

        ContentValues cv = new ContentValues();
        cv.put(DBOpenHelper.CONTENT, content);
        cv.put(DBOpenHelper.UPDATE_DATE, getDate());
        cv.put(DBOpenHelper.UPDATE_TIME, getTime());
        cv.put(DBOpenHelper.ALARM_ENABLE, alarm_enable);
        cv.put(DBOpenHelper.BG_COLOR, bgColor);
        cv.put(DBOpenHelper.PARENT_FOLDER, parentFile);
        // 更新记录的条件
        String whereClause = " _id = ? ";
        // 更新记录的条件的参数
        String[] whereArgs = new String[] { String.valueOf(_id) };

        db.update(DBOpenHelper.TABLE_NAME, cv, whereClause, whereArgs);

        // 关闭数据库
        //this.close();
    }

    // 更新闹钟enable
    public void update_alarm_enable(Context context, int _id,
            int alarm_enable) {
//        mContext = context;
        //helper = new DBOpenHelper(mContext);
		helper = DBOpenHelper.getInstance(context);
        db = helper.getWritableDatabase();

        ContentValues cv1 = new ContentValues();
        ContentValues cv2 = new ContentValues();
        cv1.put(DBOpenHelper.ALARM_ENABLE, alarm_enable);
        cv2.put(DBOpenHelper.CLOCK_ISOPEN, alarm_enable);

        // 更新记录的条件
        String whereClause = " _id = ? ";
        // 更新记录的条件的参数
        String[] whereArgs = new String[] { String.valueOf(_id) };

        db.update(DBOpenHelper.TABLE_NAME, cv1, whereClause, whereArgs);
        db.update(DBOpenHelper.CLOCK_TABLE_NAME, cv2, whereClause, whereArgs);
        // 关闭数据库
        //this.close();

    }

    // 查询单条记录的详细内容
    public Cursor queryOneNote(Context context, int _id) {
//        mContext = context;
        //helper = new DBOpenHelper(mContext);
		helper = DBOpenHelper.getInstance(context);
        db = helper.getWritableDatabase();

        String[] columns = new String[] { DBOpenHelper.CONTENT,
                DBOpenHelper.UPDATE_DATE, DBOpenHelper.UPDATE_TIME,
                DBOpenHelper.ALARM_ENABLE, DBOpenHelper.BG_COLOR,
                DBOpenHelper.ID, DBOpenHelper.IS_FOLDER,
                DBOpenHelper.PARENT_FOLDER };
        Cursor c = db.query(DBOpenHelper.TABLE_NAME, columns, " _id = ? ",
                new String[] { String.valueOf(_id) }, null, null, null);
        // 关闭数据库
        // this.close(db);

        return c;
    }

    // 查询文件夹下的记录(便签)
    public Cursor queryFromFolder(Context context, int _id) {
//        mContext = context;
        //helper = new DBOpenHelper(mContext);
		helper = DBOpenHelper.getInstance(context);
        db = helper.getWritableDatabase();

        String[] columns = new String[] { DBOpenHelper.CONTENT,
                DBOpenHelper.UPDATE_DATE, DBOpenHelper.UPDATE_TIME,
                DBOpenHelper.ALARM_ENABLE, DBOpenHelper.BG_COLOR,
                DBOpenHelper.ID, DBOpenHelper.IS_FOLDER,
                DBOpenHelper.PARENT_FOLDER };
        // 排序方法
        String orderBy = DBOpenHelper.UPDATE_DATE + " desc ,"
                + DBOpenHelper.UPDATE_TIME + " desc";
        Cursor c = db.query(DBOpenHelper.TABLE_NAME, columns,
                DBOpenHelper.PARENT_FOLDER + "  = ? ",
                new String[] { String.valueOf(_id) }, null, null, orderBy);
        // 关闭数据库
        // this.close();

        return c;
    }

    // 所有便签的数量(不管是否在文件夹下,不查询文件夹)
    public int queryAllNotes(Context context) {
//        mContext = context;
        //helper = new DBOpenHelper(mContext);
        // 待查询的列
        String[] columns = new String[] { DBOpenHelper.CONTENT,
                DBOpenHelper.UPDATE_DATE, DBOpenHelper.UPDATE_TIME,
                DBOpenHelper.ALARM_ENABLE, DBOpenHelper.BG_COLOR,
                DBOpenHelper.ID, DBOpenHelper.IS_FOLDER,
                DBOpenHelper.PARENT_FOLDER };
        // 排序方法
        String orderBy = DBOpenHelper.UPDATE_DATE + " desc ,"
                + DBOpenHelper.UPDATE_TIME + " desc";
        Cursor c = db.query(DBOpenHelper.TABLE_NAME, columns,
                DBOpenHelper.IS_FOLDER + "  = ? ", new String[] { "no" }, null,
                null, orderBy);
        
        int in = 0;
        // 关闭数据库
        //this.close();    
        if(c != null){
        	in = c.getCount();
        	c.close();
        }
        return in;
    }

    // 查询所有文件夹类型的记录
    public Cursor queryAllFolders(Context context) {
//        mContext = context;
        //helper = new DBOpenHelper(mContext);
		helper = DBOpenHelper.getInstance(context);
        db = helper.getWritableDatabase();
        // 待查询的列
        String[] columns = new String[] { DBOpenHelper.CONTENT,
                DBOpenHelper.UPDATE_DATE, DBOpenHelper.UPDATE_TIME,
                DBOpenHelper.ALARM_ENABLE, DBOpenHelper.BG_COLOR,
                DBOpenHelper.ID, DBOpenHelper.IS_FOLDER,
                DBOpenHelper.PARENT_FOLDER };
        // 排序方法
        String orderBy = DBOpenHelper.UPDATE_DATE + " desc ,"
                + DBOpenHelper.UPDATE_TIME + " desc";
        Cursor c = db.query(DBOpenHelper.TABLE_NAME, columns,
                DBOpenHelper.IS_FOLDER + "  = ? ", new String[] { "yes" },
                null, null, orderBy);
        // 关闭数据库
        // this.close(db);

        return c;
    }

    // 查询所有顶级便签
    public Cursor queryAllRootNotes(Context context) {
        //mContext = context;
        //helper = new DBOpenHelper(mContext);
		helper = DBOpenHelper.getInstance(context);
        db = helper.getWritableDatabase();
        // 待查询的列
        String[] columns = new String[] { DBOpenHelper.CONTENT,
                DBOpenHelper.UPDATE_DATE, DBOpenHelper.UPDATE_TIME,
                DBOpenHelper.ALARM_ENABLE, DBOpenHelper.BG_COLOR,
                DBOpenHelper.ID, DBOpenHelper.IS_FOLDER,
                DBOpenHelper.PARENT_FOLDER };
        // 排序方法
        String orderBy = DBOpenHelper.IS_FOLDER + "  desc , "
                + DBOpenHelper.UPDATE_DATE + " desc , "
                + DBOpenHelper.UPDATE_TIME + " desc";

        String selection = DBOpenHelper.IS_FOLDER + " = '" + "no" + "' and "
                + DBOpenHelper.PARENT_FOLDER + " = '" + "no'";

        Cursor c = db.query(DBOpenHelper.TABLE_NAME, columns, selection, null,
                null, null, orderBy);
        // 关闭数据库
        // this.close(db);

        return c;
    }

    // 查询全部文件夹和顶级便签记录
    public Cursor queryFoldersAndNotes(Context context) {
     //   mContext = context;
        //helper = new DBOpenHelper(mContext);
		helper = DBOpenHelper.getInstance(context);
        db = helper.getWritableDatabase();

        String[] columns = new String[] { DBOpenHelper.CONTENT,
                DBOpenHelper.UPDATE_DATE, DBOpenHelper.UPDATE_TIME,
                DBOpenHelper.ALARM_ENABLE, DBOpenHelper.BG_COLOR,
                DBOpenHelper.ID, DBOpenHelper.IS_FOLDER,
                DBOpenHelper.PARENT_FOLDER };
        // 排序方法
        String orderBy = DBOpenHelper.IS_FOLDER + "  desc , "
                + DBOpenHelper.UPDATE_DATE + " desc , "
                + DBOpenHelper.UPDATE_TIME + " desc";

        String selection = DBOpenHelper.IS_FOLDER + " = '" + "yes" + "' or "
                + DBOpenHelper.PARENT_FOLDER + " = '" + "no'";
        Cursor c = db.query(DBOpenHelper.TABLE_NAME, columns, selection, null,
                null, null, orderBy);
        // 关闭数据库
        // this.close();

        return c;
    }

    // 查询数据库中所有记录(不区分是否为文件夹)
    public Cursor queryAllRecords(Context context) {
//        mContext = context;
        //helper = new DBOpenHelper(mContext);
		helper = DBOpenHelper.getInstance(context);
        db = helper.getWritableDatabase();

        String[] columns = new String[] { DBOpenHelper.CONTENT,
                DBOpenHelper.UPDATE_DATE, DBOpenHelper.UPDATE_TIME,
                DBOpenHelper.ALARM_ENABLE, DBOpenHelper.BG_COLOR,
                DBOpenHelper.IS_FOLDER, DBOpenHelper.PARENT_FOLDER,
                DBOpenHelper.ID };
        // 排序方法
        String orderBy = DBOpenHelper.IS_FOLDER + "  desc , "
                + DBOpenHelper.UPDATE_DATE + " desc , "
                + DBOpenHelper.UPDATE_TIME + " desc";

        Cursor c = db.query(DBOpenHelper.TABLE_NAME, columns, null, null, null,
                null, orderBy);
        // 关闭数据库
        // this.close();
        return c;
    }

    // 日期操作
    public String getDate() {
        Calendar c = Calendar.getInstance();
        //int theYear = c.get(Calendar.YEAR);     
        //int theMonth = c.get(Calendar.MONTH);
        //int theDay = c.get(Calendar.DAY_OF_MONTH);
        //return (new Date(theYear, theMonth, theDay)).toString();
        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd");
        
        return dateFormat.format(c.getTime());
    }

    // 时间操作
    public String getTime() {
        Calendar c = Calendar.getInstance();
        int theHour = c.get(Calendar.HOUR_OF_DAY);
        int theMinute = c.get(Calendar.MINUTE);
        int theSecond = c.get(Calendar.SECOND);
        return (new Time(theHour, theMinute, theSecond)).toString();
    }

    // 对闹钟的操作
    public int insertClock(Context context, int id, ContentValues cv) {
//        mContext = context;
        //helper = new DBOpenHelper(mContext);
        db.insert(DBOpenHelper.CLOCK_TABLE_NAME, DBOpenHelper.CONTENT, cv);

        Log.v("you", "insertClock:id=" + id);
        // 关闭数据库
        //this.close();
        return id;
    }

    public void deleteClock(Context context, int id) {
//        mContext = context;
        //helper = new DBOpenHelper(mContext);

        String where = DBOpenHelper.CLOCK_ID + "=?";
        String[] whereValues = { String.valueOf(id) };
        db.delete(DBOpenHelper.CLOCK_TABLE_NAME, where, whereValues);
        // 关闭数据库
        //this.close();
    }

    public int updateClock(Context context, int id, ContentValues cv) {
//        mContext = context;
        //helper = new DBOpenHelper(mContext);

        String where = DBOpenHelper.CLOCK_ID + "=?";
        String[] whereValues = { String.valueOf(id) };
        int ret = db.update(DBOpenHelper.CLOCK_TABLE_NAME, cv, where,
                whereValues);
        Log.v("you", "updateclock: id=" + id);
        // 关闭数据库
        //this.close();
        return ret;
    }

    public Cursor getClock(Context context, int id) {
        helper = new DBOpenHelper(context);

        db = helper.getWritableDatabase();
        // helper.onCreate(db);
        String where = DBOpenHelper.CLOCK_ID + "=?";
        String[] whereValues = { String.valueOf(id) };
        Log.v("you", "getClock: valueOf(id)=" + String.valueOf(id));
        Cursor cursor = db.query(DBOpenHelper.CLOCK_TABLE_NAME, null, where,
                whereValues, null, null, null);

        return cursor;
    }

	//查询所有状态为enable的鬧鍾便簽
    public Cursor queryAllClocksEnable(Context context) {
//        mContext = context;
        // 待查询的列
        String[] columns = new String[] { DBOpenHelper.CONTENT,
                DBOpenHelper.UPDATE_DATE, DBOpenHelper.UPDATE_TIME,
                DBOpenHelper.ALARM_ENABLE, DBOpenHelper.BG_COLOR,
                DBOpenHelper.ID, DBOpenHelper.IS_FOLDER,
                DBOpenHelper.PARENT_FOLDER };
        // 排序方法
        String orderBy = DBOpenHelper.UPDATE_DATE + " desc ,"
                + DBOpenHelper.UPDATE_TIME + " desc";
        Cursor c = db.query(DBOpenHelper.TABLE_NAME, columns,
                DBOpenHelper.ALARM_ENABLE + "  = ? ", new String[] { context.getString(R.string.yes) },
                null, null, orderBy);
        // 关闭数据库
        // this.close(db);

        return c;
    }

	@Override
	public boolean onCreate() {

		helper = DBOpenHelper.getInstance(getContext());
        db = helper.getWritableDatabase();
		return true;
	}

	@Override
	public Cursor query(Uri uri, String[] projection, String selection,
			String[] selectionArgs, String sortOrder) {
		SQLiteQueryBuilder qb = new SQLiteQueryBuilder();

        // Generate the body of the query
		Log.d("Notes","DB   query()  uri = "+uri);
        int match = sURLMatcher.match(uri);
        switch (match) {
        case EVENTS:
            qb.setTables("items");
            break;
        case EVENTS_ID:
            qb.setTables("items");
            qb.appendWhere("_id=");
            qb.appendWhere(uri.getPathSegments().get(1));
            break;
        case CLOCK:
            qb.setTables("clock");
            break;
        case CLOCK_ID:
            qb.setTables("clock");
            qb.appendWhere("_id=");
            qb.appendWhere(uri.getPathSegments().get(1));
            break;
        default:
            throw new IllegalArgumentException("Unknown URL " + uri);
    }

		return qb.query(db, projection, selection, selectionArgs,
                null, null, sortOrder);
	}

	@Override
	public String getType(Uri uri) {
        int match = sURLMatcher.match(uri);
        switch (match) {
            case EVENTS:
                return "vnd.android.cursor.dir/items";
            case EVENTS_ID:
                return "vnd.android.cursor.item/items";
            case CLOCK:
                return "vnd.android.cursor.dir/clock";
            case CLOCK_ID:
                return "vnd.android.cursor.item/clock";
            default:
                throw new IllegalArgumentException("Unknown URL");
        }
	}

	@Override
	public Uri insert(Uri uri, ContentValues values) {
		if (sURLMatcher.match(uri) != EVENTS) {
			throw new IllegalArgumentException("Cannot insert into URL: " + uri);
		}
		Uri newUrl = null;
		SQLiteDatabase db = helper.getWritableDatabase();
		switch (sURLMatcher.match(uri)) {
		case EVENTS:
			long rowId = db.insert("items", DBOpenHelper.CONTENT, values);
			if (rowId < 0) {
				throw new SQLException("Failed to insert row");
			}

			newUrl = ContentUris.withAppendedId(
					Uri.parse("content://com.sprdroid.note/items"), rowId);

			break;
		case CLOCK:
			long clockId = db.insert("clock", DBOpenHelper.CLOCK_ID, values);
			if (clockId < 0) {
				throw new SQLException("Failed to insert row");
			}

			newUrl = ContentUris.withAppendedId(
					Uri.parse("content://com.sprdroid.note/clock"), clockId);

			break;
		}
		return newUrl;
	}

	@Override
	public int delete(Uri uri, String selection, String[] selectionArgs) {
		SQLiteDatabase db = helper.getWritableDatabase();
		int count;
		long rowId = 0;
		switch (sURLMatcher.match(uri)) {
		case EVENTS:
			count = db.delete("items", selection, selectionArgs);
			break;
		case EVENTS_ID:
			String segment = uri.getPathSegments().get(1);
			rowId = Long.parseLong(segment);
			if (TextUtils.isEmpty(selection)) {
				selection = "_id=" + segment;
			} else {
				selection = "_id=" + segment + " AND (" + selection + ")";
			}
			count = db.delete("items", selection, selectionArgs);
			break;
		case CLOCK:
			count = db.delete("clock", selection, selectionArgs);
			break;
		case CLOCK_ID:
			String segmentClock = uri.getPathSegments().get(1);
			rowId = Long.parseLong(segmentClock);
			if (TextUtils.isEmpty(selection)) {
				selection = "_id=" + segmentClock;
			} else {
				selection = "_id=" + segmentClock + " AND (" + selection + ")";
			}
			count = db.delete("clock", selection, selectionArgs);
			break;
		default:
			throw new IllegalArgumentException("Cannot delete from URL: " + uri);
		}

		return count;
	}

	@Override
	public int update(Uri uri, ContentValues values, String selection,
			String[] selectionArgs) {
        int count;
        long rowId = 0;
        int match = sURLMatcher.match(uri);
        SQLiteDatabase db = helper.getWritableDatabase();
        switch (match) {
            case EVENTS_ID: {
                String segment = uri.getPathSegments().get(1);
                rowId = Long.parseLong(segment);
                count = db.update("items", values, "_id=" + rowId, null);
                break;
            }
            case CLOCK_ID: {
                String segment = uri.getPathSegments().get(1);
                rowId = Long.parseLong(segment);
                count = db.update("clock", values, "_id=" + rowId, null);
                break;
            }
            default: {
                throw new UnsupportedOperationException(
                        "Cannot update URL: " + uri);
            }
        }
        return count;
	}
}
