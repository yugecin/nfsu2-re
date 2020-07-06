#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <string.h>

int main()
{
	FILE *in, *out;
	char buf[5000], buf1[1000];
	int len;
	int numentries, index, i, v;
	int offset;

	if ((in = fopen("../../NeedForSpeed U2/LANGUAGES/English.bin", "rb"))) {
		if ((out = fopen("../docs/language.txt", "wb"))) {
			fseek(in, 0x181C, SEEK_SET);
			index = 0;
			for (;;) {
				if (!fread(buf, 1, 8, in)) {
					printf("unexpected EOF\n");
					goto close;
				}
				v = *(int*)(buf);
				fwrite(buf1, sprintf(buf1,
					"%d | %08X %02X %02X %02X %02X\n",
					index,
					v,
					buf[4] & 0xFF, buf[5] & 0xFF,
					buf[6] & 0xFF, buf[7] & 0xFF), 1, out);
				if (v == -1) {
					goto next;
				}
				index++;
			}
next:
			fwrite("\n==\n\n", 5, 1, out);
			numentries = index;
			index = 0;
			offset = 0;
			for (;;) {
				len = (int) fread(buf + offset, 1, 500, in);
				len += offset;
				offset = 0;
				for (i = offset; i < len; i++) {
					if (buf[i] == 0) {
						fwrite(buf1, sprintf(buf1,
							"%d | len %04X: %s\n",
							index,
							i - offset,
							buf + offset), 1, out);
						index++;
						if (index > numentries) {
							goto close;
						}
						offset = i + 1;
					}
				}
				i = 0;
				for (i = 0; offset < len; offset++) {
					buf[i++] = buf[offset];
				}
				offset = i;
				if (len == 0) {
					goto close;
				}
			}
close:
			fclose(out);
		}
		fclose(in);
		return 0;
	}
	return 1;
}
