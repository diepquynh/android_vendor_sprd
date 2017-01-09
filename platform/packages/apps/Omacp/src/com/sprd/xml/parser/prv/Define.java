
package com.sprd.xml.parser.prv;

import android.R.integer;

public class Define {
    // OMACP version
    public static final int OMA_VERSION = 1100;

    // state define
    public static final int STATE_OK = 0;
    public static final int ERROR_PIN_ERROR = 1;
    public static final int ERROR_PIN_EXCEPTION = 2;
    public static final int ERROR_MIME_ERROR = 3;
    public static final int ERROR_RETRY = 4;
    public static final int ERROR_NO_APN = 5;
    public static final int STATE_PARAM_ERROR = 6;
    public static final int STATE_ERROR = 7;

    public static final int CHAR_PORT = 1;
    public static final int CHAR_ACCESS = 2;
    public static final int CHAR_NAPDEF = 3;
    public static final int CHAR_PXLOGICAL = 4;
    public static final int CHAR_BOOTSTRAP = 5;
    public static final int CHAR_APPLICATION = 6;
    public static final int CHAR_VENDORCONFIG = 7;
    public static final int CHAR_CLIENTIDENTITY = 8;

    // define APN Email Browse type
    public static final int TYPE_APN = 10;
    public static final int TYPE_MMS = 11;
    public static final int TYPE_EMAIL = 12;
    public static final int TYPE_BROWSE = 13;
    public static final int TYPE_BOOKMARK = 14;
    public static final int TYPE_STARTPAGE = 15;
    public static final int TYPE_EXCHANGE = 16;

    // define BootStrap data
    public static final String PROXY_ID = "proxy-id";

    // define APN data
    public static final String NAPID = "napid";
    public static final String APPID = "appid";
    public static final String TO_NAPID = "to-napid";
    public static final String TO_PROXY = "to-proxy";

    // define APP type
    public static final String MMS_APPID = "w4";
    public static final String BROWSE_APPID = "w2";
    public static final String POP_SSL = "995";
    public static final String POP_DEFAULT = "110";
    public static final String IMAP_SSL = "993";
    public static final String IMAP_DEFAULT = "143";
    public static final String SMTP_SSL = "465";
    public static final String SMTP_DEFAULT = "25";

    // define bwxml final string
    public static final String PARM_NODE_NAME = "parm";
    public static final String PARM_NODE_ATTR_NAME = "name";
    public static final String CHAR_NODE_ATTR_NAME_TYPE = "type";
    public static final String PARM_NODE_ATTR_VALUE = "value";
    public static final String CHAR_NODE_ATTR_RESOURCE = "resource";
    public static final String PARM_NODE_ATTR_STARTPAGE = "startpage";
    public static final String CHAR_NODE_ATTR_APPLICATION = "application";
    public static final String CHAR_NODE_CHARACTERISTIC = "characteristic";

    // define Browse data
    public static final String BROWSE_BOOKMARK = "bookmark";
    public static final String BROWSE_STARTPAGE = "start page";
    // add for bug 515141 begin
    public static final String INTERNET = "INTERNET";
    // add for bug 515141 end

    public static final long THREAD_ALL = -1;
    public static final long THREAD_NONE = -2;

    // OMACP Receiver action
    public final static String ACTION_APN = "com.android.ApnDataConfig";
    public final static String ACTION_EMAIL = "com.android.EmailDataConfig";
    public final static String ACTION_BROWSER = "com.andorid.BrowserDataConfig";

    public static int RECEIVE_FLAG_APN = 0x00000001;
    public static int RECEIVE_FLAG_EMAIL = 0x00000002;
    public static int RECEIVE_FLAG_BROWSER = 0x00000004;

    // define OMACP Tables
    public static final String[] TAG_TABLE_OTA = {
            "wap-provisioningdoc", // 05
            "characteristic", // 06
            "parm", // 07
    };

    public static final String[] TAG_TABLE_OTA1 = {
            "", // 05
            "characteristic", // 06
            "parm", // 07
    };

    public static final String[] ATTR_START_TABLE_OTA = {
            "name", // 0x05
            "value", // 0x06
            "name=NAME", // 0x07
            "name=NAP-ADDRESS", // 0x08
            "name=NAP-ADDRTYPE", // 0x09
            "name=CALLTYPE", // 0x0A
            "name=VALIDUNTIL", // 0x0B
            "name=AUTHTYPE", // 0x0C
            "name=AUTHNAME", // 0x0D
            "name=AUTHSECRET", // 0x0E
            "name=LINGER", // 0x0F
            "name=BEARER", // 0x10
            "name=NAPID", // 0x11
            "name=COUNTRY", // 0x12
            "name=NETWORK", // 0x13
            "name=INTERNET", // 0x14
            "name=PROXY-ID", // 0x15
            "name=PROXY-PROVIDER-ID", // 0x16
            "name=DOMAIN", // 0x17
            "name=PROVURL", // 0x18
            "name=PXAUTH-TYPE", // 0x19
            "name=PXAUTH-ID", // 0x1A
            "name=PXAUTH-PW", // 0x1B
            "name=STARTPAGE", // 0x1C
            "name=BASAUTH-ID", // 0x1D
            "name=BASAUTH-PW", // 0x1E
            "name=PUSHENABLED", // 0x1F
            "name=PXADDR", // 0x20
            "name=PXADDRTYPE", // 0x21
            "name=TO-NAPID", // 0x22
            "name=PORTNBR", // 0x23
            "name=SERVICE", // 0x24
            "name=LINKSPEED", // 0x25
            "name=DNLINKSPEED", // 0x26
            "name=LOCAL-ADDR", // 0x27
            "name=LOCAL-ADDRTYPE", // 0x28
            "name=CONTEXT-ALLOW", // 0x29
            "name=TRUST", // 0x2A
            "name=MASTER", // 0x2B
            "name=SID", // 0x2C
            "name=SOC", // 0x2D
            "name=WSP-VERSION", // 0x2E
            "name=PHYSICAL-PROXY-ID", // 0x2F
            "name=CLIENT-ID", // 0x30
            "name=DELIVERY-ERR-SDU", // 0x31
            "name=DELIVERY-ORDER", // 0x32
            "name=TRAFFIC-CLASS", // 0x33
            "name=MAX-SDU-SIZE", // 0x34
            "name=MAX-BITRATE-UPLINK", // 0x35
            "name=MAX-BITRATE-DNLINK", // 0x36
            "name=RESIDUAL-BER", // 0x37
            "name=SDU-ERROR-RATIO", // 0x38
            "name=TRAFFIC-HANDL-PRIO", // 0x39
            "name=TRANSFER-DELAY", // 0x3A
            "name=GUARANTEED-BITRATE-UPLINK", // 0x3B
            "name=GUARANTEED-BITRATE-DNLINK", // 0x3C
            "name=PXADDR-FQDN", // 0x3D
            "name=PROXY-PW", // 0x3E
            "name=PPGAUTH-TYPE", // 0x3F
            "", // 0x40
            "", // 0x41
            "", // 0x42
            "", // 0x43
            "", // 0x44
            "version", // 0x45
            "version=1.0", // 0x46
            "", // 0x47 //need buquan add by jordan
            "", // 0x48
            "", // 0x49
            "", // 0x4A
            "", // 0x4B
            "", // 0x4C
            "", // 0x4D
            "name=AUTH-ENTITY", // 0x4E
            "name=SPI", // 0x4F
            "type", // 0x50
            "type=PXLOGICAL", // 0x51
            "type=PXPHYSICAL", // 0x52
            "type=PORT", // 0x53
            "type=VALIDITY", // 0x54
            "type=NAPDEF", // 0x55
            "type=BOOTSTRAP", // 0x56
            "type=VENDORCONFIG", // 0x57
            "type=CLIENTIDENTITY", // 0x58
            "type=PXAUTHINFO", // 0x59
            "type=NAPAUTHINFO", // 0x5A
            "type=ACCESS", // 0x5B
    };

    public static final String[] ATTR_START_TABLE_OTA1 = {
            "name", // 0x05
            "value", // 0x06
            "name=NAME", // 0x07
            "", // 0x08
            "", // 0x09
            "", // 0x0A
            "", // 0x0B
            "", // 0x0C
            "", // 0x0D
            "", // 0x0E
            "", // 0x0F
            "", // 0x10
            "", // 0x11
            "", // 0x12
            "", // 0x13
            "name=INTERNET", // 0x14
            "", // 0x15
            "", // 0x16
            "", // 0x17
            "", // 0x18
            "", // 0x19
            "", // 0x1A
            "", // 0x1B
            "name=STARTPAGE", // 0x1C
            "", // 0x1D
            "", // 0x1E
            "", // 0x1F
            "", // 0x20
            "", // 0x21
            "name=TO-NAPID", // 0x22
            "name=PORTNBR", // 0x23
            "name=SERVICE", // 0x24
            "", // 0x25
            "", // 0x26
            "", // 0x27
            "", // 0x28
            "", // 0x29
            "", // 0x2A
            "", // 0x2B
            "", // 0x2C
            "", // 0x2D
            "name=AACCEPT", // 0x2E
            "name=AAUTHDATA", // 0x2F
            "name=AAUTHLEVEL", // 0x30
            "name=AAUTHNAME", // 0x31
            "name=AAUTHSECRET", // 0x32
            "name=AAUTHTYPE", // 0x33
            "name=ADDR", // 0x34
            "name=ADDRTYPE", // 0x35
            "name=APPID", // 0x36
            "name=APROTOCOL", // 0x37
            "name=PROVIDER-ID", // 0x38
            "name=TO-PROXY", // 0x39
            "name=URI", // 0x3A
            "name=RULE", // 0x3B
            "", // 0x3C
            "", // 0x3D
            "", // 0x3E
            "", // 0x3F
            "", // 0x40
            "", // 0x41
            "", // 0x42
            "", // 0x43
            "", // 0x44
            "", // 0x45
            "", // 0x46
            "", // 0x47
            "", // 0x48
            "", // 0x49
            "", // 0x4A
            "", // 0x4B
            "", // 0x4C
            "", // 0x4D
            "", // 0x4E
            "", // 0x4F
            "type", // 0x50
            "", // 0x51
            "", // 0x52
            "type=PORT", // 0x53
            "", // 0x54
            "type=APPLICATION", // 0x55
            "type=APPADDR", // 0x56
            "type=APPAUTH", // 0x57
            "", // 0x58
            "type=RESOURCE", // 0x59
    };

    public static final String[] ATTR_VALUE_TABLE_OTA = {
            "IPV4", // 0x85
            "IPV6", // 0x86
            "E164", // 0x87
            "ALPHA", // 0x88
            "APN", // 0x89
            "SCODE", // 0x8A
            "TETRA-ITSI", // 0x8B
            "MAN", // 0x8C
            "APPSRV", // 0x8D
            "OBEX", // 0x8E
            "", // 0x8F
            "ANALOG-MODEM", // 0x90
            "V.120", // 0x91
            "V.110", // 0x92
            "X.31", // 0x93
            "BIT-TRANSPARENT", // 0x94
            "DIRECT-ASYNCHRONOUS-DATA-SERVICE", // 0x95
            "", // 0x96
            "", // 0x97
            "", // 0x98
            "", // 0x99
            "PAP", // 0x9A
            "CHAP", // 0x9B
            "HTTP-BASIC", // 0x9C
            "HTTP-DIGEST", // 0x9D
            "WTLS-SS", // 0x9E
            "MD5", // 0x9F
            "", // 0xA0
            "", // 0xA1
            "GSM-USSD", // 0xA2
            "GSM-SMS", // 0xA3
            "ANSI-136-GUTS", // 0xA4
            "IS-95-CDMA-SMS", // 0xA5
            "IS-95-CDMA-CSD", // 0xA6
            "IS-95-CDMA-PACKET", // 0xA7
            "ANSI-136-CSD", // 0xA8
            "ANSI-136-GPRS", // 0xA9
            "GSM-CSD", // 0xAA
            "GSM-GPRS", // 0xAB
            "AMPS-CDPD", // 0xAC
            "PDC-CSD", // 0xAD
            "PDC-PACKET", // 0xAE
            "IDEN-SMS", // 0xAF
            "IDEN-CSD", // 0xB0
            "IDEN-PACKET", // 0xB1
            "FLEX/REFLEX", // 0xB2
            "PHS-SMS", // 0xB3
            "PHS-CSD", // 0xB4
            "TETRA-SDS", // 0xB5
            "TETRA-PACKET", // 0xB6
            "ANSI-136-GHOST", // 0xB7
            "MOBITEX-MPAK", // 0xB8
            "CDMA2000-1X-SIMPLE-IP", // 0xB9
            "CDMA2000-1X-MOBILE-IP", // 0xBA
            "", // 0xBB
            "", // 0xBC
            "", // 0xBD
            "", // 0xBE
            "", // 0xBF
            "", // 0xC0
            "", // 0xC1
            "", // 0xC2
            "", // 0xC3
            "", // 0xC4
            "AUTOBAUDING", // 0xC5
            "", // 0xC6
            "", // 0xC7
            "", // 0xC8
            "", // 0xC9
            "CL-WSP", // 0xCA
            "CO-WSP", // 0xCB
            "CL-SEC-WSP", // 0xCC
            "CO-SEC-WSP", // 0xCD
            "CL-SEC-WTA", // 0xCE
            "CO-SEC-WTA", // 0xCF
            "OTA-HTTP-TO", // 0xD0
            "OTA-HTTP-TLS-TO", // 0xD1
            "OTA-HTTP-PO", // 0xD2
            "OTA-HTTP-TLS-PO", // 0xD3
            "", // 0xD4
            "", // 0xD5
            "", // 0xD6
            "", // 0xD7
            "", // 0xD8
            "", // 0xD9
            "", // 0xDA
            "", // 0xDB
            "", // 0xDC
            "", // 0xDD
            "", // 0xDE
            "", // 0xDF
            "AAA", // 0xE0
            "HA", // 0xE1
    };

    public static final String[] ATTR_VALUE_TABLE_OTA1 = {
            "value=IPV4", // 0x85 jordan not found in OMACP SPEC.
            "IPV6", // 0x86
            "E164", // 0x87
            "ALPHA", // 0x88
            "", // 0x89
            "", // 0x8A
            "", // 0x8B
            "", // 0x8C
            "APPSRV", // 0x8D
            "OBEX", // 0x8E
            "", // 0x8F
            ",", // 0x90
            "HTTP-", // 0x91
            "BASIC", // 0x92
            "DIGEST", // 0x93
    };

    // define Nokia Tables
    public static final String[] NOKIA_TAG_TOKENS = {
            "CHARACTERISTIC-LIST", // 0x05
            "CHARACTERISTIC", // 0x06
            "PARM" // 0x07
    };

    public static final String[] NOKIA_TAG_TYPE = {
            "ADDRESS", "URL", "MMSURL", "NAME", "BOOKMARK", "ID"
    };

    public static final String[] NOKIA_ATTRIBUTE_START_TOKENS = {
            "", // 0x05
            "TYPE=ADDRESS", // 0x06
            "TYPE=URL", // 0x07
            "TYPE=NAME", // 0x08
            "", // 0x09
            "", // 0x0A
            "", // 0x0B
            "", // 0x0C
            "", // 0x0D
            "", // 0x0E
            "", // 0x0F
            "NAME", // 0x10
            "VALUE", // 0x11
            "NAME=BEARER", // 0x12
            "name=PROXY", // 0x13
            "name=PORT", // 0x14
            "name=NAME", // 0x15
            "name=PROXY_TYPE", // 0x16
            "name=URL", // 0x17
            "name=PROXY_AUTHNAME", // 0x18
            "name=PROXY_AUTHSECRET", // 0x19
            "name=SMS_SMSC_ADDRESS", // 0x1A
            "name=USSD_SERVICE_CODE", // 0x1B
            "name=GPRS_ACCESSPOINTNAME", // 0x1C
            "name=PPP_LOGINTYPE", // 0x1D
            "name=PROXY_LOGINTYPE", // 0x1E
            "", // 0x1F
            "", // 0x20
            "name=CSD_DIALSTRING", // 0x21
            "name=PPP_AUTHTYPE", // 0x22
            "name=PPP_AUTHNAME", // 0x23
            "name=PPP_AUTHSECRET", // 0x24
            "", // 0x25
            "", // 0x26
            "", // 0x27
            "name=CSD_CALLTYPE", // 0x28
            "name=CSD_CALLSPEED", // 0x29
            "", // 0x2a
            "", // 0x2b
            "", // 0x2c
            "", // 0x2d
            "", // 0x2e
            "", // 0x2f
            "", // 0x30
            "", // 0x31
            "", // 0x32
            "", // 0x33
            "", // 0x34
            "", // 0x35
            "", // 0x36
            "", // 0x37
            "", // 0x38
            "", // 0x39
            "", // 0x3a
            "", // 0x3b
            "", // 0x3c
            "", // 0x3d
            "", // 0x3e
            "", // 0x3f
            "", // 0x40
            "", // 0x41
            "", // 0x42
            "", // 0x43
            "", // 0x44
            "value=GSM/CSD", // 0x45
            "value=GSM/SMS", // 0x46
            "value=GSM/USSD", // 0x47
            "value=IS-136/CSD", // 0x48
            "value=GPRS", // 0x49
            "", // 0x4a
            "", // 0x4b
            "", // 0x4c
            "", // 0x4d
            "", // 0x4e
            "", // 0x4f
            "", // 0x50
            "", // 0x51
            "", // 0x52
            "", // 0x53
            "", // 0x54
            "", // 0x55
            "", // 0x56
            "", // 0x57
            "", // 0x58
            "", // 0x59
            "", // 0x5a
            "", // 0x5b
            "", // 0x5c
            "", // 0x5d
            "", // 0x5e
            "", // 0x5f
            "value=9200", // 0x60
            "value=9201", // 0x61
            "value=9202", // 0x62
            "value=9203", // 0x63
            "value=AUTOMATIC", // 0x64
            "value=MANUAL", // 0x65
            "", // 0x66
            "", // 0x67
            "", // 0x68
            "", // 0x69
            "value=AUTO", // 0x6A
            "value=9600", // 0x6B
            "value=14400", // 0x6C
            "value=19200", // 0x6D
            "value=28800", // 0x6E
            "value=38400", // 0x6F
            "value=PAP", // 0x70
            "value=CHAP", // 0x71
            "value=ANALOGUE", // 0x72
            "value=ISDN", // 0x73
            "value=43200", // 0x74
            "value=57600", // 0x75
            "value=MSISDN_NO", // 0x76
            "value=IPV4", // 0x77
            "value=MS_CHAP", // 0x78
            "", // 0x79
            "", // 0x7a
            "", // 0x7b
            "TYPE=MMSURL", // 0x7C
            "TYPE=ID", // 0x7D
            "NAME=ISP_NAME", // 0x7E
            "TYPE=BOOKMARK" // 0x7F
    };

}
