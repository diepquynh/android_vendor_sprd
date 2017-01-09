
package com.sprd.xml.parser.itf;

import java.util.List;

import com.sprd.process.dom2list.ListFindParam;

public interface IFindNode {
    public List<INode> getNodeByFindParam(INode node, ListFindParam param, Object obj);
}
