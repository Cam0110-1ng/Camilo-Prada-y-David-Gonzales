// compression.h
#ifndef COMPRESSION_H
#define COMPRESSION_H

int lzw_compress(const unsigned char *input, long input_size, unsigned char **output);
int lzw_decompress(const unsigned char *input, long input_size, unsigned char **output);

#endif
