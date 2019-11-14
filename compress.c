#include <stdio.h>
#include <string.h>
#include "internal.h"

struct NTC *NTC_init() {
	return calloc(1, sizeof(struct NTC));
}

// Returns 0 for ok, other for error
int NTC_analyze(struct NTC *ntc, const char msg[]) {

	if (ntc->finalized)
		return -1;

	const u32 len = strlen(msg);

	if (len >= 256)
		return -1; // We assume the length fits in one byte.

	u32 i;
	const u32 max = len - 1;
	for (i = 0; i < max; i++) {
		ntc->prob[(u8) msg[i]][(u8) msg[i + 1]]++;
	}

	return 0;
}

static int probcmp(const void *ap, const void *bp) {
	const struct prob_t *a = ap;
	const struct prob_t *b = bp;

	if (a->count > b->count)
		return -1;
	if (a->count < b->count)
		return 1;
	return 0;
}

struct node_t {
	struct node_t *left, *right;
	u32 count;
};

static int nodecmp(const void *ap, const void *bp) {
	const struct node_t * const *a = ap;
	const struct node_t * const *b = bp;

	if ((*a)->count < (*b)->count)
		return -1;
	if ((*a)->count > (*b)->count)
		return 1;
	return 0;
}

#if 0
static void printtree(const struct node_t *base, const struct node_t *n, const u32 num,
			const u32 bits, const u32 rights) {


	if (!n) return;

	const u32 name = n - base;

	if (!n->left && !n->right) {
		printf("n%u [label=\"%u 0x%x (%u bits, %u rights)\"];\n",
			name, n->count,	num, bits, rights);
	} else {
		/*u32 i;
		for (i = 0; i < rights; i++) printf(" ");
		printf("^\n");*/
		printf("n%u -> n%u;\n", name, n->left - base);
		printf("n%u -> n%u;\n", name, n->right - base);
	}
	printtree(base, n->left, num << 1, bits + 1, rights);
	printtree(base, n->right, (num << 1) | 1, bits + 1, rights + 1);
}
#endif

static void canonicalize(u8 lens[256], const struct node_t *n, const u32 bits) {
	if (!n->left && !n->right) {
		lens[bits]++;
	} else {
		canonicalize(lens, n->left, bits + 1);
		canonicalize(lens, n->right, bits + 1);
	}
}

static void huffman(const u32 totalprob[256], const u32 len, u8 canonical[256]) {
	u32 i, qlen = 0, freenode = len;
	struct node_t *queue[len];

	/*
	1. Create a leaf node for each symbol and add it to the priority queue.
	2. While there is more than one node in the queue:
		1. Remove the two nodes of highest priority (lowest probability) from the queue
		2. Create a new internal node with these two nodes as children and with
		   probability equal to the sum of the two nodes' probabilities.
		3. Add the new node to the queue.
	3. The remaining node is the root node and the tree is complete.
	*/

	struct node_t nodes[len * 2];
	memset(nodes, 0, sizeof(struct node_t) * len * 2);

	for (i = 0; i < len; i++) {
		nodes[i].count = totalprob[i];

		queue[i] = &nodes[i];
	}

	qlen = len;
	qsort(queue, qlen, sizeof(void *), nodecmp);

	while (qlen > 1) {
		struct node_t *one, *two;
		one = queue[0];
		two = queue[1];

		qlen -= 2;
		memmove(queue, &queue[2], qlen * sizeof(void *));

		struct node_t *new = &nodes[freenode++];
		new->left = one;
		new->right = two;
		new->count = one->count + two->count;

		queue[qlen++] = new;
		qsort(queue, qlen, sizeof(void *), nodecmp);
	}

//	printtree(nodes, queue[0], 0, 0, 0);

	// Convert it to canonical format
	canonicalize(canonical, queue[0], 0);
}

static void canoncode(const u8 n, const u8 canon[256], u32 *val, u32 *bits) {
	/*
	code = 0
	while more symbols:
	    print symbol, code
	    code = (code + 1) << ((bit length of the next symbol) - (current bit length))
	*/
	u32 v = 0, i, curlen, mod = 1;
	for (i = 0; i < 256; i++)
		if (canon[i])
			break;
	curlen = i;

	for (i = 0; i < n; i++, mod++) {
		u32 shift = 0;
		if (mod >= canon[curlen]) {
			mod = 0;
			curlen++;
			shift++;
			while (!canon[curlen]) {
				curlen++;
				shift++;
			}
		}
		v = (v + 1) << shift;
	}

	*val = v;
	*bits = curlen;
}

void NTC_finalize(struct NTC *ntc) {
	u32 i, j;

	if (ntc->finalized)
		return;

	u32 totalfollowers = 0;

	u8 low = 255, high = 0, used = 0;
	for (i = 0; i < 256; i++) {
		u32 followers = 0;
		for (j = 0; j < 256; j++) {
			if (!ntc->prob[i][j])
				continue;

			ntc->oprob[i].arr[followers].what = j;
			ntc->oprob[i].arr[followers].count = ntc->prob[i][j];

			followers++;
			if (ntc->prob[i][j] > ntc->maxhits)
				ntc->maxhits = ntc->prob[i][j];
		}
		if (followers > ntc->maxfollowers)
			ntc->maxfollowers = followers;

		ntc->oprob[i].count = followers;
		totalfollowers += followers;

		if (followers) {
			qsort(ntc->oprob[i].arr, ntc->oprob[i].count,
				sizeof(struct prob_t), probcmp);

			used++;
			if (i < low)
				low = i;
			if (i > high)
				high = i;
		}
	}

	printf("%u max hits, %u max followers, low-high %u %u, count %u\n",
		ntc->maxhits, ntc->maxfollowers, low, high, used);

	u32 totalprob[256] = { 0 };
	for (i = low; i <= high; i++) {
		for (j = 0; j < ntc->oprob[i].count; j++) {
			totalprob[j] += ntc->oprob[i].arr[j].count;
		}
	}

	u8 canon[256] = { 0 };

	huffman(totalprob, ntc->maxfollowers, canon);

	// Build index
	u8 canonpairs = 0;
	for (i = 0; i < 256; i++) {
		if (!canon[i]) continue;
		canonpairs++;
	}

	for (i = 0; i < ntc->maxfollowers; i++) {
		u32 val, bits;
		canoncode(i, canon, &val, &bits);

		ntc->huffval[i] = val;
		ntc->huffbits[i] = bits;
	}

	u16 sumpos[256] = { 0 }, tmp16 = 0;
	u8 sumposH[256] = { 0 };
	for (i = low; i <= high; i++) {
		sumpos[i] = tmp16;
		sumposH[i] = tmp16 >> 8;
		tmp16 += ntc->oprob[i].count;
	}

	const u8 numhighbytes = sumposH[high] + 1;

	const u32 maxbits = ntc->huffbits[ntc->maxfollowers - 1];
	const u32 numsymbols = high - low + 1;

	ntc->idxlen = 1 + maxbits + 2 + numhighbytes + numsymbols + totalfollowers;
	ntc->idx = calloc(ntc->idxlen, 1);

	u32 pos = 0;
	// Huffman tree
	ntc->idx[pos++] = maxbits;
	for (i = 1; i <= maxbits; i++) {
		ntc->idx[pos++] = canon[i];
	}

	ntc->defs.low = pos;

	ntc->idx[pos++] = low;
	ntc->idx[pos++] = numsymbols;

	for (i = 0; i <= high; i++) {
		if (i == high)
			ntc->idx[pos++] = 255;
		else if (sumposH[i + 1] != sumposH[i])
			ntc->idx[pos++] = i;
	}

	ntc->defs.counts = pos;

	// Low byte of the follower array position
	for (i = low; i <= high; i++) {
		ntc->idx[pos++] = sumpos[i] & 0xff;
	}

	ntc->defs.idxbase = pos;

	// Finally, the probability-ordered symbols themselves
	for (i = low; i <= high; i++) {
		for (j = 0; j < ntc->oprob[i].count; j++) {
			ntc->idx[pos++] = ntc->oprob[i].arr[j].what;
		}
	}

	if (pos != ntc->idxlen) abort();

	ntc->finalized = 1;
}

static const unsigned char BitReverseTable256[256] = {
#   define R2(n)     n,     n + 2*64,     n + 1*64,     n + 3*64
#   define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
#   define R6(n) R4(n), R4(n + 2*4 ), R4(n + 1*4 ), R4(n + 3*4 )
    R6(0), R6(2), R6(1), R6(3)
};

// Reversing bits moves a bit of work from decompression to compression side
static u16 bitreverse(u16 in, const u8 bits) {
	in = (BitReverseTable256[in & 0xff] << 8) |
		(BitReverseTable256[(in >> 8) & 0xff]);
	in >>= 16 - bits;
	return in;
}

// Returns the compressed size, or negative for error
int NTC_compress(const struct NTC *ntc, const char msg[], u8 *dst, const u32 dstlen) {

	if (!ntc->finalized)
		return -1;

	// Our NES decompression code can only handle up to 16 bit codes.
	// Purely a code-space saving decision
	if (ntc->huffbits[ntc->maxfollowers - 1] > 16)
		return -1;

	const u32 len = strlen(msg);
	const u8 * const endpos = dst + dstlen;
	const u8 * const start = dst;
	u32 i = 0;

	#define put(a) if (dst >= endpos) return -1; *dst = a; dst++

	put(len);
	put(msg[i++]);

	u8 bitbyte = 0, storedbits = 0;

	while (msg[i]) {
		const u8 prev = msg[i - 1];
		const u8 cur = msg[i];

		u32 j;
		for (j = 0; j < ntc->oprob[prev].count; j++) {
			if (ntc->oprob[prev].arr[j].what == cur)
				break;
		}

		const u8 prob = j;
		const u8 bits = ntc->huffbits[prob];
		u16 val = bitreverse(ntc->huffval[prob], bits);

		//printf("%c to %c (%u %u), %uth pos, %u bits\n",
		//	prev, cur, prev, cur, prob, bits);

		// inverted LE bit writer
		for (j = 0; j < bits; j++) {
			bitbyte >>= 1;
			bitbyte |= (val & 1) << 7;
			val >>= 1;

			storedbits++;

			if (storedbits == 8) {
				put(bitbyte);
				bitbyte = 0;
				storedbits = 0;
			}
		}

		i++;

		// Lingering bits
		if (!msg[i] && storedbits) {
			bitbyte >>= 8 - storedbits;
			put(bitbyte);
		}
	}

	#undef put

	return dst - start;
}

u32 NTC_indexsize(const struct NTC *ntc) {
	if (!ntc->finalized)
		return 0;
	return ntc->idxlen;
}

u8 *NTC_getindex(const struct NTC *ntc) {
	return ntc->idx;
}

void NTC_free(struct NTC *ntc) {
	free(ntc->idx);
	free(ntc);
}

struct defines NTC_getdefines(const struct NTC *ntc) {
	if (!ntc->finalized)
		return (struct defines) { 0, 0, 0 };
	return ntc->defs;
}
