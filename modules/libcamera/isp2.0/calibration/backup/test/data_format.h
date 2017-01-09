#ifndef _DATA_FORMAT_H_
#define _DATA_FORMAT_H_

#include "sci_types.h"

#ifdef __cplusplus
    extern   "C"
    {
#endif

void write_data_uint16(const char *file_name, uint16_t *data, uint32_t size);
void write_data_double(const char *file_name, double *data, uint32_t item_per_line, uint32_t items);
void write_data_uint16_dec(const char *file_name, uint16_t *data, uint32_t item_per_line, uint32_t size);
void write_data_uint16_interlace(const char *file_name, uint16_t *data, uint32_t size);
uint32_t read_data_uint16(const char *file_name, uint16_t *data, uint32_t max_size);
uint32_t read_file(const char* file_name, void *data_buf,uint32_t buf_size);
int32_t save_file(const char *file_name, void *data, uint32_t data_size);
int32_t image_blc(uint16_t *dst, uint16_t *src, uint32_t width, uint32_t height, uint32_t bayer_pattern,
				 uint16_t blc_gr, uint16_t blc_r, uint16_t blc_b, uint16_t blc_gb);
void split_bayer_raw(uint16_t *dst[4], uint16_t *src, uint32_t w, uint32_t h);

#ifdef __cplusplus
	}
#endif

#endif
