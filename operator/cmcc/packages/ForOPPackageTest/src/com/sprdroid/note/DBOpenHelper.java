package com.sprdroid.note;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

public class DBOpenHelper extends SQLiteOpenHelper {
    // 数据库版本号
    private static final int DB_VERSION = 1;
    // 数据库名称
    private static final String DB_NAME = "Notes1.db";
    // 数据库中的表的名称
    public static final String TABLE_NAME = "items";

    // 数据库表中的字段:八列
    // 主键
    public static final String ID = "_id";
    // 便签内容
    public static final String CONTENT = "content";
    // 最后更新的日期(date类型)
    public static final String UPDATE_DATE = "cdate";
    // 最后更新的时间(time类型)
    public static final String UPDATE_TIME = "ctime";
    // 闹钟提醒时间(time类型)
    // public static final String ALARM_TIME = "atime";
    // public static final String ALARM_ID = "alarm_id";
    // 闹钟是否开启
    public static final String ALARM_ENABLE = "alarm_enable";
    // 便签的背景颜色(varchar类型)
    public static final String BG_COLOR = "bgcolor";
    // 标识是否为文件夹(varchar类型,yes和no来区分)
    public static final String IS_FOLDER = "isfile";
    // 如果不是文件夹,本字段存储其所属文件夹(varchar类型(存储被标记为文件夹的记录的_id字段的值))
    public static final String PARENT_FOLDER = "parentfile";

    // 闹钟设置表
    public final static String CLOCK_TABLE_NAME = "clock";
    public final static String CLOCK_ID = "_id";
    public final static String CLOCK_ISOPEN = "isopen";
    public final static String CLOCK_DATE = "date";
    public final static String CLOCK_TIME = "time";
    public final static String CLOCK_ISREPEAT = "isrepeat";
    public final static String CLOCK_ISVIBRATE = "isvibrate";
    public final static String CLOCK_RINGS = "rings";
    public final static String CLOCK_URI = "uri";
    
	private static DBOpenHelper helper  = null;	
	public static DBOpenHelper getInstance(Context context)
	{
		if(helper == null)
		{
			helper = new DBOpenHelper(context);
		}
		return helper;
		
	}

    public DBOpenHelper(Context context) {
        super(context, DB_NAME, null, DB_VERSION);
    }


    /*
     * @Override public void onOpen(SQLiteDatabase db) { super.onOpen(db);
     * if(!db.isReadOnly()) { // Enable foreign key constraints
     * db.execSQL("PRAGMA foreign_keys=ON;"); } }
     */
    @Override
    public void onCreate(SQLiteDatabase db) {

        String sql = "CREATE TABLE IF NOT EXISTS " + CLOCK_TABLE_NAME + " ("
                + CLOCK_ID + " integer primary key , " + CLOCK_ISOPEN
                + " integer, " + CLOCK_DATE + " text, " + CLOCK_TIME + " text, "
                + CLOCK_ISREPEAT + " text, " + CLOCK_ISVIBRATE + " integer, "
                + CLOCK_RINGS + " text, " + CLOCK_URI + " text );";
        db.execSQL(sql);
        Log.v("you", "Create Table: " + CLOCK_TABLE_NAME);

        db.execSQL(" CREATE TABLE IF NOT EXISTS " + TABLE_NAME + " ( " + ID
                + " integer primary key autoincrement , " + CONTENT
                + " text , " + UPDATE_DATE + " date , " + UPDATE_TIME
                + " time , "
                // + ALARM_ID +
                // " integer ,FOREIGN KEY ("+ALARM_ID+") REFERENCES "+CLOCK_TABLE_NAME+" ("+CLOCK_ID+" ), "
                + ALARM_ENABLE + " integer , " + BG_COLOR + " varchar , "
                + IS_FOLDER + " varchar , " + PARENT_FOLDER + " varchar );");
        Log.v("you", "Create Table: " + TABLE_NAME);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        // 数据库升级调用此函数,需要重写，不然编译不过。
        db.execSQL(" DROP TABLE IF EXISTS " + TABLE_NAME);
        db.execSQL(" DROP TABLE IF EXISTS " + CLOCK_TABLE_NAME);
        onCreate(db);
    }

}
