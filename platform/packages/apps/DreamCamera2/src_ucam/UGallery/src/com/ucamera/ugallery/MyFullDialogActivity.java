/**
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */
package com.ucamera.ugallery;

import android.app.Activity;
import android.os.Bundle;

import com.ucamera.ugallery.preference.MyDialogStub;

public class MyFullDialogActivity  extends Activity {
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        int type = getIntent().getIntExtra(UGalleryConst.DIALOG_TYPE_SETTINGS, 0);
        MyDialogStub stub = MyDialogStub.create(type);
        /* SPRD: CID 108992 : Dereference null return value (NULL_RETURNS) @{ */
        if(stub != null){
            this.setContentView(stub.getContentView());
            stub.bind(this);
        } else {
            finish();
        }
        // this.setContentView(stub.getContentView());
        // stub.bind(this);
        /* @} */
    }

    public void closeDialog() {
        finish();
    }
}
