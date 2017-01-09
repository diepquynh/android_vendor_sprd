/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#define YTAG_TAG_PROPERTY       0x05
#define YTAG_TAG_NEWFILE_BEGIN  0x10
#define YTAG_TAG_NEWFILE_END    0x11
#define YTAG_TAG_RAWDATA        0x12
struct ytag {
    unsigned int tag; /* u32bits le32_to_cpu; All of them are Little Endian stored */
    unsigned int len;
    /* unsigned int namelen; */
    char data[0];
};

#define ytag_rawdata(buf, count) do { \
    ytag.tag = YTAG_TAG_RAWDATA; \
    ytag.len = sizeof(ytag) + count; \
    write(STDOUT_FILENO, &ytag, sizeof(ytag)); \
    if (count) \
        ret = write(STDOUT_FILENO, buf, count); \
} while (0);

#define ytag_newfile(_YTAG, _NAME) do { \
    int ynlen; char *ylname = _NAME; \
    ytag.tag = _YTAG; \
    ytag.len = sizeof(ytag); \
    if (ylname) { \
        ynlen = strlen(ylname); \
        ytag.len += ynlen; \
    } \
    write(STDOUT_FILENO, &ytag, sizeof(ytag)); \
    if (ylname && ynlen) \
        write(STDOUT_FILENO, ylname, ynlen); \
} while (0);

#define ytag_newfile_begin(_NAME) ytag_newfile(YTAG_TAG_NEWFILE_BEGIN, _NAME)
#define ytag_newfile_end(_NAME) ytag_newfile(YTAG_TAG_NEWFILE_END, _NAME)
