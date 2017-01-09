/*
 * Created by Spreadst
 */

/*  write string to file  */
status_t WriteToFile(const std::string& preMsg, const std::string& file, const std::string& str, const char byte) {
    int fd;

    if ((fd = open(file.c_str(), O_WRONLY)) < 0) {
        PLOG(ERROR) << preMsg << StringPrintf(" unable to open file (%s)", file.c_str());
        return -EIO;
    }

    int writeRes = OK;
    if (str.empty()) {
        LOG(VERBOSE) << StringPrintf(" write byte num %d to file \'%s\'", byte, file.c_str());
        writeRes = write(fd, &byte, 1);
    } else {
        LOG(VERBOSE) << StringPrintf(" write string \'%s\' to file \'%s\'", str.c_str(), file.c_str());
        writeRes = write(fd, str.c_str(), str.length());
    }
    if (writeRes < 0) {
        PLOG(ERROR) << preMsg << StringPrintf(" unable to write file (%s)", file.c_str());
        close(fd);
        return -EIO;
    }

    close(fd);
    return OK;
}
/* @} */

status_t ForceUnmount(const std::string& path) {
    const char* cpath = path.c_str();
    if (!umount2(cpath, UMOUNT_NOFOLLOW) || errno == EINVAL || errno == ENOENT) {
        return OK;
    }
    /* SPRD: replace storage path to /storage @{ */
    LOG(WARNING) << "Failed to unmount " << path;

    std::string storage_path = path;
    std::string path_prefix = "/mnt/runtime/default";
    std::size_t pos = storage_path.find(path_prefix);
    LOG(WARNING) << "storage_path1= " + storage_path << ", pos=" << pos;
    if (pos != std::string::npos) {
        storage_path = storage_path.replace(pos, path_prefix.length(), "/storage");
    }
    path_prefix = "/mnt/runtime/write";
    pos = storage_path.find(path_prefix);
    LOG(WARNING) << "storage_path2= " + storage_path << ", pos=" << pos;
    if (pos != std::string::npos) {
        storage_path = storage_path.replace(pos, path_prefix.length(), "/storage");
    }

    const char* cpath2 = storage_path.c_str();
    /* @} */
    // Apps might still be handling eject request, so wait before
    // we start sending signals
    /* SPRD: replace storage path to /storage and add for unmount sdcard performance @{
     */
    sleep(1);
    Process::killProcessesWithOpenFiles(cpath2, SIGINT);
    sleep(1);
    if (!umount2(cpath, UMOUNT_NOFOLLOW) || errno == EINVAL || errno == ENOENT) {
        return OK;
    }
    LOG(WARNING) << "After SIGINT. Failed to unmount " << path;

    Process::killProcessesWithOpenFiles(cpath2, SIGTERM);
    sleep(1);
    if (!umount2(cpath, UMOUNT_NOFOLLOW) || errno == EINVAL || errno == ENOENT) {
        return OK;
    }
    LOG(WARNING) << "After SIGTERM. Failed to unmount " << path;

    Process::killProcessesWithOpenFiles(cpath2, SIGKILL);
    sleep(5);
    if (!umount2(cpath, UMOUNT_NOFOLLOW) || errno == EINVAL || errno == ENOENT) {
        return OK;
    }
    LOG(WARNING) << "After SIGKILL. Failed to unmount " << path;

    Process::killProcessesWithOpenFiles(cpath2, SIGKILL);
    sleep(5);
    if (!umount2(cpath, UMOUNT_NOFOLLOW) || errno == EINVAL || errno == ENOENT) {
        return OK;
    }
    LOG(WARNING) << "After Second SIGKILL. Failed to unmount " << path;

    /* SPRD: Lazy umount @{ */
    sleep(7);
    if (!umount2(cpath, UMOUNT_NOFOLLOW|MNT_DETACH) || errno == EINVAL || errno == ENOENT) {
        PLOG(WARNING) << "Success lazy unmount " << path;
        return OK;
    }
    LOG(ERROR) << "Failed to Lazy unmount " << path;
    /* @} */
    return -errno;
}

/* SPRD: add for storage @{ */
/*  create symlink  */
status_t CreateSymlink(const std::string& source, const std::string& target) {
    status_t res = 0;
    if(!remove(target.c_str())) {
        LOG(INFO) << "delete symlink " << target << "sucess";
    } else {
        LOG(INFO) << "delete symlink " << target << "fail:" << strerror(errno);
    }
    LOG(INFO) << "create symlink " << target << "->" << source;
    if (symlink(source.c_str(), target.c_str()) < 0) {
        PLOG(ERROR)<< "Failed to create symlink " << target << "->" << source;
        res = -errno;
    }
    return res;
}

/*  delete symlink  */
status_t DeleteSymlink(const std::string& path) {
    status_t res = 0;
    LOG(INFO) << "delete symlink " << path;
    if (remove(path.c_str()) < 0) {
        PLOG(ERROR)<< "Failed to delete symlink " << path;
        res = -errno;
    }
    return res;
}

status_t getBlkDeviceSize(const std::string& path, unsigned long long & size64) {
    status_t res = -1;
    const char* c_path = path.c_str();
    unsigned long long card_size;

    int fd = TEMP_FAILURE_RETRY(open(c_path, O_RDONLY));
    if (fd == -1) {
        PLOG(ERROR) << "Failed to open " << path;
        return res;
    }

    if ((ioctl(fd, BLKGETSIZE64, &card_size)) == -1) {
        PLOG(ERROR) << "Failed to get size(64bits) of block device " << path;
        goto done;
    }

    size64 = card_size;
    res = 0;

done:
    close(fd);
    return res;
}


