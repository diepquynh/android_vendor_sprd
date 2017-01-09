#ifndef MORPHO_HDR
#define MORPHO_HDR

#define BYTE unsigned char

#ifdef __cplusplus
extern "C"
{
#endif
int HDR_Function(BYTE *Y0, BYTE *Y1, BYTE *Y2, BYTE* output, int height, int width, char *format);
#ifdef __cplusplus
}
#endif

#endif /* ifndef MORPHO_HDR */
