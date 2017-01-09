/*
 * Created by Spreadst
 */

// SPRD: support double sdcard change not kill system_server
int Process::killProcessesWithOpenFiles(const char *path, int signal) {
    int count = 0;
    DIR* dir;
    struct dirent* de;

    if (!(dir = opendir("/proc"))) {
        SLOGE("opendir failed (%s)", strerror(errno));
        return count;
    }

    while ((de = readdir(dir))) {
        int pid = getPid(de->d_name);
        char name[PATH_MAX];

        if (pid == -1)
            continue;
        getProcessName(pid, name, sizeof(name));

        char openfile[PATH_MAX];

        if (checkFileDescriptorSymLinks(pid, path, openfile, sizeof(openfile))) {
            SLOGE("Process %s (%d) has open file %s", name, pid, openfile);
        } else if (checkFileMaps(pid, path, openfile, sizeof(openfile))) {
            SLOGE("Process %s (%d) has open filemap for %s", name, pid, openfile);
        } else if (checkSymLink(pid, path, "cwd")) {
            SLOGE("Process %s (%d) has cwd within %s", name, pid, path);
        } else if (checkSymLink(pid, path, "root")) {
            SLOGE("Process %s (%d) has chroot within %s", name, pid, path);
        } else if (checkSymLink(pid, path, "exe")) {
            SLOGE("Process %s (%d) has executable path within %s", name, pid, path);
        } else {
            continue;
        }
        /* SPRD: support double sdcard kill slog process force when unmount sd card for performance @{ */
        if(!strcmp(name, "/system/bin/slog") || !strcmp(name, "/system/bin/slogmodem")){
            SLOGE("Sending SIGKILL to %s process %d", name , pid);
            kill(pid, SIGKILL);
            count++;
            continue;
        }
        /* @} */
        if (signal != 0) {
          /* SPRD: support double sdcard do not kill system_server process with signal SIGTERM @{ */
          if (!strcmp(name, "system_server")) {
              SLOGW("Process name is system_server, do not kill \n");
              count++;
              continue;
          }else {
            SLOGW("Sending %s to process %d", strsignal(signal), pid);
            kill(pid, signal);
            count++;
          /* SPRD: support double sdcard do not kill system_server process with signal SIGTERM @{ */
         }
           /* @} */
        }
    }
    closedir(dir);
    return count;
}

/* SPRD: Add for boot performance in cryptfs mode {@  */
extern "C" void vold_getProcessName(int pid, char *buffer, size_t max) {
   Process::getProcessName(pid, buffer, max);
}
/* @} */

/* SPRD: Add for boot performance in cryptfs mode {@ */
extern "C" int vold_checkFileDescriptorSymLinks(int pid, const char *mountPoint, char *openFilename, size_t max) {
    return Process::checkFileDescriptorSymLinks(pid, mountPoint, openFilename, max);
}

extern "C" int vold_checkSymLink(int pid, const char *mountPoint, const char *name) {
    return Process::checkSymLink(pid, mountPoint, name);
}

/* @} */

/* SPRD: Add for boot performance in cryptfs mode {@ */
extern "C" int vold_getPid(const char *s) {
    return Process::getPid(s);
}
/* @} */


