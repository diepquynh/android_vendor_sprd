
package com.sprd.xml.parser.prv;

public class OTAKeyDefine {

    /******************************************************************************************
     * APN {Main, Option}
     ******************************************************************************************/
    /** Main Key **/
    public static final String NAPID = "napid"; // (m)
    public static final String PXADDR = "pxaddr"; // (m)

    public static final String PROXY_ID = "proxy-id"; // (m)
    public static final String TO_NAPID = "to-napid"; // (m)
    public static final String NAPDEF_NAME = "napdef_name"; // (m)
    public static final String NAP_ADDRESS = "nap-address"; // (m)

    public static final String PXLOGICAL_NAME = "pxlogical_name"; // (m)
    public static final String PHYSICAL_PROXY_ID = "physical-proxy-id"; // (m)

    /** Option Key **/
    public static final String TRUST = "trust"; // (o)
    public static final String DOMAIN = "domain"; // (o)
    public static final String MASTER = "master"; // (o)
    public static final String STARTPAGE = "startpage"; // (o)
    public static final String PUSHENABLED = "pushenabled"; // (o)
    public static final String PULLEANABLED = "pullenabled"; // (o)

    public static final String PROXY_PW = "proxy-pw"; // (o)
    public static final String BASAUTH_ID = "basauth-id"; // (o)
    public static final String BASAUTH_PW = "basauth-pw"; // (o)

    public static final String PPGAUTH_TYPE = "ppgauth-type"; // (o)
    public static final String WSP_VERSION = "wsp-version"; // (o)
    public static final String PROXY_PROVIDER_ID = "proxy_provider-id"; // (o)

    public static final String SIP = "sip"; // (o)
    public static final String SID = "sid"; // (o)
    public static final String SOC = "soc"; // (o)
    public static final String RULE = "rule"; // (o)
    public static final String APPID = "appid"; // (o)
    public static final String LINGER = "linger"; // (o)
    public static final String BEARER = "bearer"; // (o)

    public static final String PROVURL = "provurl"; // (o)
    public static final String PORTNBR = "portnbr"; // (o)
    public static final String SERVICE = "servive"; // (o)
    public static final String COUNTRY = "country"; // (o)
    public static final String NETWORK = "network"; // (o)
    public static final String CALLTYPE = "calltype"; // (o)
    public static final String INTERNET = "internet"; // (o)
    public static final String AUTHTYPE = "authtype"; // (o)
    public static final String AUTHNAME = "authname"; // (o)
    public static final String LINKSPEED = "linkspeed"; // (o)
    public static final String VALIDUNTIL = "validuntil"; // (o)
    public static final String AUTHSECRET = "authsecret"; // (o)
    public static final String DNLINKSPEED = "dnlinkspeed"; // (o)
    public static final String PXADDRTYPE = "pxaddrtype"; // (o)

    public static final String T_BIT = "t-bit"; // (o)
    public static final String DNS_ADDR = "dns-addr"; // (o)
    public static final String TO_PROXY = "to-proxy"; // (0)
    public static final String PXAUTH_ID = "pxauth-id"; // (o)
    public static final String PXAUTH_PW = "pxauth-pw"; // (o)
    public static final String LOCAL_ADDR = "local-addr"; // (o)
    public static final String PXADDR_FQDN = "pxaddr-fqdn"; // (o)
    public static final String PXAUTH_TYPE = "pxauth-type"; // (o)
    public static final String NAP_ADDRTYPE = "nap-addrtype"; // (o)
    public static final String LOCAL_ADDRTYPE = "local-addrtype"; // (o)

    public static final String RESIDUAL_BER = "residual-ber"; // (o)
    public static final String MAX_SDU_SIZE = "max-sdu-size"; // (o)
    public static final String TRAFFIC_CLASS = "traffic-class"; // (o)
    public static final String TRAFFIC_DELAY = "traffic-delay"; // (o)

    public static final String DELIVERY_ORDER = "delivery-order"; // (o)
    public static final String REGER_THRESHOLD = "reger-threshold"; // (o)

    public static final String CLIENT_ID = "client-id"; // (o)
    public static final String AUTH_ENTITY = "auth-entity"; // (o)
    public static final String CONTEXT_ALLOW = "context-allow"; // (o)
    public static final String SDU_ERRO_RATIO = "sdu-erro-ratio"; // (o)
    public static final String MAX_NUM_RETRY = "max-num-retry"; // (o)
    public static final String DELIVERY_ERR_SDU = "delivery-err-sdu"; // (o)
    public static final String MAX_BITRATE_UPLINK = "max-bitrate-uplink"; // (o)
    public static final String MAX_BITRATE_DNLINK = "max-bitrate-dnlink"; // (o)
    public static final String TRAFFIC_HANDL_PRIO = "traffic-handl-prio"; // (o)

    public static final String FIRST_RETRY_TIMEOUT = "first-retry-timeout"; // (o)
    public static final String GUARANTEED_BITRATE_UPLINK = "guaranteed-bitrate-uplink"; // (o)
    public static final String GUARANTEED_BITRATE_DNLINK = "guaranteed-bitrate-dnlink"; // (o)

    /******************************************************************************************
     * Email {Main, Option}
     ******************************************************************************************/
    /** Main Key **/
    public static final String EMAIL_APPLICATION_APPID = "appid"; // (m)
    /** Option Key **/
    public static final String EMAIL_APPLICATION_FROM = "from"; // (o)
    public static final String EMAIL_APPLICATION_TO_NAPID = "to-napid"; // (o)
    public static final String EMAIL_APPLICATION_TO_PROXY = "to-proxy"; // (o)
    public static final String EMAIL_APPLICATION_PROVIDER_ID = "provider-id"; // (o)
    public static final String EMAIL_APPLICATION_APPADD_ADDR = "addr"; // (o)
    public static final String EMAIL_APPLICATION_APPADD_ADDRTYPE = "addrtype"; // (o)
    public static final String EMAIL_APPLICATION_PORT_SERVICE = "service"; // (o)
    public static final String EMAIL_APPLICATION_PORT_PORTNBR = "portnbr"; // (o)
    public static final String EMAIL_APPLICATION_APPAUTH_AAUTHNAME = "aauthname"; // (o)
    public static final String EMAIL_APPLICATION_APPAUTH_AAUTHTYPE = "aauthtype"; // (o)
    public static final String EMAIL_APPLICATION_APPAUTH_AAUTHDATA = "aauthdata"; // (o)
    public static final String EMAIL_APPLICATION_APPAUTH_AAUTHLEVEL = "aauthlevel"; // (o)
    public static final String EMAIL_APPLICATION_APPAUTH_AAUTHSECRET = "aauthsecret"; // (o)

    // Exchange key
    public static final String EMAIL_APPLICATION_MON = "mon";
    public static final String EMAIL_APPLICATION_TUE = "tue";
    public static final String EMAIL_APPLICATION_WED = "wed";
    public static final String EMAIL_APPLICATION_THU = "thu";
    public static final String EMAIL_APPLICATION_FRI = "fri";
    public static final String EMAIL_APPLICATION_SAT = "sat";
    public static final String EMAIL_APPLICATION_SUN = "sun";
    public static final String EMAIL_APPLICATION_SSL = "ssl";
    public static final String EMAIL_APPLICATION_POPUP = "popup";
    public static final String EMAIL_APPLICATION_APPREF = "appref";
    public static final String EMAIL_APPLICATION_PEAKEND = "peakend";
    public static final String EMAIL_APPLICATION_DOMAIN = "domain";
    public static final String EMAIL_APPLICATION_CONFLICT = "conflict";
    public static final String EMAIL_APPLICATION_PEAKSTART = "peakstart";
    public static final String EMAIL_APPLICATION_SIGNATURE = "signature";
    public static final String EMAIL_APPLICATION_EMAILADDR = "emailaddr";
    public static final String EMAIL_APPLICATION_TASK_FILTER = "task_filter";
    public static final String EMAIL_APPLICATION_TASK_ENABLED = "task_enabled";
    public static final String EMAIL_APPLICATION_TASK_REPLACE = "task_replace";
    public static final String EMAIL_APPLICATION_EMAIL_FILTER = "email_filter";
    public static final String EMAIL_APPLICATION_PEAKFREQUENCY = "peakfrequency";
    public static final String EMAIL_APPLICATION_SYNCWHENROAMING = "syncwhenroaming";
    public static final String EMAIL_APPLICATION_OFFPEAKFREQUENCY = "offpeakfrequency";
    public static final String EMAIL_APPLICATION_CALENDAR_ENABLED = "calendar_enabled";
    public static final String EMAIL_APPLICATION_CALENDAR_FILTER = "calendar_filter";
    public static final String EMAIL_APPLICATION_CALENDAR_REPLACE = "calendar_replace";
    public static final String EMAIL_APPLICATION_CONTACT_FILTER = "contact_filter";
    public static final String EMAIL_APPLICATION_CONTACT_ENABLED = "contact_enabled";
    public static final String EMAIL_APPLICATION_CONTACT_REPLACE = "contact_replace";

    /******************************************************************************************
     * BookMark {Main, Option}
     ******************************************************************************************/
    /** Main Key **/
    public static final String BOOKMARK_APPLICATION_APPID = "appid"; // (m)
    public static final String BOOKMARK_APPLICATION_RESOURCE_URL = "uri"; // (m)
    /** Option Key **/
    public static final String BOOKMARK_APPLICATION_TO_NAPID = "to-napid"; // (o)
    public static final String BOOKMARK_APPLICATION_TO_PROXY = "to-proxy"; // (o)

    public static final String BOOKMARK_APPLICATION_NAME = "application_name"; // (o)
    public static final String BOOKMARK_APPLICATION_RESOURCE_NAME = "resource_name"; // (o)
    public static final String BOOKMARK_APPLICATION_RESOURCE_STARTPAGE = "startpage"; // (o)
    public static final String BOOKMARK_APPLICATION_RESOURCE_AAUTHTYPE = "aauthtype"; // (o)
    public static final String BOOKMARK_APPLICATION_RESOURCE_AAUTHNAME = "aauthname"; // (o)
    public static final String BOOKMARK_APPLICATION_RESOURCE_AAUTHSECRET = "aauthsecret"; // (o)
    public static final String BOOKMARK_APPLICATION_RESOURCE_AAUTHDATA = "aauthdata"; // (o)

    /******************************************************************************************
     * HomePage {Main, Option}
     ******************************************************************************************/
    /** Main Key **/
    public static final String HOMEPAGE_APPLICATION_APPID = "appid"; // (m)
    public static final String HOMEPAGE_APPLICATION_RESOURCE_URL = "uri"; // (m)
    /** Option Key **/
    public static final String HOMEPAGE_APPLICATION_TO_NAPID = "to-napid"; // (o)
    public static final String HOMEPAGE_APPLICATION_TO_PROXY = "to-proxy"; // (o)

    public static final String HOMEPAGE_APPLICATION_NAME = "application_name"; // (o)
    public static final String HOMEPAGE_APPLICATION_RESOURCE_NAME = "resource_name"; // (o)
    public static final String HOMEPAGE_APPLICATION_RESOURCE_STARTPAGE = "startpage"; // (o)
    public static final String HOMEPAGE_APPLICATION_RESOURCE_AAUTHTYPE = "aauthtype"; // (o)
    public static final String HOMEPAGE_APPLICATION_RESOURCE_AAUTHNAME = "aauthname"; // (o)
    public static final String HOMEPAGE_APPLICATION_RESOURCE_AAUTHSECRET = "aauthsecret"; // (o)
    public static final String HOMEPAGE_APPLICATION_RESOURCE_AAUTHDATA = "aauthdata"; // (o)

}
