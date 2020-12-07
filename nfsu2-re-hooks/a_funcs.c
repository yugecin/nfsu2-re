static
char* getCareerString(short offset)
{
	return *ptr838428 + offset;
}

static
int cshash(char *input)
{
	unsigned int result;
	char c;
	int i, j;

	if (input == NULL) {
		return -1;
	}
	result = -1;
	i = strlen(input);
	j = 0;
	while (j < i) {
		result *= 33;
		result += input[j++];
	}
	return result;
}

static
unsigned int cihash(char *input)
{
	unsigned int result;
	char c;
	int i, j;

	if (input == NULL) {
		return -1;
	}
	result = -1;
	i = strlen(input);
	j = 0;
	while (j < i) {
		result *= 33;
		c = input[j++];
		if (c < 'a' || 'z' < c) {
			result += c;
		} else {
			result += c - 0x20;
		}
	}
	return result;
}

/*570A40*/
static
int UseCarUKNames()
{
	if (*_game_region == 1) {
		return 1;
	}
	if (*_game_region <= 4) {
		return 0;
	}
	if (*_game_region <= 9) {
		return 1;
	}
	return 0;
}

/*4FF9D0*/
static
char *GetLanguageStringOrNull(unsigned int hash)
{
	struct LanguageTableEntry *table;
	int min, max, mid;
	int midhash;

	table = loadedLanguage->ptrTable;
	min = 0;
	max = loadedLanguage->numStrings - 1;
	mid = (min + max) / 2;
	if (hash == loadedLanguage->ptrTable[mid].hash) {
		return loadedLanguage->ptrTable[mid].string;
	}

	for (;;) {
		if (max - min <= 2) {
			if (hash == table[min].hash) {
				return table[min].string;
			}
			if (hash == table[max].hash) {
				return table[max].string;
			}
		}
		midhash = table[mid].hash;
		if (hash == midhash) {
			return table[mid].string;
		} else {
			if (min == mid) {
				return NULL;
			}
			if (hash < midhash) {
				max = mid;
			} else {
				min = mid;
			}
		}
		mid = (min + max) / 2;
	}
}

/*4FFA80*/
static
char *GetLanguageString(unsigned int hash)
{
	char *str;

	str = GetLanguageStringOrNull(hash);
	if (str) {
		return str;
	}
	return GetLanguageStringOrNull(0xC01A6F63/*DEFAULT_STRING_ERROR*/);
}

static
_declspec(naked)
void __stdcall SmsMessageList__SendMessageEvenIfAlreadyReceived(struct SmsData *smsdata)
{
	_asm {
		push ebp
		mov ebp, esp
		push ebx
		mov ebx, [ebp+0x8] //smsdata
		push esi
		push edi
		mov edi, 0x860A60 // struct SmsMessageList*
		mov eax, 0x529605 // 5295F0+xx
		jmp eax
	}
}

static
_declspec(naked)
void __stdcall SmsMessageList__SendMessageByHash(int hash)
{
	_asm {
		mov ecx, 0x860A60
		mov eax, 0x533890
		jmp eax
	}
}

static
void wchar2char(char *dst, short *src)
{
	while ((*(dst++) = (0xFF & *(src++))));
}
