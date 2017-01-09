#ifndef _SPL_MERGE_H_
#define _SPL_MERGE_H_

#define  SPL_CONTENT_MAX_LEN 0x8000
#define  SPL_CHECKSUM_LEN 0x6000
#define  SPL_HEADER_LEN   512

// for secure boot
#define  CONFIG_SPL_HASH_LEN (1024) /*1KB*/

#define MERGE_TYPE_MTD  1
#define MERGE_TYPE_EMMC  2
#define MERGE_TYPE_UBI  3

int spl_merge(int merge_type, char *old_header, char *new_buf, int new_buf_len,
              char *merged_buf, int merged_buf_len, int checksum_len);

#endif /* _SPL_MERGE_H_ */
