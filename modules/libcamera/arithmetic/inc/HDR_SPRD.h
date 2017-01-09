#ifndef SPRD_HDR
#define SPRD_HDR

#define BYTE unsigned char

#ifdef __cplusplus
extern "C"
{
#endif
void sprd_hdr_pool_init ();//线程初始化
int HDR_Function(BYTE *Y0, BYTE *Y1, BYTE *Y2, BYTE* output, int height, int width, char *format);
int sprd_hdr_pool_destroy ();//线程销毁
#ifdef __cplusplus
}
#endif

#endif /* ifndef SPRD_HDR */
