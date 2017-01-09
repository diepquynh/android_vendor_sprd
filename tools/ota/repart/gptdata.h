
#ifndef GPTDATA_H
#define GPTDATA_H

#include "common.h"
#include <fs_mgr.h>
#define SECTOR_SIZE 512
#define REPART_FLAG_FILE  ".repart_flag_file"
#define COMMAND_FILE  "/cache/recovery/command"
#define FIX_PATH  "/dev/block/platform/sdio_emmc/by-name"

#include "showinfo/show_info.h"

char * dev_name;
enum { RUN_ERROR = 0, RUN_NO_CHANGE, RUN_CHANGE, RUN_NO_SPACE, RUN_SUCCESS};
enum { CLEAR_FLAG = 0, SET_FLAG, GET_FLAG};
enum { RESIZEFS_MIN = 0, RESIZEFS_FULL, RESIZEFS_GETSIZE};
typedef struct fstab_rec Volume;

typedef struct _Partition{
    char        part_name[36];
    uint64_t    part_size;
    int         backup_flag;
    int         recovery_flag;
    uint32_t    real_pos;
} Partition;

typedef struct __GPTtab
{
  Partition           *partitions;
  uint32_t            partition_num;
  uint64_t            system_tora_size;
  int                 block_based;
  Partition           *import_part;
} GPTtab;

typedef struct __GPTHeader
{
   uint64_t signature;
   uint32_t revision;
   uint32_t headerSize;
   uint32_t headerCRC;
   uint32_t reserved;
   uint64_t currentLBA;
   uint64_t backupLBA;
   uint64_t firstUsableLBA;
   uint64_t lastUsableLBA;
   char diskGUID[16];
   uint64_t partitionEntriesLBA;
   uint32_t numParts;
   uint32_t sizeOfPartitionEntries;
   uint32_t partitionEntriesCRC;
   unsigned char reserved2[420];
} GPTHeader;

typedef struct __GPTPart
{
    //PartType partitionType;
    //GUIDData uniqueGUID;
    char PartType[16];  // PartType
    char GUIDData[16];  // GUIDData
    uint64_t firstLBA;
    uint64_t lastLBA;
    uint64_t attributes;
    uint16_t name[PARTITION_NAME];
} GPTPart;

typedef struct __GPTData
{
   GPTHeader mainHeader;
   GPTPart *partitions;
   uint32_t numParts;
   GPTHeader secondHeader;
   //MBRData protectiveMBR;
   char device[256];
   //DiskIO myDisk;
   int fd;
   uint32_t blockSize;
   uint64_t diskSize;
   int mainCrcOk;
   int secondCrcOk;
   int mainPartsCrcOk;
   int secondPartsCrcOk;

   uint64_t *partitionSize;
   GPTtab gpttab;
   int count;
   char *backup_dir;
   int needBackup;
   int repart_flag;
} GPTData;


void DisplayGPTData(GPTData *pData);
void DisplayConfigFile(GPTData *pData);
int FinalDestory(GPTData *pData);
int RecoveryPartition(GPTData *pData);
int ParseConfigFile(GPTData * pData, char *dev_name);
int BackupPartData2File(GPTData *pData, int change_pos);
int ChangePartitionSize(GPTData *pData);
int AddNewPartition(GPTData *pData);
int DeletePartition(GPTData *pData);
void InitializeGPTData(GPTData *pData);
int LoadPartitions(GPTData *pData, char *name);
int IsChangePartition(GPTData *pData, char *name);
char *par_getline(char *buf, int size, FILE *file);
int load_partition_table(GPTData *pData, char *name);
int OpenForRDWR(GPTData *pData, char *name);
int GetBlockSize(int fd);
long long GetDiskSize(int fd);
int ForceLoadGPTData(GPTData *pData);
int LoadHeader(GPTData *pData, GPTHeader *header, int fd, uint64_t sector, int *crcOk);
int CheckHeaderCRC(GPTHeader* header, int fd);
int SetGPTSize(GPTData *pData, uint32_t numEntries, int fillGPTSectors);
void MoveSecondHeaderToEnd(GPTData *pData);
int CheckHeaderValidity(GPTData *pData);
void RebuildSecondHeader(GPTData *pData);
void RebuildMainHeader(GPTData *pData);
int LoadPartitionTable(GPTData *pdata, GPTHeader *header, int fd);
int SaveGPTData(GPTData *pData, int change_pos);
void RecomputeCRCs(GPTData *pData);
int Write(void* buffer, int numBytes, int fd);
int SaveHeader(GPTHeader *header, int fd, uint64_t sector);
int DiskSync(int fd);
int SavePartitionTable(GPTData *pData, int fd, uint64_t sector);
int CheckPartitionChange(GPTData *pData);
int ChangePartition(GPTData *pData, int change_pos);
int IfNeedFormatPartition(GPTData * pData, const int change_pos, const char* part_name, const char* mount_point);
int RepartFlagFromBackdir(const char * back_dir, int mode, int * repart_flag);
int ParseCommandFile();
#endif
