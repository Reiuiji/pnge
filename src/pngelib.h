#ifndef PNGEUTIL_H
# define PNGEUTIL_H

#include "config.h"
#include "pngelib.c"
#include "secure.c"

/* Library Functions */
bool file_write_check(char *filename, pnge_options options);
void read_png_file(char *filename);
void write_png_file(char *filename,pnge_options options);
void write_text(char *filename, pnge_options options);
void encode_png_header(pnge_options options);
void decode_png_header(pnge_options options);
void encode_png_file(pnge_options options);
void decode_png_file(pnge_options options);
void process_text(char *filename);
void close_data(DataBuffer data);

/* Helper functions */
void print_value_bin(uint64_t value, int lenth);
void print_value_bit(uint64_t value,int length);

#endif
