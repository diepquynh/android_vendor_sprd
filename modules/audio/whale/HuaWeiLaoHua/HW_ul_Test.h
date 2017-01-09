#ifndef HW_ul_Test_h
#define HW_ul_Test_h


#define int16 short
#define int32 int
#define uint16  unsigned  short


typedef  struct {
    int   hwStestResult;
    int   isReady;
    int   frequency;
    int16 input[160];
    int16 len;
}   AUDIO_LOOP_TEST_INFO_STRUCT_T;


typedef void* audio_hw_test_handle;

audio_hw_test_handle audio_HW_ul_Test_init(int16 Energy_ratio_thd , int16 pass_ratio_thd);
int audio_HW_ul_Test(AUDIO_LOOP_TEST_INFO_STRUCT_T* audio_test_info ,audio_hw_test_handle handle);
void audio_HW_ul_Test_deinit(audio_hw_test_handle handle);


#endif





