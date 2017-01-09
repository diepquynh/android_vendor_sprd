/*
** Copyright 2016,Spreadtrum
*/

dir_rec_t android_app_preload_dir;
dir_rec_t android_app_vital_dir;
bool init_globals_from_data_and_root(const char* data, const char* root) {
    // Get the android data directory.
    if (get_path_from_string(&android_data_dir, data) < 0) {
        return false;
    }

    // Get the android app directory.
    if (copy_and_append(&android_app_dir, &android_data_dir, APP_SUBDIR) < 0) {
        return false;
    }

    // Get the android protected app directory.
    if (copy_and_append(&android_app_private_dir, &android_data_dir, PRIVATE_APP_SUBDIR) < 0) {
        return false;
    }

    // Get the android ephemeral app directory.
    if (copy_and_append(&android_app_ephemeral_dir, &android_data_dir, EPHEMERAL_APP_SUBDIR) < 0) {
        return -1;
    }

    // Get the android app native library directory.
    if (copy_and_append(&android_app_lib_dir, &android_data_dir, APP_LIB_SUBDIR) < 0) {
        return false;
    }

    // Get the sd-card ASEC mount point.
    if (get_path_from_env(&android_asec_dir, "ASEC_MOUNTPOINT") < 0) {
        return false;
    }

    // Get the android media directory.
    if (copy_and_append(&android_media_dir, &android_data_dir, MEDIA_SUBDIR) < 0) {
        return false;
    }

    // Get the android external app directory.
    if (get_path_from_string(&android_mnt_expand_dir, "/mnt/expand/") < 0) {
        return false;
    }

    // Get the android profiles directory.
    if (copy_and_append(&android_profiles_dir, &android_data_dir, PROFILES_SUBDIR) < 0) {
        return false;
    }

    // Take note of the system and vendor directories.
    android_system_dirs.count = 6;

    android_system_dirs.dirs = (dir_rec_t*) calloc(android_system_dirs.count, sizeof(dir_rec_t));
    if (android_system_dirs.dirs == NULL) {
        ALOGE("Couldn't allocate array for dirs; aborting\n");
        return false;
    }

    dir_rec_t android_root_dir;
    if (get_path_from_string(&android_root_dir, root) < 0) {
        return false;
    }

    android_system_dirs.dirs[0].path = build_string2(android_root_dir.path, APP_SUBDIR);
    android_system_dirs.dirs[0].len = strlen(android_system_dirs.dirs[0].path);

    android_system_dirs.dirs[1].path = build_string2(android_root_dir.path, PRIV_APP_SUBDIR);
    android_system_dirs.dirs[1].len = strlen(android_system_dirs.dirs[1].path);

    android_system_dirs.dirs[2].path = strdup("/vendor/app/");
    android_system_dirs.dirs[2].len = strlen(android_system_dirs.dirs[2].path);

    android_system_dirs.dirs[3].path = strdup("/oem/app/");
    android_system_dirs.dirs[3].len = strlen(android_system_dirs.dirs[3].path);

    /* SPRD: add feature for scan the preload and vital directory @{ */
    android_app_preload_dir.path = strdup("/system/preloadapp/");
    android_app_preload_dir.len = strlen(android_app_preload_dir.path);
    android_app_vital_dir.path = strdup("/system/vital-app/");
    android_app_vital_dir.len = strlen(android_app_vital_dir.path);
    android_system_dirs.dirs[4].path = android_app_preload_dir.path;
    android_system_dirs.dirs[4].len = android_app_preload_dir.len;
    android_system_dirs.dirs[5].path = android_app_vital_dir.path;
    android_system_dirs.dirs[5].len = android_app_vital_dir.len;
    /* @} */

    return true;
}
