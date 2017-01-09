package android.content.pm;

/**
 * this file is for AppClone.
 * extends UserInfo,and we will add new function in this file.
 *
 * @hide
 */


public class AppCloneUserInfo extends UserInfo {

    /**
     * Indicates that this user is create by SPRD to create a profile for APP_CLONE.
     */
    public static final int FLAG_APP_CLONE = 0x00000080;

    public AppCloneUserInfo(UserInfo orig){
        super(orig);
    }

    public boolean isAppClone() {
        return (flags & FLAG_APP_CLONE) == FLAG_APP_CLONE;
    }

}
