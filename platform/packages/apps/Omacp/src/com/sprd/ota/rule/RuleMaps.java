
package com.sprd.ota.rule;

import java.util.HashMap;
import java.util.Set;

/**
 * String : RootPath KeySet: All Child Parameter set Use Path to find keyset,
 * judging whether keyset contains the element or not.
 */
public class RuleMaps extends HashMap<String, RuleList> {

    public RuleMaps() {
        initEnv();
    }

    private void initEnv() {
        addPxlogicalList();
        addPxPortList();
        addPxPhyList();
        addPxPhyPortList();
        addNapdefList();
        addNapAuthList();
        addBootStrapList();
        addAppList();
        addAppAddrPortList();
        addAccessList();
    }

    private void addPxlogicalList() {
        RuleList pxList = new RuleList();
        RuleItem domain = new RuleItem(PXLOGICAL_DOMAIN, RuleItem.ZeroOrMore);
        pxList.add(domain);
        this.put(PATH_PXLOGICAL, pxList);
    }

    private void addPxPortList() {
        RuleList pxPortList = new RuleList();
        RuleItem service = new RuleItem(PXLOGICAL_PORT_SERVICE, RuleItem.ZeroOrMore);
        pxPortList.add(service);
        this.put(PATH_PXLOGICAL_PORT, pxPortList);
    }

    private void addPxPhyList() {
        RuleList pxPhyList = new RuleList();
        RuleItem domain = new RuleItem(PXLOGICAL_PXPHYSICAL_DOMAIN, RuleItem.ZeroOrMore);
        RuleItem toNapid = new RuleItem(PXLOGICAL_PXPHYSICAL_TONAPID, RuleItem.OneOrMore);
        pxPhyList.add(domain);
        pxPhyList.add(toNapid);
        this.put(PATH_PXLOGICAL_PXPHYSICAL, pxPhyList);
    }

    private void addPxPhyPortList() {
        RuleList pxPhyPortList = new RuleList();
        RuleItem service = new RuleItem(PXLOGICAL_PXPHYSICAL_PORT_SERVICE, RuleItem.ZeroOrMore);
        pxPhyPortList.add(service);
        this.put(PATH_PXLOGICAL_PXPHYSICAL_PORT, pxPhyPortList);
    }

    private void addNapdefList() {
        RuleList napdefList = new RuleList();
        RuleItem bearer = new RuleItem(NAPDEF_BEARER, RuleItem.ZeroOrMore);
        RuleItem dnsAddr = new RuleItem(NAPDEF_DNS_ADDR, RuleItem.ZeroOrMore);
        napdefList.add(bearer);
        napdefList.add(dnsAddr);
        this.put(PATH_NAPDEF, napdefList);
    }

    private void addNapAuthList() {
        RuleList napAuthList = new RuleList();
        RuleItem entity = new RuleItem(NAPDEF_NAPAUTHINFO_ENTITY, RuleItem.ZeroOrMore);
        napAuthList.add(entity);
        this.put(PATH_NAPDEF_NAPAUTHINFO, napAuthList);
    }

    private void addBootStrapList() {
        RuleList bootList = new RuleList();
        RuleItem netWork = new RuleItem(BOOTSTRAP_NETWORK, RuleItem.ZeroOrMore);
        RuleItem proxyId = new RuleItem(BOOTSTRAP_PROXY_ID, RuleItem.ZeroOrMore);
        bootList.add(netWork);
        bootList.add(proxyId);
        this.put(PATH_BOOTSTRAP, bootList);
    }

    private void addAppList() {
        RuleList appList = new RuleList();
        RuleItem toProxy = new RuleItem(APPLICATION_TOPROXY, RuleItem.ZeroOrMore);
        RuleItem toNapId = new RuleItem(APPLICATION_TONAPID, RuleItem.ZeroOrMore);
        RuleItem addr = new RuleItem(APPLICATION_ADDR, RuleItem.ZeroOrMore);
        appList.add(toProxy);
        appList.add(toNapId);
        appList.add(addr);
        this.put(PATH_APPLICATION, appList);
    }

    private void addAppAddrPortList() {
        RuleList portList = new RuleList();
        RuleItem service = new RuleItem(APPLICATION_APPADDR_PORT_SERVICE, RuleItem.ZeroOrMore);
        portList.add(service);
        this.put(PATH_APPLICATION_APPADDR_PORT, portList);
    }

    private void addAccessList() {
        RuleList accessList = new RuleList();
        RuleItem appId = new RuleItem(ACCESS_APPID, RuleItem.ZeroOrMore);
        RuleItem portnbr = new RuleItem(ACCESS_PORTNBR, RuleItem.ZeroOrMore);
        RuleItem rule = new RuleItem(ACCESS_RULE, RuleItem.OneOrMore);
        RuleItem toNapid = new RuleItem(ACCESS_TONAPID, RuleItem.ZeroOrMore);
        RuleItem toProxy = new RuleItem(ACCESS_TOPROXY, RuleItem.ZeroOrMore);
        RuleItem domain = new RuleItem(ACCESS_DOMAIN, RuleItem.ZeroOrMore);
        accessList.add(appId);
        accessList.add(portnbr);
        accessList.add(rule);
        accessList.add(toProxy);
        accessList.add(domain);
        accessList.add(toNapid);
        this.put(PATH_ACCESS, accessList);
    }

    public void Debug() {
        System.out.println("<<<------------------begin RuleMaps-----------------------");
        Set<String> keySet = this.keySet();
        for (String key : keySet) {
            System.out.println("key = [" + key + "]");
            this.get(key).Debug();
        }
        System.out.println("------------------end RuleMaps----------------------->>>");

    }

    // ACCESS
    public static final String PATH_ACCESS = "ACCESS";

    // PXLOGICAL
    public static final String PATH_PXLOGICAL = "PXLOGICAL";
    public static final String PATH_PXLOGICAL_PXAUTHINFO = "PXLOGICAL/PXAUTHINFO";
    public static final String PATH_PXLOGICAL_PORT = "PXLOGICAL/PORT";
    public static final String PATH_PXLOGICAL_PXPHYSICAL = "PXLOGICAL/PXPHYSICAL";
    public static final String PATH_PXLOGICAL_PXPHYSICAL_PORT = "PXLOGICAL/PXPHYSICAL/PORT";

    // NAPDEF
    public static final String PATH_NAPDEF = "NAPDEF";
    public static final String PATH_NAPDEF_VALIDITY = "NAPDEF/VALIDITY";
    public static final String PATH_NAPDEF_NAPAUTHINFO = "NAPDEF/NAPAUTHINFO";

    // BOOTSTRAP
    public static final String PATH_BOOTSTRAP = "BOOTSTRAP";

    // VENDORCONFIG
    public static final String PATH_VENDORCONFIG = "VENDORCONFIG";

    // APPLICATION
    public static final String PATH_APPLICATION = "APPLICATION";
    public static final String PATH_APPLICATION_APPADDR = "APPLICATION/APPADDR";
    public static final String PATH_APPLICATION_APPADDR_PORT = "APPLICATION/APPADDR/PORT";
    public static final String PATH_APPLICATION_APPAUTH = "APPLICATION/APPAUTH";
    public static final String PATH_APPLICATION_RESOURCE = "APPLICATION/RESOURCE";

    // the parm name
    public static final String PXLOGICAL_DOMAIN = "DOMAIN";
    public static final String PXLOGICAL_PORT_SERVICE = "SERVICE";
    public static final String PXLOGICAL_PXPHYSICAL_DOMAIN = "DOMAIN";
    public static final String PXLOGICAL_PXPHYSICAL_TONAPID = "TO-NAPID";
    public static final String PXLOGICAL_PXPHYSICAL_PORT_SERVICE = "SERVICE";
    public static final String NAPDEF_BEARER = "BEARER";
    public static final String NAPDEF_DNS_ADDR = "DNS-ADDR";
    public static final String NAPDEF_NAPAUTHINFO_ENTITY = "AUTH-ENTITY";
    public static final String BOOTSTRAP_NETWORK = "NETWORK";
    public static final String BOOTSTRAP_PROXY_ID = "PROXY-ID";
    public static final String APPLICATION_TOPROXY = "TO-PROXY";
    public static final String APPLICATION_TONAPID = "TO-NAPID";
    public static final String APPLICATION_ADDR = "ADDR";
    public static final String APPLICATION_APPADDR_PORT_SERVICE = "SERVICE";
    public static final String ACCESS_RULE = "RULE";
    public static final String ACCESS_APPID = "APPID";
    public static final String ACCESS_DOMAIN = "DOMAIN";
    public static final String ACCESS_PORTNBR = "PORTNBR";
    public static final String ACCESS_TONAPID = "TO-NAPID";
    public static final String ACCESS_TOPROXY = "TO-PROXY";

}
