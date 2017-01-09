
package com.sprd.xml.parser.itf;

import java.util.List;

import com.sprd.process.dom2list.ListFindParam;
import com.sprd.xml.parser.impl.SAXAttribute;

public interface INode {

    public String getName();

    public void setName(String szValue);

    public void addValue(String szValue);

    public void setValue(String szValue);

    public String getValue();

    public void setChild(List<INode> list);

    public List<INode> getChild();

    public void Debug();

    public void setAttribute(SAXAttribute attr);

    public SAXAttribute getAttribute();

    public void setParent(INode parent);

    public INode getParent();

    public List<INode> getNodeByFindParam(ListFindParam param) throws Exception;

    public INode getRoot();

    public int visitor(INode node, Object obj, IVisitor impl);

    public void setFindNodeImpl(IFindNode impl);

    public boolean getEmptyFlag();

    public void setEmptyFlag(boolean flag);

}
