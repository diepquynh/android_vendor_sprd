package com.android.sprdlauncher2;

import android.content.ComponentName;
import android.content.Context;

/**
 * SPRD: Feature 259193, porting from Android.4.1 @{
 */
public class SprdShortcutInfo extends ShortcutInfo {

    public SprdShortcutInfo(){
        super();
    }

    public SprdShortcutInfo(Context context, ShortcutInfo info){
        super(context, info);
        setUnReadNumber(info.intent.getComponent());
    }

    public SprdShortcutInfo(AppInfo info){
        super(info);
        setUnReadNumber(info.intent.getComponent());
    }

    public void setUnReadNumber(ComponentName c){
        if(c.equals(UnreadLoader.phoneComponentName)){
             unreadNum = UnreadLoader.missCallCount;
         }else if(c.equals(UnreadLoader.mmsComponentName)){
             unreadNum = UnreadLoader.unReadMmsCount;
         }
    }
}
/** @} */