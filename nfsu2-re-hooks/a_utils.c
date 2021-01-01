static
void mkjmp(int at, void *to)
{
	/*at -= 0x400000; at += base;*/
	*((unsigned char*) at) = 0xE9;
	at++;
	*((int*) at) = (int) to - (at + 4);
}

static
void nop(int at, int num)
{
	/*at -= 0x400000; at += base;*/
	while (num--) {
		*((unsigned char*) at++) = 0x90;
	}
}

static
int isprocstaticmem(int loc)
{
	return base < loc && loc < base + 0x4BB000;
}

static
__declspec(naked) int nfsu2_stricmp(char *a, char *b)
{
	_asm {
		mov eax, 0x43DCC0
		jmp eax
	}
}

static
__declspec(naked) char *nfsu2_GetLanguageString(int hash)
{
	_asm {
		mov eax, 0x4FFA80
		jmp eax
	}
}

unsigned int hatoi(char *text)
{
	unsigned int col;
	int j, c;

	col = 0;
	for (j = 0; j < 8; j++) {
		c = text[j] - '0';
		if (c < 0 || 9 < c) {
			c = text[j] - 'A' + 10;
			if (c < 10 || 15 < c) {
				c = text[j] - 'a' + 10;
				if (c < 10 || 15 < c) {
					return 0;
				}
			}
		}
		col |= c << ((7 - j) * 4);
	}
	return col;
}

static
char *languagelabel(unsigned int key)
{
	static char *data = 0;
	static struct {
		int wcharTableOffset;
		int numStrings;
		int tableOffset;
		int stringsOffset;
	} *language_header;
	static char *strings;
	static struct {
		unsigned int hash;
		int stringOffset;
	} *table;

	FILE *in;
	struct {
		int magic;
		int size;
	} section_header;
	int offset;
	int i;

	if (!data) {
		if (!(in = fopen("../LANGUAGES/Labels.bin", "rb"))) {
			return "<failed to open labels file>";
		}

		offset = 0;
		while (fread(&section_header, 1, sizeof(section_header), in)) {
			offset += sizeof(section_header);
			if (section_header.magic == 0x39000) {
				data = malloc(section_header.size);
				fread(data, section_header.size, 1, in);
				language_header = (void*) data;
				strings = data + language_header->stringsOffset;
				table = (void*) (data + language_header->tableOffset);
				break;
			}
			offset += section_header.size;
			fseek(in, section_header.size, SEEK_CUR);
		}
		fclose(in);
	}

	for (i = 0; i < language_header->numStrings; i++) {
		if (table[i].hash == key) {
			return strings + table[i].stringOffset;
		}
	}
	return "<not found>";
}
