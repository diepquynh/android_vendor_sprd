/*
** Copyright 2016. Spreadtrum
*/

static int copy_file(const char* src, const char* dest, arg_chown* arg) {
        FILE *fpSrc, *fpDest;
        fpSrc = fopen(src, "rb");
        if (fpSrc == NULL) {
                ALOGE( "Source file open failure.\n");
                return -1;
        }
        fpDest = fopen(dest, "wb");
        if (fpDest == NULL) {
                ALOGE("Destination file open failure.\n");
                fclose(fpSrc);
                return -1;
        }
        int c;
        while ((c = fgetc(fpSrc)) != EOF) {
               fputc(c, fpDest);
        }
        fclose(fpSrc);
        fclose(fpDest);
        if (arg != NULL) {
                if (chmod(dest, arg->mode) < 0) {
                        ALOGE("cannot chmod file '%s': %s\n", dest, strerror(errno));
                }
                if (chown(dest, arg->uid, arg->gid) < 0) {
                       ALOGE("cannot chown file '%s': %s\n", dest, strerror(errno));
                }
        }
        return 0;
}

static inline int is_dir(const char* path) {
        if (access(path, 0))
               return 0;
        struct stat info;
        if(stat(path, &info))
            return 0;
        return S_ISDIR(info.st_mode);
}

static size_t file_name_predeal(char* new_name, const char* name) {
        strcpy(new_name, name);
        size_t index = strlen(name) - 1;
        if (new_name[index] != '/') {
                new_name[++index] = '/';
        }
        new_name[++index] = '\0';
        return index;
}

static int create_dir(const char* name, arg_chown* arg) {
        if (mkdir(name, 0777) < 0) {
                ALOGE("dest folder: %s can't create, %s\n", name, strerror(errno));
                return -1;
        }
        if (arg != NULL) {
                if (chmod(name, arg->mode) < 0) {
                        ALOGE("cannot chmod file '%s': %s\n", name, strerror(errno));
                }
                if (chown(name, arg->uid, arg->gid) < 0) {
                        ALOGE("can not chown file '%s': %s\n", name, strerror(errno));
                }
        }
        return 0;
}

static int _copy_folder(const char* src, const char* dest, char is_copy_lib,
              arg_chown* arg) {
        if (!is_dir(src)) {
                ALOGE("file: %s is not a folder!\n", src);
                return -2;
        }
        if (!is_dir(dest) && create_dir(dest, arg) < 0) {
                return -1;
        }
        char srcfile_name[PKG_PATH_MAX];
        size_t src_index = file_name_predeal(srcfile_name, src);
        char destfile_name[PKG_PATH_MAX];
        size_t dest_index = file_name_predeal(destfile_name, dest);
        char* tmpp;

        DIR *d = opendir(src);
        if (d == NULL) {
                ALOGE("in backup_app, Unable to opendir %s\n", src);
                return -1;
        }
        struct dirent *de;
        while ((de = readdir(d))) {
                const char *name = de->d_name;
                if (!strcmp(name, ".") || !strcmp(name, "..")) {
                        continue;
                }
                if (!is_copy_lib && !strcmp(name, "lib")) {
                        continue;
                }
                tmpp = srcfile_name + src_index;
                strcpy(tmpp, name);
                tmpp = destfile_name + dest_index;
                strcpy(tmpp, name);
                switch (de->d_type) {
                case DT_DIR:
                        if (access(destfile_name, 0) == 0
                                       || create_dir(destfile_name, arg) >= 0) {
                               _copy_folder(srcfile_name, destfile_name, 1, arg);
                        }
                        break;
               case DT_REG:
                       if (copy_file(srcfile_name, destfile_name, arg) < 0) {
                             ALOGW("copy file from %s to %s failed\n", srcfile_name, destfile_name);
                        }
                        break;
               }
        }
        closedir(d);
        return 0;
}

static int copy_folder(const char* src, const char* dest, char is_copy_lib,
              arg_chown* arg) {
        ALOGD("copy folder from %s to %s\n", src, dest);
        if (arg->uid < 0 || arg->gid < 0) {
              arg = NULL;
        }
        return _copy_folder(src, dest, is_copy_lib, arg);
}

int backup_app(const char* pkgname, const char* dest_path, int uid, int gid) {
        char pkgdir[PKG_PATH_MAX];
        if (create_pkg_path(pkgdir, pkgname, PKG_DIR_POSTFIX, 0)) {
                ALOGE("in backup_app, cannot create package path\n");
                return -1;
        }
        arg_chown arg;
        arg.mode = 0771;
        arg.uid = uid;
        arg.gid = gid;
        return copy_folder(pkgdir, dest_path, 0, &arg);
}

int restore_app(const char* src_path, const char* pkgname, int uid, int gid) {
        char pkgdir[PKG_PATH_MAX];
        if (create_pkg_path(pkgdir, pkgname, PKG_DIR_POSTFIX, 0)) {
                ALOGE("in restore_app, cannot create package path\n");
                return -1;
        }
        arg_chown arg;
        arg.mode = 0771;
        arg.uid = uid;
        arg.gid = gid;
        return copy_folder(src_path, pkgdir, 1, &arg);
}
