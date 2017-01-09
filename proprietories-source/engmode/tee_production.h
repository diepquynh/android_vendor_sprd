#ifndef  __TEEPRODUCTION_H__
#define __TEEPRODUCTION_H__

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
/* return 0 for success, not-zero for failure */
extern uint32_t TEECex_SendMsg_To_TEE(uint8_t* msg,uint32_t msg_len,uint8_t* rsp, uint32_t* rsp_len);
#ifdef __cplusplus
}
#endif //__cplusplus
#endif
