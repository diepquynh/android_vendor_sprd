package com.sprd.cellbroadcastreceiver.provider;

import com.sprd.cellbroadcastreceiver.provider.MncMccTableDefine;

public interface CreateLangViewDefine {
    
    public static final String VIEW_LANG_NAME = "view_lang";
    public static final String _ID = "_id";

    public static final String INDEX_ID                = "_id";
    public static final String INDEX_MNC               = "mnc";
    public static final String INDEX_MCC               = "mcc";
    public static final String INDEX_LANGUAGE_ID       = "lang_id";
    public static final String INDEX_SUBID             = "sub_id";
    public static final String INDEX_DESCRIPTION       = "description";
    public static final String INDEX_SHOW              = "show";
    public static final String INDEX_ENABLED           = "enable";
    public static final String INDEX_MNCMCCID          = "mncmcc_id";

    public static final String[] QUERY_COLUMNS = { LangMapTableDefine._ID,
        MncMccTableDefine.MNC, MncMccTableDefine.MCC, MncMccTableDefine.MNCMCC_ID,
        LangMapTableDefine.LANG_ID, LangMapTableDefine.SUBID, LangNameTableDefine.DESCRIPTION,
        LangMapTableDefine.SHOW, LangMapTableDefine.ENABLE };
    
    public static final String CREATE_LANG_VIEW =
            "CREATE VIEW " + VIEW_LANG_NAME + " AS "
                + "SELECT "
                + "b." + LangMapTableDefine._ID +", " 
                + "a." + MncMccTableDefine.MNC  + ", "
                + "a." + MncMccTableDefine.MCC + ", "
                + "a." + MncMccTableDefine.MNCMCC_ID + ", "
                + "b." + LangMapTableDefine.LANG_ID + ", "
                + "b." + LangMapTableDefine.SUBID + ", "
                + "c." + LangNameTableDefine.DESCRIPTION + ", "
                + "b." + LangMapTableDefine.SHOW + ", "
                + "b." + LangMapTableDefine.ENABLE
                + " FROM "
                + MncMccTableDefine.MNC_MCC_TABLE_NAME + " AS a" + ", " 
                + LangMapTableDefine.LANG_MAP_TABLE_NAME + " AS b" + ", " 
                + LangNameTableDefine.LANG_NAME_TABLE_NAME + " AS c"
                + " WHERE " + LangMapTableDefine.LANG_ID + " = " + LangNameTableDefine.LANG_NAME_ID + " AND "
                + LangMapTableDefine.MNC_MCC_ID + " = " + MncMccTableDefine.MNCMCC_ID;

}
