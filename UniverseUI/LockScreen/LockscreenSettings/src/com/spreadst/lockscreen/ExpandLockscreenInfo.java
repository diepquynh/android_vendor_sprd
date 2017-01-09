/** Create by Spreadst */

package com.spreadst.lockscreen;

public class ExpandLockscreenInfo {

    private int id;

    private String name_string_id;

    private String class_name;

    private String apk_name;

    private String package_name;

    private String preview_id;

    public String getName_string_id() {
        return name_string_id;
    }

    public void setName_string_id(String name_string_id) {
        this.name_string_id = name_string_id;
    }

    public String getClass_name() {
        return class_name;
    }

    public int getId() {
        return id;
    }

    public void setId(int id) {
        this.id = id;
    }

    public String getApk_name() {
        return apk_name;
    }

    public void setApk_name(String apk_name) {
        this.apk_name = apk_name;
    }

    public String getPackage_name() {
        return package_name;
    }

    public void setPackage_name(String package_name) {
        this.package_name = package_name;
    }

    public String getPreview_id() {
        return preview_id;
    }

    public void setPreview_id(String preview_id) {
        this.preview_id = preview_id;
    }

    public void setClass_name(String class_name) {
        this.class_name = class_name;
    }

    @Override
    public String toString() {
        return "id=" + id + ", class_name=" + class_name + ", apk_name="
                + apk_name + ", package_name=" + package_name + ", preview_id="
                + preview_id;
    }

}
