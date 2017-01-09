
package com.sprd.xml.parser.itf;

public interface IVisitor {

    public int startTagHandle(INode node, Object obj);

    public int visitor(INode node, Object obj);

    public int endTagHandle(INode node, Object obj);

    public int getErrorCode(int nReture);

    public boolean isSuccessfully(int nValue);

    public boolean isFailure(int nValue);

    public String getErrorDesc(int nCode);

    public static final int VISITOR_MASK        = 0x00FFFFFF;
    public static final int VISITOR_OK          = 0x00000000;
    public static final int VISITOR_FAILURE_BASE= 0xFF000000;
    public static final int VISITOR_SKIP        = (VISITOR_OK | 0x00000001);
    public static final int VISITOR_CONTINUE    = (VISITOR_OK | 0x00000002);
    public static final int VISITOR_BROTHER     = (VISITOR_OK | 0x00000004);
    public static final int VISITOR_FATHER      = (VISITOR_OK | 0x00000008);
    public static final int VISITOR_NOT_CMP     = (VISITOR_OK | 0x00000010);
    public static final int VISITOR_CMP         = (VISITOR_OK | 0x00000011);
    
    public static final int VISITOR_ERR_PARAM  = (VISITOR_FAILURE_BASE | 0x00000001);
    public static final int VISITOR_FAILURE    = (VISITOR_FAILURE_BASE | 0x00FFFFFF);
}
