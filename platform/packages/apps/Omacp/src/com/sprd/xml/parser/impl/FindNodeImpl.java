
package com.sprd.xml.parser.impl;

import java.util.List;

import com.sprd.process.dom2list.DefaultVisitor;
import com.sprd.process.dom2list.ListFindParam;
import com.sprd.xml.parser.itf.IFindNode;
import com.sprd.xml.parser.itf.INode;

public class FindNodeImpl implements IFindNode {

    private static final String TAG = "FindNodeImpl";

    public List<INode> getNodeByFindParam(INode node, ListFindParam param, Object obj) {
        System.out.println("This is default implementation of GetNodeByPath");
        ListFindParam findParam = param;
        DefaultVisitor dv = new DefaultVisitor();
        int ret = dv.visitor(node, findParam);
        System.out.println("ret = " + Integer.toHexString(ret));
        // return dv.getNodeList();
        return null;
    }
}
