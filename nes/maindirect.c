#include <stdio.h>
#include "data.h"

void NTC_decompress(const unsigned char *src, char *dst);

int main() {

	static unsigned short i;
	static char buf[128];

	for (i = 0; i < NUM; i++) {
		NTC_decompress(strings[i], buf);
		printf("%s\n", buf);
	}

	return 0;
}
