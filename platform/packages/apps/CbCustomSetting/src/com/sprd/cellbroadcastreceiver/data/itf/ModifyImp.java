package com.sprd.cellbroadcastreceiver.data.itf;

import android.util.Log;

public class ModifyImp implements IModify {

	@Override
	public int getFlag() {
		return mnFlag;
	}

	@Override
	public void setFlag(int nFlag) {
		switch (nFlag) {
		case OP_UNKNOW:
		case OP_NORMAL:
		case OP_INSERT:
		case OP_DELETE:
		case OP_UPDATE:
			mnFlag = nFlag;
			break;
		default:
			Log.e("IModifyImp", "", new RuntimeException("SetFlag Error"));
		}
	}

	private int mnFlag = OP_NORMAL;
}
