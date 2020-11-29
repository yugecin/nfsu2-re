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
		int something;
		short entries[256];
	} *wchar_table;
	struct {
		unsigned int hash;
		int stringOffset;
	} *language_table_entry;
	struct {
		int wcharTableOffset;
		int numStrings;
		int tableOffset;
		int stringsOffset;
	} *language_header;
	char *strings;
	int i;
#endif
	char *data;
	void *pos;

	data = malloc(size);
	fread(data, size, 1, in);

#if defined PRINT_LANGUAGE_STRINGS
	language_header = (void*) data;
	language_table_entry = (void*) (data + language_header->tableOffset);
	wchar_table = (void*) (data + language_header->wcharTableOffset);
	strings = data + language_header->stringsOffset;
	printf("converstion tbl offset %08X\n", language_header->wcharTableOffset);
	printf("string tbl offset %08X\n", language_header->tableOffset);
	printf("strings offset %08X\n", language_header->stringsOffset);
	for (i = 0; i < 256; i++) {
		pos = (int) &wchar_table->entries[i] - (int) data + 8;
		printf("%p %3d: %04X\n", pos, i, wchar_table->entries[i] & 0xFFFF);
	}
	for (i = 0; i < language_header->numStrings; i++) {
		printf("%08X: ", language_table_entry[i].hash);
		printf("%s\n", strings + language_table_entry[i].stringOffset);
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
