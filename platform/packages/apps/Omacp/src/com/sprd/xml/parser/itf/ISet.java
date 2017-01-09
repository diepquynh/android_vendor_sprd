
package com.sprd.xml.parser.itf;

import java.util.HashSet;
import java.util.Set;

public interface ISet {

    public int getType();

    public int check(Set<String> obj);

    public boolean initEvn();

    public void debug();

    public static final int BASE_OK = 0x7F000000;
    public static final int BASE_FAILURE = 0xFF000000;
    public static final int ER_PARAM = (BASE_FAILURE | 0X00000001);
    public static final int ER_LOSE_MAIN_KEY = (BASE_FAILURE | 0X00000002);

    public static final int OK_LOSE_OPT_KEY = (BASE_OK | 0X00000001);

    /********************************************************************************************
     * Supported Type // extend class type define First: Define Type Second:
     * Implements Class Three: Add to Factory Create Instance; Four: Add Type To
     * Singleton;
     ********************************************************************************************/
    public static final int OTA_UNKNOW = 0xFFFFFFFF;
    public static final int OTA_APN = 0x00000001;
    public static final int OTA_EMAIL = 0x00000002;
    public static final int OTA_STARTPAGE = 0x00000003;
    public static final int OTA_BOOKMARK = 0x00000004;
}
