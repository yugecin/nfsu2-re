#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef PRINT_LANGUAGE_STRINGS

static
void read_language_strings(FILE *in, int size)
{
#if defined PRINT_LANGUAGE_STRINGS
	struct {
		int somethingOffset;
		int numStrings;
		int tableOffset;
		int stringsOffset;
	} *language_header;
	int pos;
	int i;
#endif
	char *data;

	data = malloc(size);
	fread(data, size, 1, in);

#if defined PRINT_LANGUAGE_STRINGS
	language_header = (void*) data;
	for (i = 0; i < language_header->numStrings; i++) {
		pos = language_header->tableOffset;
		pos += i * 8;
		printf("%08X: ", *(int*) (data + pos));
		pos += 4;
		pos = language_header->stringsOffset + *(int*) (data + pos);
		printf("%s\n", (char*) (data + pos));
	}
#endif

	free(data);
}

int main()
{
	struct {
		int magic;
		int size;
	} section_header;
	FILE *in;
	int bytes_read;
	int offset;

	if (!(in = fopen("../../NeedForSpeed U2/LANGUAGES/English.bin", "rb"))) {
		puts("can't open file");
		return 1;
	}

	offset = 0;
	while (1) {
		printf("offset %X\n", offset);
		bytes_read = (int) fread(&section_header, 1, sizeof(section_header), in);
		if (!bytes_read) {
			puts("end");
			goto close;
		}
		if (bytes_read != sizeof(section_header)) {
			printf("expected %d bytes for section header but read only %d\n",
				sizeof(section_header), bytes_read);
			goto err;
		}
		printf("section %08X size %X\n", section_header.magic, section_header.size);
		if (section_header.magic == 0x39000) {
			read_language_strings(in, section_header.size);
		} else {
			fseek(in, section_header.size, SEEK_CUR);
		}
		offset += section_header.size + sizeof(section_header);
	}

close:
	fclose(in);
	return 0;
err:
	fclose(in);
	return 1;
}
