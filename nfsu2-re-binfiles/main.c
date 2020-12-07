#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef PRINT_LANGUAGE_STRINGS
#define LINE_INDENT "    "

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
	void *pos;
	int i;
#endif
	char *data;

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

int read_sections(FILE *in, int max_offset, char *lineprefix)
{
	struct {
		int magic;
		int size;
	} section_header;
	int bytes_read;
	int offset;
	char newlineprefix[50];

	offset = 0;
	while (1) {
		printf("%soffset %X\n", lineprefix, offset);
		offset += section_header.size + sizeof(section_header);

		bytes_read = (int) fread(&section_header, 1, sizeof(section_header), in);
		if (!bytes_read) {
			printf("%send\n", lineprefix);
			return 0;
		}
		if (bytes_read != sizeof(section_header)) {
			printf("%sexpected %d bytes for section header but read only %d\n",
				lineprefix, sizeof(section_header), bytes_read);
			return 1;
		}
		printf("%ssection %08X size %X\n",
			lineprefix, section_header.magic, section_header.size);
		switch (section_header.magic) {
		case 0x39000:
			read_language_strings(in, section_header.size);
			break;
		case 0x80034147:
			sprintf(newlineprefix, "%s%s", lineprefix, LINE_INDENT);
			read_sections(in, section_header.size, newlineprefix);
			break;
		case 0x80034A10:
			sprintf(newlineprefix, "%s%s", lineprefix, LINE_INDENT);
			read_sections(in, section_header.size, newlineprefix);
			break;
		default:
			fseek(in, section_header.size, SEEK_CUR);
			break;
		}

		if (offset >= max_offset) {
			printf("%snest end\n", lineprefix);
			return;
		}
	}
}

int read_file(char *filename)
{
	FILE *in;
	int result;

	printf("%s\n", filename);
	if (!(in = fopen(filename, "rb"))) {
		puts("can't open file");
		return 1;
	}

	result = read_sections(in, 0x7FFFFFFF, "");
	fclose(in);
	return result;
}

int main()
{
	return
		read_file("../../NeedForSpeed U2/Languages/English.bin") ||
		read_file("../../NeedForSpeed U2/TRACKS/ROUTESL4RF/Paths4604.bin") ||
		puts("\n\n\n") ||
		read_file("../../NeedForSpeed U2/GLOBAL/GLOBALB.BUN");
}
