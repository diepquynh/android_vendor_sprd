
package com.sprd.process.list2hashmap;

public abstract class BaseHandlerImpl implements IProcess {

    public final static int STATE_OK = 1;
    public final static int STATE_ERROR = -1;
    public final static int STATE_PARAM_ERROR = 0;
    public final String TAG = "BaseHandler";

}
