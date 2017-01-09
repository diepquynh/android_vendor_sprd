
#define LOG_TAG "record_nr"
#include"record_nr_api.h"
#define PROCESS_UNIT 1920

typedef struct  record_nr {
    void *pLeftBuffer;
    void *pRightBuffer;
    void *pProcBuffer;
    void *read_buffer;
    char *stage_buffer;
    int stage_size;
    CallBack read_pcm_data;
    void *data_read_handle;
    int channel;
} T_AUDIO_RECORD_NR;

//Record Nr Init
record_nr_handle AudioRecordNr_Init(int16 *pNvPara, CallBack read_pcm_data,
                                    void *data_read_handle, int request_channel)
{
    T_AUDIO_RECORD_NR *record_nr_res = NULL;
    record_nr_res = (T_AUDIO_RECORD_NR *)malloc(sizeof(T_AUDIO_RECORD_NR));
    if (NULL == record_nr_res)
    { goto err; }
    memset(record_nr_res, 0 , sizeof(T_AUDIO_RECORD_NR));

    record_nr_res->pLeftBuffer = (void *)malloc(PROCESS_UNIT / 2);
    if (NULL == record_nr_res->pLeftBuffer)
    { goto err; }
    memset(record_nr_res->pLeftBuffer, 0 , PROCESS_UNIT / 2);

    record_nr_res->pRightBuffer = (void *)malloc(PROCESS_UNIT / 2);
    if (NULL == record_nr_res->pRightBuffer)
    { goto err; }
    memset(record_nr_res->pRightBuffer, 0 , PROCESS_UNIT / 2);

    record_nr_res->pProcBuffer = (void *)malloc(PROCESS_UNIT);
    if (NULL == record_nr_res->pProcBuffer)
    { goto err; }
    memset(record_nr_res->pProcBuffer, 0 , PROCESS_UNIT);

    record_nr_res->stage_buffer = (char *)malloc(PROCESS_UNIT);
    if (NULL == record_nr_res->stage_buffer)
    { goto err; }
    memset(record_nr_res->stage_buffer, 0 , PROCESS_UNIT);

    record_nr_res->read_buffer = (void *)malloc(PROCESS_UNIT);
    if (NULL == record_nr_res->read_buffer)
    { goto err; }
    memset(record_nr_res->read_buffer, 0 , PROCESS_UNIT);

    record_nr_res->read_pcm_data = read_pcm_data;
    record_nr_res->data_read_handle = data_read_handle;
    record_nr_res->channel = request_channel;

    int16 *pNvInfo = pNvPara;
    ALOGD("Audio Nr Nv Info: 0x%x   0x%x    0x%x   %d    %d   %d   %d   %d",
          pNvInfo[0], pNvInfo[1],
          pNvInfo[2], pNvInfo[3], pNvInfo[4], pNvInfo[5], pNvInfo[6],
          pNvInfo[7]);
    /*init record nr*/
    audio_record_nr_init(pNvInfo);
    ALOGE("NR:audio_record_nr_init compelte\n");

    return (void *)record_nr_res;

err:
    if(record_nr_res) {
        if( record_nr_res->pLeftBuffer)
        { free(record_nr_res->pLeftBuffer); }
        if( record_nr_res->pRightBuffer)
        { free(record_nr_res->pRightBuffer); }
        if( record_nr_res->read_buffer)
        { free(record_nr_res->read_buffer); }
        if( record_nr_res->pProcBuffer)
        { free(record_nr_res->pProcBuffer); }
        if( record_nr_res->stage_buffer)
        { free(record_nr_res->stage_buffer); }
        free(record_nr_res);
    }
    return NULL;
}
static void AudioRecordNr_ProcEx(record_nr_handle nr_hande, void *buffer,
                                 size_t bytes, int channel)
{

    size_t read_bytes = bytes;
    unsigned int dest_count = 0;
    int proc_size = PROCESS_UNIT;

    if(channel == 2)
    { proc_size = PROCESS_UNIT; }
    else
    { proc_size = PROCESS_UNIT / 2; }
    T_AUDIO_RECORD_NR *record_nr_res = NULL;

    record_nr_res = (T_AUDIO_RECORD_NR *)nr_hande;
    int16_t  *ptrBuffer = (int16_t *)buffer;
    int16_t *buffLeft = record_nr_res->pLeftBuffer;
    int16_t *buffRight = record_nr_res->pRightBuffer;
    int16_t *temp_buf = (int16_t *)record_nr_res->pProcBuffer;

    int i = 0;
    do {
        if(proc_size <=  bytes) {
            read_bytes = proc_size;
        } else {
            read_bytes = bytes;
        }
        bytes -= read_bytes;
        ALOGE("AudioRecordNr_ProcEx byres = %d, read_bytes = %d", bytes, read_bytes);
        if(channel == 1) { //one channel,left buffer same as right buffer
            for ( i = 0;  i < read_bytes / 2; i++) {
                buffLeft[i] =  ptrBuffer[i];
                buffRight[i] = ptrBuffer[i];
            }
            AUDPROC_ProcessDpEx(buffLeft, buffRight, read_bytes / 2, temp_buf,
                                temp_buf + (read_bytes / 2), &dest_count);
            ALOGE("AudioRecordNr_ProcEx 1  read_byres  = %d, dest_count = %d", read_bytes,
                  dest_count);
            for ( i = 0;  i < read_bytes / 2; i++) {
                ptrBuffer[i] = temp_buf[i];
            }
        } else { //channel == 2
            for ( i = 0;  i < read_bytes / 4; i++) {
                buffLeft[i] =  ptrBuffer[i * 2];
                buffRight[i] = ptrBuffer[i * 2 + 1];
            }
            AUDPROC_ProcessDpEx( buffLeft, buffRight, read_bytes / 4, temp_buf,
                                 temp_buf + ( read_bytes / 4), &dest_count);
            ALOGE("AudioRecordNr_ProcEx 2 read_byres  = %d, dest_count = %d", read_bytes,
                  dest_count);
            for ( i = 0;  i < read_bytes / 4; i++) {
                ptrBuffer[i * 2] = temp_buf[i];
                ptrBuffer[i * 2 + 1] = temp_buf[i + read_bytes / 4];
            }
        }
        ptrBuffer = (uint8_t *) ptrBuffer + read_bytes;
    } while(bytes);
}

int AudioRecordNr_Proc(record_nr_handle nr_hande, void *buffer, size_t bytes)
{
    int ret = 0;
    int unit = PROCESS_UNIT;
    T_AUDIO_RECORD_NR *record_nr_res = NULL;
    record_nr_res = (T_AUDIO_RECORD_NR *)nr_hande;
    void *in = record_nr_res->data_read_handle;
    int channel = record_nr_res->channel;

    int transfered_size = 0;//the total size of the data  which have been transfered
    int new_storage_size = 0;

    if(record_nr_res->stage_size < bytes) {
        memcpy((char *)buffer, (char *)record_nr_res->stage_buffer,
               record_nr_res->stage_size);
        transfered_size = record_nr_res->stage_size;
        record_nr_res->stage_size = 0;
        while(1) {
            ret = record_nr_res->read_pcm_data(in, record_nr_res->read_buffer, unit);
            if(ret < 0) {
                ALOGE("read pcm data error");
                goto err;
            }
            AudioRecordNr_ProcEx(record_nr_res, record_nr_res->read_buffer, unit, channel);
            if((bytes - transfered_size) > unit) {
                memcpy((char *)buffer + transfered_size, (char *)record_nr_res->read_buffer,
                       unit);
                transfered_size += unit;
            } else {
                memcpy((char *)buffer + transfered_size, (char *)record_nr_res->read_buffer,
                       bytes - transfered_size);
                //storage the data left
                new_storage_size = transfered_size + unit - bytes; //update the storage size
                memcpy((char *)record_nr_res->stage_buffer,
                       (char *)record_nr_res->read_buffer + (unit - new_storage_size),
                       new_storage_size);
                record_nr_res->stage_size = new_storage_size;
                break;
            }
        }
    } else {
        memcpy(buffer, record_nr_res->stage_buffer, bytes);
        record_nr_res->stage_size = record_nr_res->stage_size - bytes;
        memcpy((char *)record_nr_res->stage_buffer,
               (char *)record_nr_res->stage_buffer + bytes, record_nr_res->stage_size);
    }
    return ret;
err:
    return -1;
}
void AudioRecordNr_Deinit(record_nr_handle nr_hande)
{
    T_AUDIO_RECORD_NR *record_nr_res = NULL;
    record_nr_res = (T_AUDIO_RECORD_NR *)nr_hande;
    ALOGE("AudioRcordNr_Deinit In");
    if(record_nr_res) {
        if( record_nr_res->pLeftBuffer)
        { free(record_nr_res->pLeftBuffer); }
        if( record_nr_res->pRightBuffer)
        { free(record_nr_res->pRightBuffer); }
        if( record_nr_res->read_buffer)
        { free(record_nr_res->read_buffer); }
        if( record_nr_res->pProcBuffer)
        { free(record_nr_res->pProcBuffer); }
        if( record_nr_res->stage_buffer)
        { free(record_nr_res->stage_buffer); }
        free(record_nr_res);
    }
    ALOGE("AudioRcordNr_Deinit OUT");
}

