#include "internal.h"

// Plain decompression, no preprocessing of the index
// note that destination overflow is not checked
void NTC_decompress(const u8 *idx, const u8 *src, char *dst) {

	const u8 numhuff = *idx++;
	const u8 * const huffs = idx;
	idx += numhuff;

	const u8 low = *idx++;
	const u8 numsym = *idx++;
	const u8 * const highbytes = idx;

	while (*idx++ != 255);

	const u8 * const counts = idx;
	idx += numsym;

	const u8 * const idxbase = idx;

	u8 len = *src++;
	len--;
	u8 prev = *src++;
	*dst++ = prev;

	u16 huffcode, bits;
	u8 huffpos, hufflen, storedbits, readbits = 0, srcbyte = 0, huffmod;

	srcbyte = *src;

	while (len--) {
		// Read bits one at a time, check against each possible huffman code
		huffpos = 0;
		huffmod = 0;
		huffcode = 0;
		storedbits = 0;
		bits = 0;
		hufflen = 1;
		while (!huffs[hufflen - 1])
			hufflen++;

		while (1) {
			do {
				bits <<= 1;
				bits |= srcbyte & 1;
				srcbyte >>= 1;

				storedbits++;
				readbits++;

				if (readbits == 8) {
					src++;
					srcbyte = *src;
					readbits = 0;
				}
			} while (storedbits < hufflen);

			// Generate each huffman code for this length, check them
			u8 found = 0;
			while (1) {
				if (bits == huffcode) {
					found = 1;
					break;
				}
				//printf("Huffman %u was %x, %u bits\n", huffpos, huffcode,
				//					hufflen);

				huffpos++;
				huffmod++;
				if (huffs[hufflen - 1] == huffmod) {
					const u8 prevlen = hufflen;
					hufflen++;
					while (!huffs[hufflen - 1])
						hufflen++;
					huffmod = 0;

					huffcode++;
					huffcode <<= hufflen - prevlen;
					break;
				}

				huffcode++;
			}

			if (found)
				break;
		}

		// Find our previous character's positition in the index stream
		// Reuse some vars to save RAM
		idx = idxbase;
		#define i huffmod

		for (i = 0; ; i++) {
			if (prev <= highbytes[i]) {
				idx += i << 8;
				break;
			}
		}

		idx += counts[prev - low];

		#undef i

		*dst++ = prev = idx[huffpos];
	}

	*dst = '\0';
}
