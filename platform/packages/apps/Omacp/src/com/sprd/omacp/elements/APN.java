
package com.sprd.omacp.elements;

import com.sprd.xml.parser.impl.BaseSet;
import com.sprd.xml.parser.prv.OTAKeyDefine;

public class APN extends BaseSet {
    public APN(int nType) {
        super(nType);
    }

    public boolean initEvn() {
        super.initEvn();
        addMainKeySet();
        addOptKeySet();
        return true;
    }

    private void addMainKeySet() {
        addMainKey(OTAKeyDefine.NAPID);
        // addMainKey(OTAKeyDefine.PXADDR);
        // addMainKey(OTAKeyDefine.PROXY_ID);
        // addMainKey(OTAKeyDefine.TO_NAPID);
        addMainKey(OTAKeyDefine.NAP_ADDRESS);
        addMainKey(OTAKeyDefine.NAPDEF_NAME);
        // addMainKey(OTAKeyDefine.PXLOGICAL_NAME);
        // addMainKey(OTAKeyDefine.PHYSICAL_PROXY_ID);
    }

    private void addOptKeySet() {
        addOptionKey(OTAKeyDefine.PROXY_PW);
        addOptionKey(OTAKeyDefine.PPGAUTH_TYPE);
        addOptionKey(OTAKeyDefine.DOMAIN);
        addOptionKey(OTAKeyDefine.TRUST);
        addOptionKey(OTAKeyDefine.MASTER);
        addOptionKey(OTAKeyDefine.STARTPAGE);
        addOptionKey(OTAKeyDefine.BASAUTH_ID);
        addOptionKey(OTAKeyDefine.BASAUTH_PW);
        addOptionKey(OTAKeyDefine.PROXY_PROVIDER_ID);
        addOptionKey(OTAKeyDefine.WSP_VERSION);
        addOptionKey(OTAKeyDefine.PUSHENABLED);
        addOptionKey(OTAKeyDefine.PULLEANABLED);
        addOptionKey(OTAKeyDefine.RULE);
        addOptionKey(OTAKeyDefine.APPID);
        addOptionKey(OTAKeyDefine.TO_PROXY);
        addOptionKey(OTAKeyDefine.PXADDRTYPE);
        addOptionKey(OTAKeyDefine.PXADDR_FQDN);
        addOptionKey(OTAKeyDefine.PXAUTH_TYPE);
        addOptionKey(OTAKeyDefine.PXAUTH_ID);
        addOptionKey(OTAKeyDefine.PXAUTH_PW);
        addOptionKey(OTAKeyDefine.BEARER);
        addOptionKey(OTAKeyDefine.INTERNET);
        addOptionKey(OTAKeyDefine.NAP_ADDRTYPE);
        addOptionKey(OTAKeyDefine.DNS_ADDR);
        addOptionKey(OTAKeyDefine.CALLTYPE);
        addOptionKey(OTAKeyDefine.LOCAL_ADDR);
        addOptionKey(OTAKeyDefine.LOCAL_ADDRTYPE);
        addOptionKey(OTAKeyDefine.LINKSPEED);
        addOptionKey(OTAKeyDefine.DNLINKSPEED);
        addOptionKey(OTAKeyDefine.LINGER);
        addOptionKey(OTAKeyDefine.DELIVERY_ERR_SDU);
        addOptionKey(OTAKeyDefine.DELIVERY_ORDER);
        addOptionKey(OTAKeyDefine.TRAFFIC_CLASS);
        addOptionKey(OTAKeyDefine.MAX_SDU_SIZE);
        addOptionKey(OTAKeyDefine.MAX_BITRATE_UPLINK);
        addOptionKey(OTAKeyDefine.MAX_BITRATE_DNLINK);
        addOptionKey(OTAKeyDefine.RESIDUAL_BER);
        addOptionKey(OTAKeyDefine.SDU_ERRO_RATIO);
        addOptionKey(OTAKeyDefine.TRAFFIC_HANDL_PRIO);
        addOptionKey(OTAKeyDefine.TRAFFIC_DELAY);
        addOptionKey(OTAKeyDefine.GUARANTEED_BITRATE_UPLINK);
        addOptionKey(OTAKeyDefine.GUARANTEED_BITRATE_DNLINK);
        addOptionKey(OTAKeyDefine.MAX_NUM_RETRY);
        addOptionKey(OTAKeyDefine.FIRST_RETRY_TIMEOUT);
        addOptionKey(OTAKeyDefine.REGER_THRESHOLD);
        addOptionKey(OTAKeyDefine.T_BIT);
        addOptionKey(OTAKeyDefine.AUTHTYPE);
        addOptionKey(OTAKeyDefine.AUTHNAME);
        addOptionKey(OTAKeyDefine.AUTHSECRET);
        addOptionKey(OTAKeyDefine.AUTH_ENTITY);
        addOptionKey(OTAKeyDefine.SIP);
        addOptionKey(OTAKeyDefine.COUNTRY);
        addOptionKey(OTAKeyDefine.NETWORK);
        addOptionKey(OTAKeyDefine.SID);
        addOptionKey(OTAKeyDefine.SOC);
        addOptionKey(OTAKeyDefine.VALIDUNTIL);
        addOptionKey(OTAKeyDefine.CLIENT_ID);
        addOptionKey(OTAKeyDefine.PROVURL);
        addOptionKey(OTAKeyDefine.CONTEXT_ALLOW);
        addOptionKey(OTAKeyDefine.PORTNBR);
        addOptionKey(OTAKeyDefine.SERVICE);
    }
}
