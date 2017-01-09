/*
 * Created by Spreadst
 */


#ifndef UBIUTILS_H_
#define UBIUTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* get ubi device name from volume name, like /dev/ubi0_system */
char *ubi_get_devname(const char *volume_name);

/* mount_point is like "/system"
 * filesystem is like "ubifs"
 */
int ubi_mount(const char *volume_name, const char *mount_point,
                        const char *filesystem, int read_only);

/* attach ubi device file */
int ubi_attach(int ubi_num, const char *mtd_part_name);

/* used for format ubifs volume */
int ubi_update(const char *volume_name, long long len, const char *mountpoint);
int ubi_fupdate(int fd, long long len);

/* used for open ubifs volume device file */
int ubi_open(const char *volume_name, int flags);

#ifdef __cplusplus
}
#endif

#endif  // UBIUTILS_H_
