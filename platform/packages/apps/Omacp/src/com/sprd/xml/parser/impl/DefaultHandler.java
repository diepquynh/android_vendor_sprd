
package com.sprd.xml.parser.impl;

import java.util.List;
import android.util.Log;
import com.sprd.xml.parser.itf.INode;
import com.sprd.omacp.parser.WbxmlParser;

import org.xmlpull.v1.XmlPullParserException;

public class DefaultHandler {

    public DefaultHandler(INode node) {

        setRoot(node);
        setCurrentNode(node);
    }

    public DefaultHandler() {
        mNodeIns = new SAXNode();
        setRoot(mNodeIns);
        // INode node = new SAXNode();
        // setRoot(node);
        setCurrentNode(mNodeIns);
    }

    public static INode getNodeIns() {
        // if (mNodeIns == null) {
        // System.out.println("create new nodeIns");
        // mNodeIns = new SAXNode();
        // }
        return mNodeIns;
    }

    public static DefaultHandler getInstance() {
        if (mInstance == null) {
            synchronized (DefaultHandler.class) { // double check
                if (mInstance == null) {
                    mInstance = new DefaultHandler();
                }
            }
        }
        return mInstance;
    }

    synchronized public static void releaseIns() {
        if (mInstance != null) {
            mInstance = null;
        }
    }

    public void startDocument() {
        System.out.println("<<<=== startDocument ===>>>");
    }

    public void endDocument() {
        System.out.println("<<<=== endDocument ===>>>");
        if (getRoot().getChild().size() == 1) {
            setRoot(getRoot().getChild().get(0));
        }
    }

    public void startTag(WbxmlParser parser) throws XmlPullParserException {
        String tagName = parser.getName();
        System.out.println("CurrentNode nodeTAG = " + tagName);
        INode subNode = (INode) new SAXNode(tagName, "", getCurrentNode());
        getCurrentNode().getChild().add(subNode);
        setCurrentNode(subNode);
        /*
         * if (parser.isEmptyElementTag()) {
         * System.out.println("this is empty tag : " + parser.getName());
         * getCurrentNode().setEmptyFlag(true); }
         */
        int attrCount = parser.getAttributeCount();
        if (attrCount > 0) {
            getCurrentNode().setAttribute(new SAXAttribute());
            for (int i = 0; i < attrCount; ++i) {
                getCurrentNode().getAttribute().put(parser.getAttributeName(i),
                        parser.getAttributeValue(i));
                // System.out.println("CurrentNode AttrName:" +
                // parser.getAttributeName(i));
                // System.out.println("CurrentNode AttrValue:" +
                // parser.getAttributeValue(i));
            }
        }
    }

    public void clearData() {
        Log.d(TAG, "clear data");
        releaseIns();
        mNodeIns = null;
    }

    public void endTag() {
        System.out.println("enter endTag()");
        setCurrentNode(getCurrentNode().getParent());
    }

    public void startText(String text) {
        System.out.println("text = " + text);
        // getCurrentNode().setValue(text);
        getCurrentNode().addValue(text); // ensure the entire text
        System.out.println("CurrentNode value:" + getCurrentNode().getValue());
    }

    public void characters(char[] ch, int start, int length) {
        mCount++;
        getCurrentNode().addValue(new String(ch, start, length));
        // System.out.println("CurrentNode Value: "+getCurrentNode().getValue());
        // getCurrentNode().Debug();
    }

    public INode getRoot() {
        return mRoot;
    }

    public void setRoot(INode node) {
        mRoot = node;
    }

    private INode getCurrentNode() {
        return mCurNode;
    }

    private void setCurrentNode(INode curnode) {
        mCurNode = curnode;
    }

    private INode lastNodeOfList(List<INode> list) {
        if (list.isEmpty()) {
            return null;
        }
        int size = list.size();
        return list.get(size - 1);
    }

    private int mCount = 0;
    private INode mCurNode = null;
    private INode mRoot = null;
    private static INode mNodeIns = null;
    private static DefaultHandler mInstance = null;
    private static final String TAG = "DefaultHandler";
}
