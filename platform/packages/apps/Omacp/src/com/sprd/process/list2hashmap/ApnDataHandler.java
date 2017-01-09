
package com.sprd.process.list2hashmap;

import java.util.Set;
import java.util.HashSet;
import java.util.HashMap;
import java.util.List;

import com.sprd.xml.parser.prv.Define;

public class ApnDataHandler extends BaseHandlerImpl {

    public ApnDataHandler() {
    }

    @Override
    public int process(OtaMapData data) {
        System.out.println("enter ApnHandler process()");
        if (null == data) {
            return Define.STATE_PARAM_ERROR;
        }
        if (apnProcess(data) != Define.STATE_OK) {
            System.out.println("otaApnProcess(data) != Define.STATE_OK");
            return Define.STATE_ERROR;
        }

        // Application APN Reference
        apnReference(data);
        return Define.STATE_OK;
    }

    private int apnProcess(OtaMapData data) {
        System.out.println("enter apnProcess()");
        if (null == data) {
            System.out.println("param is null in apnProcess()");
            return Define.STATE_PARAM_ERROR;
        }
        // one Apn point to HashMap
        String toNapId = "";
        // add for bug 515141 begin
        boolean isInternet = false;
        // add for bug 515141 end
        List<MyHashMap> pxLogicalList = data.get(Define.CHAR_PXLOGICAL);
        List<MyHashMap> napdefList = data.get(Define.CHAR_NAPDEF);
        HashMap<String, String> apnMap = new MyHashMap();
        if ((pxLogicalList == null || pxLogicalList.isEmpty())
                && (napdefList == null || napdefList.isEmpty())) {
            System.out.println("pxLogicalList and napdefList All empty");
            return Define.STATE_PARAM_ERROR;
        }
        if ((pxLogicalList == null || pxLogicalList.isEmpty()) && !napdefList.isEmpty()) {
            System.out.println("pxLogicalList empty, just add napdef to APN");
            for (HashMap<String, String> napdefMap : napdefList) {
                apnMap = getApnFromSingleMap(napdefMap);
                data.put(Define.TYPE_APN, (MyHashMap) apnMap);
            }
            return Define.STATE_OK;
        }
        if ((napdefList == null || napdefList.isEmpty()) && !pxLogicalList.isEmpty()) {
            System.out.println("napdef empty, just add pxLogicalList to APN");
            for (HashMap<String, String> logicalMap : pxLogicalList) {
                toNapId = logicalMap.get(Define.TO_NAPID);
                if (logicalMap.containsKey(Define.NAPID)
                        && logicalMap.get(Define.NAPID).equalsIgnoreCase(toNapId)) {
                    System.out
                            .println("pxLogicalList contain NapDef and equals to napid, add to APN");
                    apnMap = getApnFromSingleMap(logicalMap);
                    data.put(Define.TYPE_APN, (MyHashMap) apnMap);
                }
            }
            return Define.STATE_OK;
        }
        for (HashMap<String, String> napdefMap : napdefList) {
            for (HashMap<String, String> logicalMap : pxLogicalList) {
                // add for 515141 begin
                Set<String> toNapIdSets = new HashSet<String>();
                for (String keyItem : logicalMap.keySet()) {
                    if (keyItem.contains(Define.TO_NAPID)) {
                        toNapIdSets.add(logicalMap.get(keyItem));
                    }
                }
                System.out.println("------------toNapIdSets = " + toNapIdSets.toString());
                // add for bug 515141 begin
                // napdef has internet attr and the tp-napid is internet in
                // proxyLogical
                if (!isInternet && napdefMap.containsKey((Define.INTERNET).toLowerCase())
                        && toNapIdSets.contains(Define.INTERNET)) {
                    isInternet = true;
                    ((MyHashMap) napdefMap).setUserData(true);
                    apnMap = getApnFromMultiMap(logicalMap, napdefMap);
                    data.put(Define.TYPE_APN, (MyHashMap) apnMap);
                }
                // add for bug 515141 end
                else if (napdefMap.containsKey(Define.NAPID)
                        && toNapIdSets.contains(napdefMap.get(Define.NAPID))
                        && (((MyHashMap) napdefMap).getUserData() == null)) {
                    // one pxlogical to one napdef,if more pxlogical to one
                    // napdef,just choose the first pxlogical
                    ((MyHashMap) napdefMap).setUserData(true);
                    // System.out.println("the logical' to-napId is equal NAPID : "
                    // + logicalMap.get("pxlogical_name"));
                    apnMap = getApnFromMultiMap(logicalMap, napdefMap);
                    System.out.println("after getApnMap(), apnMap size = " + apnMap.size());
                    data.put(Define.TYPE_APN, (MyHashMap) apnMap);
                }
                // pxlogical contains napdef and it's to-napid is equals to the
                // napid of internal napdef.
                else {
                    if (logicalMap.containsKey(Define.NAPID)
                            && toNapIdSets.contains(napdefMap.get(Define.NAPID))
                            && (((MyHashMap) logicalMap).getUserData() == null)) {
                        ((MyHashMap) logicalMap).setUserData(true);
                        System.out
                                .println("pxLogicalList contain NapDef and it's to-napid equals to napid, add to APN");
                        apnMap = getApnFromSingleMap(logicalMap);
                        data.put(Define.TYPE_APN, (MyHashMap) apnMap);
                    }
                }
                // add for 515141 end
            }
            if (((MyHashMap) napdefMap).getUserData() == null) {
                System.out.println("pxLogicalList size is 0, just add napdef to APN");
                apnMap = getApnFromSingleMap(napdefMap);
                data.put(Define.TYPE_APN, (MyHashMap) apnMap);
            }
        }
        // add for bug 515141 begin
        isInternet = false;
        // add for bug 515141 end
        // System.out.println(TAG + "end process(), the apnList size = "
        // + data.get(Define.TYPE_APN).size());

        return Define.STATE_OK;
    }

    private HashMap<String, String> getApnFromSingleMap(HashMap<String, String> napMap) {
        System.out.println(TAG + " enter getApnMap(), napMap size = " + napMap.size());
        HashMap<String, String> apnMap = new MyHashMap();
        for (String key : napMap.keySet()) {
            if (!apnMap.containsKey(key)) {
                apnMap.put(key, napMap.get(key));
            } else {
                System.out.println(TAG + " apnMap contain key : " + key);
            }
        }
        // System.out.println(TAG + "enter getApnMap(), apnMap size = " +
        // apnMap.size());
        return apnMap;
    }

    private HashMap<String, String> getApnFromMultiMap(HashMap<String, String> pxMap,
            HashMap<String, String> napMap) {
        // System.out.println(TAG + " enter getApnMap(), pxMap size = " +
        // pxMap.size()
        // + " napMap size = " + napMap.size());
        HashMap<String, String> apnMap = new MyHashMap();
        for (String key2 : napMap.keySet()) {
            if (!apnMap.containsKey(key2)) {
                // System.out.println("\t [" + key2 + "] = [" + napMap.get(key2)
                // + "]");
                apnMap.put(key2, napMap.get(key2));
            } else {
                System.out.println(TAG + " apnMap contain key : " + key2);
            }
        }
        for (String key1 : pxMap.keySet()) {
            if (!apnMap.containsKey(key1)) {
                // System.out.println("\t [" + key1 + "] = [" + pxMap.get(key1)
                // + "]");
                apnMap.put(key1, pxMap.get(key1));
            } else {
                System.out.println(TAG + " apnMap contain key : " + key1);
            }
        }

        // System.out.println(TAG + "enter getApnMap(), apnMap size = " +
        // apnMap.size());
        return apnMap;
    }

    private void apnReference(OtaMapData data) {
        // System.out.println("enter apnReference()");
        for (int nType : OtaMapData.nType) {
            switch (nType) {
                case Define.TYPE_APN:
                case Define.CHAR_PORT:
                case Define.CHAR_NAPDEF:
                case Define.CHAR_ACCESS:
                case Define.CHAR_BOOTSTRAP:
                case Define.CHAR_PXLOGICAL:
                case Define.CHAR_APPLICATION:
                case Define.CHAR_VENDORCONFIG:
                case Define.CHAR_CLIENTIDENTITY:
                    break;

                default: {
                    processApnReference(nType, data);
                }

            }
        }
    }

    private void processApnReference(int nType, OtaMapData data) {
        System.out.println("enter processApnReference() type = " + nType);
        List<MyHashMap> appList = data.get(nType);
        List<MyHashMap> apnList = data.get(Define.TYPE_APN);
        if (appList == null || apnList == null) {
            return;
        }

        for (MyHashMap appMap : appList) {
            processOneApp(appMap, apnList);
        }

    }

    private void processOneApp(MyHashMap app, List<MyHashMap> listApn) {
        System.out.println("enter processOneApp()");
        if (listApn == null) {
            return;
        }
        String to_napid = Define.TO_NAPID;
        String to_proxy = Define.TO_PROXY;
        // ignoreCase when use containsKey() or containsValue()
        app.setIgnoreCase(true);
        if (!app.containsKey(to_napid) && !app.containsKey(to_proxy)) {
            System.out.println("!app.containsKey(to_napid) || !app.containsKey(to_proxy)");
            return;
        }
        String napidApn = "";
        String proxyApn = "";
        if (app.containsKey(to_napid)) {
            napidApn = app.get(to_napid);
        }
        if (app.containsKey(to_proxy)) {
            proxyApn = app.get(to_proxy);
        }

        // System.out.println("proxyApn = [" + proxyApn + "] napidApn = [" +
        // napidApn + "]");
        for (MyHashMap apn : listApn) {
            // System.out.println("APN = " + apn.get(Define.NAPID));
            if (proxyApn != null && apn.get(Define.NAPID).equalsIgnoreCase(proxyApn)) {
                // System.out.println("!proxyApn.isEmpty() && apn.containsValue(proxyApn)");
                app.setUserData(apn);
            }
            if (napidApn != null && apn.get(Define.NAPID).equalsIgnoreCase(napidApn)) {
                // System.out.println("!napidApn.isEmpty() && apn.containsValue(napidApn)");
                app.setUserData(apn);
            }
        }
    }

    private final String TAG = "ApnDataHandler";
}
