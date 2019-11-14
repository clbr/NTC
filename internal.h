#include <stdlib.h>
#include "nestextcomp.h"

// Index format:
// u8 size of Huffman tree
// canonical Huffman len list
// u8 low, num
// list of high byte ranges of follower positions
// list of follower positions' low bytes, u8 each
// probability-ordered lists

struct prob_t {
	u8 what;
	u32 count;
};

struct outerprob_t {
	struct prob_t arr[256];
	u8 count;
};

struct NTC {
	u8 finalized;

	u32 prob[256][256];
	u32 maxhits; // The largest follower hit count
	u32 maxfollowers; // The largest amount of followers

	struct outerprob_t oprob[256];

	u16 huffval[256];
	u8 huffbits[256];

	u8 *idx;
	u32 idxlen;

	struct defines defs;
};
