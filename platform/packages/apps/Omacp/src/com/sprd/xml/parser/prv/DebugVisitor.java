
package com.sprd.xml.parser.prv;

import java.io.PrintWriter;

import android.util.Log;

import com.sprd.xml.parser.impl.BaseVisitorImpl;
import com.sprd.xml.parser.impl.SAXAttribute;
import com.sprd.xml.parser.itf.INode;
import com.sprd.xml.parser.itf.IVisitor;

public class DebugVisitor extends BaseVisitorImpl {

    private static final int TP_ADD = 0;
    private static final int TP_DEC = 1;
    private static final int TP_DEFAULT = 2;
    public static final String TAG = "DebugVisitor";

    public static class VisitorParam {
        public VisitorParam() {
        }

        public int inc() {
            return (++mCount);
        }

        public int dec() {
            return (--mCount);
        }

        public int getValue() {
            return mCount;
        }

        private int mCount = 0;
    }

    private boolean mbPrintRT = false;

    private void setPrintRT(boolean bRT) {
        mbPrintRT = bRT;
    }

    private boolean isPrintRT() {
        return mbPrintRT;
    }

    private int processParam(Object obj, int nType) {

        if (obj == null) {
            return -1;
        } else if (!(obj instanceof VisitorParam)) {
            return -1;
        }
        switch (nType) {
            case TP_ADD:
                ((VisitorParam) obj).inc();
                break;

            case TP_DEC:
                ((VisitorParam) obj).dec();
                break;

            default:
                break;
        }
        return ((VisitorParam) obj).getValue();
    }

    private void printSpace(Object obj) {
        VisitorParam ins = null;
        if (obj instanceof VisitorParam) {
            ins = (VisitorParam) obj;
        } else {
            return;
        }
        for (int nIndex = 0; nIndex < ins.getValue(); ++nIndex) {
            System.out.print("\t");
            PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).print("\t");
        }
    }

    @Override
    public int startTagHandle(INode node, Object obj) {
        // Log.d(TAG,"startTag nodeName : " + node.getName());
        processParam(obj, TP_ADD);
        if (!isPrintRT()) {
            System.out.println("");
            PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println("\t");
            setPrintRT(true);
        }
        return 0;
    }

    @Override
    public int visitor(INode node, Object obj) {
        // Log.d(TAG,"enter visitor nodeName : " + node.getName());
        if (node.getName() != null || node.getName() == "") {
            printSpace(obj);
            System.out.print("<" + node.getName() + " ");
            PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).print(
                    "<" + node.getName() + " ");
            SAXAttribute attr = node.getAttribute();
            if (attr != null)
                attr.Debug();
            System.out.print(">");
            PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).print(">");
            setPrintRT(false);
            System.out.print(node.getValue().trim());
            PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).print(
                    node.getValue().trim());
        } else {
            System.out.println("the tag is empty");
        }
        // VisitorParam ins = (VisitorParam) obj;
        // System.out.println(" return test ["+ins.GetValue()+"]");
        return IVisitor.VISITOR_OK;
    }

    @Override
    public int endTagHandle(INode node, Object obj) {
        // Log.d(TAG,"endTag nodeName : " + node.getName());
        if (isPrintRT()) {
            printSpace(obj);
        }
        if (node.getName() != null || node.getName() == "") {
            System.out.print("</" + node.getName() + ">");
            PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).print(
                    "</" + node.getName() + ">");
            setPrintRT(true);
            System.out.println();
            PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println();
            // Log.d("jordan_parser", "</" + node.getName() + ">"); // not log
        } else {
            System.out.println("the tag is empty");
        }
        processParam(obj, TP_DEC);
        return 0;
    }

}
