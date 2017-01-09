#ifndef AUDIO_DEV_LAOHUA_TEST_H
#define AUDIO_DEV_LAOHUA_TEST_H
struct dev_laohua_test_buffer_info_t {
    int16 *base ;
    int size ;
    int offset ;
    int total_size ;
};
struct dev_laohua_test_info_t{
    pthread_t thread;
    bool is_exit;
    sem_t   sem;
    int fd;
    struct pcm_config config ;
    int devices;
    int frequency ;
    struct pcm *pcm ;
    struct dev_laohua_test_buffer_info_t bufffer_info ;
    struct tiny_audio_device *dev;
};

enum dev_laohua_test_mode {
    IN_MODE =0,
    OUT_MODE,
};

#endif
