
//#define DEBUG_UV_DENOISE     "uv_denoise: L %d, %s: "
//#define DENOISE_LOG(format,...) printf(DEBUG_UV_DENOISE format,  ##__VA_ARGS__)


typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef signed int int32_t;
typedef signed short int16_t;
typedef signed char int8_t;

typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef signed int int32;
typedef signed short int16;
typedef signed char int8;


//uint32_t uv_denoise_slice(int8_t *dst, int8_t *src, uint32_t w, uint32_t h, int32 slice_pat, int8_t *buffer_dc);
uint32_t uv_denoise_slice(int8_t *dst, int8_t *src, uint32_t w, uint32_t h, int32 slice_pat, int8_t *buffer_dc, uint8_t *in_y, uint8_t *out_y);

