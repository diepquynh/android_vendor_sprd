#ifndef __ENG_DEBUG_H_
#define __ENG_DEBUG_H_

int eng_file_lock(void);
int eng_file_unlock(int fd);
void* eng_printlog_thread(void *x);

#endif /*__ENG_DEBUG_H_*/

