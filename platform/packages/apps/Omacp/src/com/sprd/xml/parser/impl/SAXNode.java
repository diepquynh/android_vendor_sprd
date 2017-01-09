
package com.sprd.xml.parser.impl;

import java.util.LinkedList;
import java.util.List;

import com.sprd.process.dom2list.ListFindParam;
import com.sprd.xml.parser.itf.IFindNode;
import com.sprd.xml.parser.itf.INode;
import com.sprd.xml.parser.itf.IVisitor;

public class SAXNode implements INode {

    /**
     * @param tag
     * @param value
     * @param node
     */
    public SAXNode(String tag, String value, INode node) {
        this();
        mNodeName = tag;
        mNodeValue = value;
        mParent = node;
    }

    public SAXNode() {
        mNodeName = "";
        mNodeValue = "";
        mNodeAttr = null;
        mParent = null;
        mLlistchild = null;
        isEmpty = false;
        mFindImpl = new FindNodeImpl();
    }

    public String getName() {
        return mNodeName;
    }

    public void setName(String mNodeName) {
        this.mNodeName = mNodeName;
    }

    public void setValue(String value) {
        mNodeValue = value;
    }

    public void addValue(String value) {
        if (mNodeValue == null || mNodeValue == "") {
            mNodeValue = value;
        } else {
            mNodeValue += value;
        }
    }

    public String getValue() {
        return mNodeValue;
    }

    public void setChild(List<INode> list) {
        mLlistchild = list;
    }

    public List<INode> getChild() {
        if (mLlistchild == null)
            mLlistchild = new LinkedList<INode>();
        return mLlistchild;
    }

    public void Debug() {
        System.out.println("\n[" + getName().trim() + "] = [" + getValue().trim() + "]");
        SAXAttribute attr = getAttribute();
        if (attr != null)
            attr.Debug();
        for (INode obj : getChild()) {
            obj.Debug();
        }
    }

    public void createAttribute() {
        mNodeAttr = new SAXAttribute();
    }

    public void setAttribute(SAXAttribute attr) {
        mNodeAttr = attr;
    }

    public SAXAttribute getAttribute() {
        return mNodeAttr;
    }

    public void setParent(INode parent) {
        mParent = parent;
    }

    public INode getParent() {
        return mParent;
    }

    public List<INode> getNodeByFindParam(ListFindParam param) throws Exception {
        return getNodeByFindParam(this, param);
    }

    protected List<INode> getNodeByFindParam(INode node, ListFindParam param) throws Exception {
        if (getFindNodeImpl() != null)
            return getFindNodeImpl().getNodeByFindParam(node, param, null);
        else
            throw new Exception("Find Interface Do not Implements");
    }

    /*
     * eg:<?xml version="1.0" encoding="UTF-8" ?> is a special node, so the real
     * root node is it's first child node.
     */
    public INode getRoot() {
        INode temp = this;
        while (temp.getParent() != null)
            temp = temp.getParent();
        if (temp.getChild().size() <= 0)
            return null;
        else
            return temp.getChild().get(0);
    }

    public int visitor(INode node, Object obj, IVisitor impl) {
        if (impl == null || node == null) {
            System.out.println("node or impl of Ivisitor is null");
            return IVisitor.VISITOR_ERR_PARAM;
        }

        impl.startTagHandle(node, obj);

        int nRet = impl.visitor(node, obj);
        // System.out.println("Visitor Result =[0X"+Integer.toHexString(nRet)+"]");
        switch (nRet) {
            case IVisitor.VISITOR_OK:
            case IVisitor.VISITOR_CONTINUE:
                if (node.getChild() == null) {
                    System.out.println("node.getChild(), will return IVisitor.VISITOR_CONTINUE");
                    return IVisitor.VISITOR_CONTINUE;
                }
                for (INode chnode : node.getChild()) {
                    nRet = visitor(chnode, obj, impl);
                    if (nRet != IVisitor.VISITOR_OK && nRet != IVisitor.VISITOR_CONTINUE) {
                        // System.out.println("nRet = " + nRet);
                        return nRet;
                    }
                }
                impl.endTagHandle(node, obj);
                return IVisitor.VISITOR_CONTINUE;

            case IVisitor.VISITOR_FATHER:
                // System.out.println("IVisitor.VISITOR_FATHER nRet=" + nRet);
                break;

            case IVisitor.VISITOR_BROTHER:
                // System.out.println("IVisitor.VISITOR_BROTHER nRet=" + nRet);
                int size = node.getParent().getChild().size();
                int index = node.getParent().getChild().indexOf(node);
                // System.out.println("size = " + size + ", index = " + index);
                if (index + 1 < size) {
                    INode brotherNode = node.getParent().getChild().get(index + 1);
                    nRet = visitor(brotherNode, obj, impl);
                    // System.out.println("nRet = " + nRet);
                }

                // System.out.println("query brother node complete");
                break;

            case IVisitor.VISITOR_ERR_PARAM:
                // System.out.println("IVisitor.VISITOR_ERR_PARAM nRet=" +
                // nRet);
                break;

            case IVisitor.VISITOR_FAILURE:
                // System.out.println("IVisitor.VISITOR_FAILURE nRet=" + nRet);
                break;

            case IVisitor.VISITOR_NOT_CMP:
                // System.out.println("IVisitor.VISITOR_NOT_CMP nRet=" + nRet);
                break;

            default:
                break;
        }
        // System.out.println("IVisitor.VISITOR_CMP nRet=" + nRet);
        return IVisitor.VISITOR_CMP;
    }

    public void setFindNodeImpl(IFindNode impl) {
        mFindImpl = impl;
    }

    private IFindNode getFindNodeImpl() {
        return mFindImpl;
    }

    public boolean getEmptyFlag() {
        return isEmpty;
    }

    public void setEmptyFlag(boolean flag) {
        isEmpty = flag;
    }

    private INode mParent;
    private boolean isEmpty;
    private String mNodeName;
    private String mNodeValue;
    private SAXAttribute mNodeAttr;
    private List<INode> mLlistchild;
    private static IFindNode mFindImpl;
    private static final String TAG = "SAXNode";

}
