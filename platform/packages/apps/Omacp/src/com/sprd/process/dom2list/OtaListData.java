
package com.sprd.process.dom2list;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import com.sprd.xml.parser.itf.INode;
import com.sprd.xml.parser.prv.Define;

public class OtaListData extends HashMap<Integer, List<INode>> {

    public OtaListData() {
        super();
        initEnv();
    }

    public List<INode> put(Integer nType, List<INode> list) {
        if (checkType(nType)) {
            return super.put(nType, list);
        } else {
            return null;
        }
    }

    private boolean checkType(int nType) {
        switch (nType) {
            case Define.CHAR_PORT:
            case Define.CHAR_NAPDEF:
            case Define.CHAR_ACCESS:
            case Define.CHAR_PXLOGICAL:
            case Define.CHAR_BOOTSTRAP:
            case Define.CHAR_APPLICATION:
            case Define.CHAR_CLIENTIDENTITY:
            case Define.CHAR_VENDORCONFIG:
                return true;
            default:
                System.out.println("checkType error : " + nType);
                return false;
        }
    }

    public void initEnv() {
        // main nodes type
        int nType[] = new int[] {
                Define.CHAR_NAPDEF, Define.CHAR_PORT, Define.CHAR_ACCESS, Define.CHAR_PXLOGICAL,
                Define.CHAR_BOOTSTRAP, Define.CHAR_APPLICATION, Define.CHAR_CLIENTIDENTITY,
                Define.CHAR_VENDORCONFIG
        };
        for (int nItemType : nType) {
            put(nItemType, new ArrayList<INode>());
        }
    }

}
