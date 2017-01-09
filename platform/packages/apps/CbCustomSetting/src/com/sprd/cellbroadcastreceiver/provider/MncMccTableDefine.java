package com.sprd.cellbroadcastreceiver.provider;

public interface MncMccTableDefine {

    public static final String MNC_MCC_TABLE_NAME = "mnc_mcc";

    public static final String _ID = "_id";
    public static final String MNCMCC_ID = "mncmcc_id";
    public static final String MNC = "mnc";
    public static final String MCC = "mcc";

    public static final String[] QUERY_COLUMNS = { _ID, MNCMCC_ID, MNC, MCC, };

    public static final String CREATE_MNC_MCC_TABLE =
            "CREATE TABLE "+ MNC_MCC_TABLE_NAME + " ("
            + _ID + " INTEGER PRIMARY KEY AUTOINCREMENT,"
            + MNCMCC_ID + " INTEGER,"
            + MNC + " INTEGER,"
            + MCC + " INTEGER);";

}
