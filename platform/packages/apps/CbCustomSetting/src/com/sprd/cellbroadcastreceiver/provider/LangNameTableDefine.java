package com.sprd.cellbroadcastreceiver.provider;

public interface LangNameTableDefine {

    public static final String LANG_NAME_TABLE_NAME = "lang_name";

    public static final String _ID                  = "_id";
    public static final String LANG_NAME_ID         = "lang_name_id";
    public static final String DESCRIPTION          = "description";
    
    public static final String[] QUERY_COLUMNS = { _ID, LANG_NAME_ID,
            DESCRIPTION };

    public static final String CREATE_LANG_NAME_TABLE
        = "CREATE TABLE " + LANG_NAME_TABLE_NAME + " ("
            + _ID + " INTEGER PRIMARY KEY AUTOINCREMENT,"
            + LANG_NAME_ID + " INTEGER,"
            + DESCRIPTION + " TEXT);";

}
