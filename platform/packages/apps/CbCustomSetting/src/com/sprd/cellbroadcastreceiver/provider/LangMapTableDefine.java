package com.sprd.cellbroadcastreceiver.provider;

public interface LangMapTableDefine {

    public static final String LANG_MAP_TABLE_NAME  = "lang_map";

    public static final String _ID                  = "_id";
    public static final String LANG_ID              = "lang_id";
    public static final String MNC_MCC_ID           = "mnc_mcc_id";
    public static final String SHOW                 = "show";
    public static final String ENABLE               = "enable";
    public static final String SUBID                = "sub_id";
    
    public static final String[] QUERY_COLUMNS = { _ID, LANG_ID, MNC_MCC_ID,
            SHOW, ENABLE, SUBID };

    public static final String CREATE_LANG_MAP_TABLE
        = "CREATE TABLE " + LANG_MAP_TABLE_NAME + " ("
            + _ID + " INTEGER PRIMARY KEY AUTOINCREMENT,"
            + LANG_ID + " INTEGER,"
            + MNC_MCC_ID + " INTEGER,"
            + SHOW + " INTEGER,"
            + ENABLE + " INTEGER,"
            + SUBID + " INTEGER);";

}
