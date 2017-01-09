package com.android.util;


import com.google.android.mms.pdu.PduPart;

public interface ResizeImageResultCallback {
	void onResizeResult(PduPart part, boolean append);
}
