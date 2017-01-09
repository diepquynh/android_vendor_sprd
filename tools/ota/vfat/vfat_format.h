/*
 * Created by Spreadst
 */


#ifndef VFAT_FORMAT_H_
#define VFAT_FORMAT_H_

#ifdef __cplusplus
extern "C" {
#endif

int format_vfat(const char *blk_device, unsigned int num_sectors, int wipe);

#ifdef __cplusplus
}
#endif

#endif  // VFAT_FORMAT_H_
