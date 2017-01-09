
package com.sprd.process.list2hashmap;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;
import android.os.Debug;
import com.sprd.xml.parser.prv.Define;
import com.sprd.xml.parser.prv.PrintLogHelper;

public class OtaMapData extends HashMap<Integer, ArrayList<MyHashMap>> {
    public OtaMapData() {
        super();
        initEnv();
    }

    public ArrayList<MyHashMap> put(Integer nType, MyHashMap hashMap) {
        if (!checkType(nType)) {
            return null;
        }

        ArrayList<MyHashMap> list = get(nType);
        if (list == null) {
            list = new ArrayList<MyHashMap>();
            list.add((MyHashMap) hashMap);
            return super.put(nType, list);
        } else {
            get(nType).add((MyHashMap) hashMap);
            return get(nType);
        }

    }

    public ArrayList<MyHashMap> put(Integer nType, ArrayList<MyHashMap> hashMap) {
        if (checkType(nType)) {
            return super.put(nType, hashMap);
        } else {
            return null;
        }
    }

    private boolean checkType(int nType) {
        switch (nType) {
            case Define.TYPE_APN:
            case Define.TYPE_MMS:
            case Define.TYPE_EMAIL:
            case Define.TYPE_BROWSE:
            case Define.CHAR_PORT:
            case Define.CHAR_NAPDEF:
            case Define.CHAR_ACCESS:
            case Define.TYPE_BOOKMARK:
            case Define.TYPE_EXCHANGE:
            case Define.TYPE_STARTPAGE:
            case Define.CHAR_PXLOGICAL:
            case Define.CHAR_BOOTSTRAP:
            case Define.CHAR_APPLICATION:
            case Define.CHAR_VENDORCONFIG:
            case Define.CHAR_CLIENTIDENTITY:
                return true;
            default:
                System.out.println("checkType error : " + nType);
                return false;
        }
    }

    public void initEnv() {
        for (int nItemType : nType) {

            put(nItemType, new ArrayList<MyHashMap>());
        }
    }

    public void Debug() {
        Set<Integer> set = keySet();
        for (int nIndex : set) {
            System.out.print("\r\n <<< =========   type =[" + nIndex + "]   ========= >>>");
            PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                    "\r\n <<< =========   type =[" + nIndex + "]   ========= >>>");
            DebugArrayList(get(nIndex));
        }
    }

    private void DebugArrayList(List<MyHashMap> list) {
        if (null == list) {
            return;
        }
        for (HashMap<String, String> hashMap : list) {
            System.out.println("\r\n===>>> Start HashMap\r\n");
            PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                    "\r\n===>>> Start HashMap\r\n");
            DebugHashMap((MyHashMap) hashMap);
            System.out.println("\r\n ===>>> End HashMap\r\n");
            PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                    "\r\n ===>>> End HashMap\r\n");

        }
    }

    public void DebugHashMap(HashMap<String, String> hashMap) {
        if (hashMap == null) {
            return;
        }
        Set<String> set = hashMap.keySet();
        for (String szItem : set) {
            System.out.print("\t [" + szItem + "] = [" + hashMap.get(szItem) + "]");
            PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                    "\t [" + szItem + "] = [" + hashMap.get(szItem) + "]");
        }

        DebugMapUserData(((MyHashMap) hashMap).getUserData());
    }

    private void DebugMapUserData(Object userData) {
        if (userData == null) {
            return;
        }
        System.out.println("\n\t ===>>> begin UserData\r\n");
        PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                "\n\t ===>>> begin UserData\r\n");

        if (!(userData instanceof HashMap)) {
            System.out.println("userData is not instanceof HashMap");
            PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                    "userData is not instanceof HashMap");
            return;
        }
        Set<String> set = ((HashMap) userData).keySet();
        for (String szItem : set) {
            System.out.print("\t[" + szItem + "] = [" + ((HashMap) userData).get(szItem) + "] ");
            PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                    "\t[" + szItem + "] = [" + ((HashMap) userData).get(szItem) + "] ");
        }
        System.out.println("\n\t ===>>> End UserData\r\n");
        PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                "\n\t ===>>> End UserData\r\n");

    }

    public static final int nType[] = new int[] {
            Define.CHAR_NAPDEF, Define.CHAR_PORT, Define.CHAR_ACCESS, Define.CHAR_PXLOGICAL,
            Define.CHAR_BOOTSTRAP, Define.CHAR_APPLICATION, Define.CHAR_CLIENTIDENTITY,
            Define.CHAR_VENDORCONFIG, Define.TYPE_APN, Define.TYPE_EMAIL, Define.TYPE_BOOKMARK,
            Define.TYPE_STARTPAGE, Define.TYPE_BROWSE, Define.TYPE_MMS, Define.TYPE_EXCHANGE
    };

}
