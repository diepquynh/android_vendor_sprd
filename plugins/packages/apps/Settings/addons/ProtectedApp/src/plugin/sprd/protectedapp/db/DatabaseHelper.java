package plugin.sprd.protectedapp.db;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

public class DatabaseHelper extends SQLiteOpenHelper {

    private static final String TAG = DatabaseHelper.class.getSimpleName();

    private static final String DATABASE_NAME = "security_clear.db";
    private static final int DATABASE_VERSION = 1;
    private static DatabaseHelper mSingleton;

    protected interface Tables {
        public static final String TABLE_SECURITY_INDEX = "security_index";
    }

    protected interface IndexColumns {
        public static final String ID = "_id";
        public static final String PKGNAME = "packagename";
        public static final String ENABLED = "enabled";
        public static final String USER_ID = "user_id";
    }

    protected String[] mColumnStrings = {
            IndexColumns.ID,
            IndexColumns.PKGNAME,
            IndexColumns.ENABLED,
            IndexColumns.USER_ID };

    private static final String CREATE_SECURITY_QUERIES_TABLE =
            "CREATE TABLE IF NOT EXISTS " + Tables.TABLE_SECURITY_INDEX +
                    "(" +
                    IndexColumns.ID + " integer primary key autoincrement" +
                    ", " +
                    IndexColumns.PKGNAME + " VARCHAR(64) NOT NULL" +
                    ", " +
                    IndexColumns.ENABLED + " INTEGER NOT NULL" +
                    ", " +
                    IndexColumns.USER_ID + " VARCHAR(32)" +
                    ")";

    public synchronized static DatabaseHelper getInstance(Context context) {
        if (mSingleton == null) {
            mSingleton = new DatabaseHelper(context);
        }
        return mSingleton;
    }

    public DatabaseHelper(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        createDB(db);
    }

    private void createDB(SQLiteDatabase db) {
        db.execSQL(CREATE_SECURITY_QUERIES_TABLE);
        Log.i(TAG, "Bootstrapped database");
    }

    @Override
    public void onOpen(SQLiteDatabase db) {
        super.onOpen(db);
        Log.i(TAG, "Using schema version: " + db.getVersion());
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        if (oldVersion < DATABASE_VERSION) {
            Log.w(TAG, "Detected schema version '" +  oldVersion + "'. " +
                    "Index needs to be rebuilt for schema version '" + newVersion + "'.");
            // We need to drop the tables and recreate them
            reconstruct(db);
        }
    }

    private void reconstruct(SQLiteDatabase db) {
        dropTables(db);
        createDB(db);
    }

    private void dropTables(SQLiteDatabase db) {
        Log.w(TAG, "drop table if exists");
        db.execSQL("DROP TABLE IF EXISTS " + Tables.TABLE_SECURITY_INDEX);
    }
}
