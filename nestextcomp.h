#ifndef NESTEXTCOMP_H
#define NESTEXTCOMP_H

#include "lrtypes.h"

struct NTC;

// Compression
struct NTC *NTC_init();
int NTC_analyze(struct NTC *ntc, const char msg[]); // Returns 0 for ok, other for error
void NTC_finalize(struct NTC *ntc);

// Returns the compressed size, or negative for error
int NTC_compress(const struct NTC *ntc, const char msg[], u8 *dst, const u32 dstlen);

// You need this for decompression
u32 NTC_indexsize(const struct NTC *ntc);
u8 *NTC_getindex(const struct NTC *ntc);

void NTC_free(struct NTC *ntc);

// For use with the direct NES decompressor
struct defines {
	u16 low;
	u16 counts;
	u16 idxbase;
};

struct defines NTC_getdefines(const struct NTC *ntc);

// Decompression

// note that destination overflow is not checked
void NTC_decompress(const u8 *idx, const u8 *src, char *dst);

#endif
