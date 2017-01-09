package com.sprd.common;

import android.net.Uri;
import com.android.browser.R;

public class BrowserPlugInDrm {

    public BrowserPlugInDrm(){
    }

    public boolean canGetDrmPath(){
        return true;
    }

    public Uri getDrmPath(String url, String mimetype , String filename){
        return null;
    }

    public CharSequence getFilePath(CharSequence path , String str , String filename){
        return path;
    }

    public boolean getMimeType(String mimetype){
        return true;
    }

}
