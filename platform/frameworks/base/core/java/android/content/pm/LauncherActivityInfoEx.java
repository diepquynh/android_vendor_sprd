package android.content.pm;

import java.util.ArrayList;
import java.util.List;

import android.content.pm.PackageManager;
import android.content.pm.UserInfo;
import android.content.pm.AppCloneUserInfo;
import android.content.Context;
import android.os.UserHandle;
import android.os.UserManager;
import android.content.res.Resources;

/**
 *
 * add for app-clone
 * @hide
 * */

public class LauncherActivityInfoEx extends LauncherActivityInfo{

    private ResolveInfo mResolveInfo;
    private UserManager mUserManager;
    private final PackageManager mPm;
    private Context mContext;

    LauncherActivityInfoEx(Context context, ResolveInfo info,
        UserHandle user) {
        super(context, info.activityInfo, user);
        mContext = context;
        mResolveInfo = info;
        mPm = context.getPackageManager();
    }

    //this is like super.getLabel , but we will distinguish the userInfo.flag
    public CharSequence getLabel() {
        UserInfo userInfo = getUserIfProfile(getUser().getIdentifier());
        if(userInfo != null){
            AppCloneUserInfo appCloneUserInfo = new AppCloneUserInfo(userInfo);
            if(appCloneUserInfo.isAppClone()){
                return mResolveInfo.loadLabel(mPm)+ appCloneUserInfo.name;
            }
        }
        return mResolveInfo.loadLabel(mPm);
    }

    UserManager getUserManager() {
        if (mUserManager == null) {
            mUserManager = UserManager.get(mContext);
        }
        return mUserManager;
    }

    public UserInfo getUserIfProfile(int userHandle) {
        List<UserInfo> userProfiles = getUserManager().getProfiles(UserHandle.myUserId());
        for (UserInfo user : userProfiles) {
            if (user.id == userHandle) {
                return user;
            }
        }
        return null;
    }
}
