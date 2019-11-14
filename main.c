#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "nestextcomp.h"

void nukenewline(char buf[]) {
	u32 i;
	for (i = 0; buf[i]; i++) {
		if (buf[i] == '\n') {
			buf[i] = '\0';
			break;
		}
	}
}

int main(int argc, char **argv) {

	if (argc != 2) {
		printf("Usage: %s file\n\n"
			"Take each non-empty line in the text file as a string,\n"
			"compress it, count the total sizes, and verify decompression.\n",
			argv[0]);
		return 0;
	}

	FILE *f = fopen(argv[1], "r");
	if (!f) {
		printf("Can't open file\n");
		return 1;
	}

	char buf[PATH_MAX];
	struct NTC *ntc = NTC_init();

	while (fgets(buf, PATH_MAX, f)) {
		nukenewline(buf);
		if (strlen(buf) < 2)
			continue;

		if (NTC_analyze(ntc, buf)) {
			printf("Analyze error\n");
			return 1;
		}
	}

	NTC_finalize(ntc);

	// Compression time
	rewind(f);
	u32 size = 0, packedsize = 0;

	packedsize = NTC_indexsize(ntc);
	const u8 *idx = NTC_getindex(ntc);
	printf("Index %u bytes\n", packedsize);

	if (0) { // If using the direct decompressor, print its defines
		const struct defines d = NTC_getdefines(ntc);
		printf("LOWOFF = %u\n", d.low);
		printf("COUNTSOFF = %u\n", d.counts);
		printf("IDXBASEOFF = %u\n", d.idxbase);
	}

	while (fgets(buf, PATH_MAX, f)) {

		u8 dst[PATH_MAX];
		char tmp[PATH_MAX];

		nukenewline(buf);
		const u32 len = strlen(buf);
		if (len < 2)
			continue;

		int ret = NTC_compress(ntc, buf, dst, PATH_MAX);
		if (ret < 0) {
			printf("Error compressing\n");
			return 1;
		}

		size += len;
		packedsize += ret;

		// Verify both decompression methods match the original
		NTC_decompress(idx, dst, tmp);
		if (strcmp(buf, tmp) != 0) {
			printf("_decompress failed for line '%s', got '%s'\n", buf, tmp);
			return 1;
		}
	}

	NTC_free(ntc);

	// And print stats, we're done
	printf("Compressed %u to %u bytes, %.2f%%\n",
		size, packedsize, packedsize * 100.0f / size);

	fclose(f);

	return 0;
}
