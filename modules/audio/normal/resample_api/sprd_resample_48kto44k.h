#include"trans_samplerate44k.h"

typedef void* transform_handle;
typedef int (*callback)(void* nr_hande, void* buffer, int bytes);
transform_handle SprdSrc_To_44K_Init(callback get_proc_data,void *data_handle, int request_channel);
int SprdSrc_To_44K_DeInit(transform_handle handle);
int SprdSrc_To_44K(transform_handle handle, void *buffer , int bytes);
