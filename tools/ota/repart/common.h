#ifndef COMMON_H
#define COMMON_H

#define SUCCESS 1
#define FAILER 0
#define SECTOR_SIZE 512
#define HEADER_SIZE 92
#define GPT_SIZE 128
#define GPT_ENTRY_SIZE 128
#define PARTITION_NAME 36
#define BACKUP_FLAG		47  //to flag if this partition data be backed up.
#define USERDATA_TOLETATE_SIZE (100 *1024 *1024) //Byte unit
#define IMPORT_PART_NUM 6
#define FOURK_BLOCK 4096

#define GPT_SIGNATURE 0x5452415020494645

//enum GPTValidity {gpt_valid, gpt_corrupt, gpt_invalid};

#if __LP64__
typedef unsigned long __uint64_t;
#else
typedef unsigned long long	uint64_t;
#endif
typedef	unsigned int	uint32_t;
typedef	unsigned short	uint16_t;

#endif
