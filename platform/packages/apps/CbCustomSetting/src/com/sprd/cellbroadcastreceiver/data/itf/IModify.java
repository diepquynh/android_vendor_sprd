package com.sprd.cellbroadcastreceiver.data.itf;

public interface IModify {
	   public final static int OP_UNKNOW 	=0X00000000;
	   public final static int OP_NORMAL 	=0X00000001;
	   public final static int OP_INSERT 	=0X00000002;
	   public final static int OP_UPDATE 	=0X00000004;
	   public final static int OP_DELETE 	=0X00000008;
	   
	   public int getFlag();
	   public void setFlag(int nFlag);
}
