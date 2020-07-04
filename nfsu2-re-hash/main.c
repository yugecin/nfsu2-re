#include <stdio.h>
#include <string.h>

int main()
{
	char buf[200];
	char c;
	unsigned int len, i;
	unsigned int cs, ci;

	for (;;) {
		fgets(buf, sizeof(buf), stdin);
		len = (unsigned int) strlen(buf) - 1;
		buf[len] = 0; /*remove LF*/
		cs = ci = -1;
		for (i = 0; i < len; i++) {
			c = buf[i];
			cs *= 33;
			ci *= 33;
			cs += c;
			if (c < 'a' || 'z' < c) {
				ci += c;
			} else {
				ci += c - 0x20;
			}
		}
		printf("cs %08X ci %08X\n", cs, ci);
	}
	return 1;
}