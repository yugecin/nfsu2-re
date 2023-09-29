#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef PRINT_3414A
#undef PRINT_LANGUAGE_STRINGS
#undef PRINT_30220_CARPRESETS
#undef PRINT_34A19_CAREERSPONSORS
#undef PRINT_34A1F_CARUNLOCKS

#define LINE_INDENT "    "

static
void read_3414A_stuff(FILE *in, int size)
{
#if PRINT_3414A
	char *data;
	char *pos;
	char *end;
	int counter434;
	int part_size;
	int i;
	struct {
		int f0, f4, f8, fC, f10, f14, f18, f1C;
		float f20;
		float f24;
		float f28;
		float f2C;
		int f30, f34, f38, f3C;
		short radius;
		short f42;
		float f44, f48;
	} *m;

	data = malloc(size);
	fread(data, size, 1, in);

	printf("3414A\n");
	counter434 = 0;
	pos = data;
	end = data + size;
	while (pos < end) {
		m = (void*) pos;
		part_size = *(short*) (pos + 0x42);
		printf("  part %d (size %d 0%02Xh):", counter434, part_size, part_size);
		if (pos[0] != 0x12) {
			printf("\n");
		} else {
			for (i = 0; i < part_size; i++) {
				if (!(i % 20)) {
					printf("\n   ");
				}
				printf(" %02X", pos[i] & 0xFF);
			}
			printf("\n");
			printf("    20 %.1f 24 %.1f 28 %.1f 2C %.1f\n", m->f20, m->f24, m->f28, m->f2C);
			printf("    30 %.1f 34 %.1f 38 %.1f 3C %.1f\n", m->f30, m->f34, m->f38, m->f3C);
			printf("    radius %d 44 %.1f 48 %.1f\n", m->radius, m->f44, m->f48);
		}
		counter434++;
		pos += part_size;
	}
	free(data);
#else
	fseek(in, size, SEEK_CUR);
#endif
}

static
void read_language_strings(FILE *in, int size)
{
#if defined PRINT_LANGUAGE_STRINGS
	struct {
		int numEntries;
		short entries[];
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
	char *data;

	data = malloc(size);
	fread(data, size, 1, in);
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
	free(data);
#else
	fseek(in, size, SEEK_CUR);
#endif
}

static
void read_30220_carpresets(FILE *in, int size)
{
#if defined PRINT_30220_CARPRESETS
	struct {
		void *link[2];
		char modelName[32];
		char name[32];
	} *preset;

	char *data;

	data = malloc(size);
	fread(data, size, 1, in);
	preset = (void*) data;
	while (size > 0) {
		printf("30220 entry: %s model: %s\n", preset->name, preset->modelName);
		preset = (void*) ((int) preset + 0x338);
		size -= 0x338;
	}
	printf("30220 size left is %d\n", size);
	free(data);
#else
	fseek(in, size, SEEK_CUR);
#endif
}

static
void read_34A19_careersponsors(FILE *in, int size)
{
#if defined PRINT_34A19_CAREERSPONSORS
	struct { /*8 members, size 10h*/ 
	/**offset in string table careerStringPool838428, which will point to something like STREETGLOW,
	which is then used to get name and info like SPONSOR_%S SPONSOR_INFO_%S*/
	/*0*/	short sponsorNameStrpoolOffset;
	/*2*/	short bankPerRaceWon;
	/*4*/	char field_4;
	/*5*/	char field_5;
	/*6*/	char field_6;
	/*7*/	char field_7;
	/*8*/	char _pad8[0x4];
	/**money amount*/
	/*C*/	short signingBonus;
	/**this sponsor will only be available when player's average reputation per race in the
	previous stage was equal or higher than this*/
	/*E*/	short requiredAverageReputation;
	} *sponsor;

	char *data;

	data = malloc(size);
	fread(data, size, 1, in);
	sponsor = (void*) data;
	while (size > 0) {
		printf("careersponsor:\n");
		printf("  bankPerRaceWon %d\n", sponsor->bankPerRaceWon);
		printf("  signingBonus %d\n", sponsor->signingBonus);
		printf("  requiredAverageReputation %d\n", sponsor->requiredAverageReputation);
		sponsor = (void*) ((int) sponsor + 0x10);
		size -= 0x10;
	}
	printf("34A19 size left is %d\n", size);
	free(data);
#else
	fseek(in, size, SEEK_CUR);
#endif
}

static
void read_34A1F_carunlocks(FILE *in, int size)
{
#if defined PRINT_34A1F_CARUNLOCKS
	struct { /*3 members, size Ch*/
		/*0*/	unsigned int carNameHash;
		/**to be matched with struct Bin34A11.hash8 when game region is US*/
		/*4*/	unsigned int hash4;
		/**to be matched with struct Bin34A11.hash8 when game region is NOT US*/
		/*8*/	unsigned int hash8;
	} *unlock;

	char *data;

	data = malloc(size);
	fread(data, size, 1, in);
	unlock = (void*) data;
	printf("car unlocks:\n");
	printf("  car-----  eu------  us------\n");
	while (size > 0) {
		printf("  %08X  %08X  %08X\n", unlock->carNameHash, unlock->hash4, unlock->hash8);
		unlock = (void*) ((int) unlock + 0xC);
		size -= 0xC;
	}
	printf("34A1F size left is %d\n", size);
	free(data);
#else
	fseek(in, size, SEEK_CUR);
#endif
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

		bytes_read = (int) fread(&section_header, 1, sizeof(section_header), in);
		offset += section_header.size + sizeof(section_header);
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
		case 0x3414A:
			read_3414A_stuff(in, section_header.size);
			break;
		case 0x34A1F:
			read_34A1F_carunlocks(in, section_header.size);
			break;
		case 0x30220:
			read_30220_carpresets(in, section_header.size);
			break;
		case 0x34A19:
			read_34A19_careersponsors(in, section_header.size);
			break;
		default:
			if (section_header.magic & 0x80000000) {
				sprintf(newlineprefix, "%s%s", lineprefix, LINE_INDENT);
				read_sections(in, section_header.size, newlineprefix);
			} else {
				fseek(in, section_header.size, SEEK_CUR);
			}
			break;
		}

		if (offset >= max_offset) {
			printf("%snest end\n", lineprefix);
			return 1;
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
		puts("\n\n\n") ||
		read_file("../../NeedForSpeed U2/TRACKS/ROUTESL4RA/Paths4001.bin") ||
		puts("\n\n\n") ||
		read_file("../../NeedForSpeed U2/GLOBAL/InGameRace.bun") ||
		puts("\n\n\n") ||
		read_file("../../NeedForSpeed U2/GLOBAL/GLOBALB.BUN");
}
