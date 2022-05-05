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

static
__declspec(naked)
void AddFngToUIObject_1(char *fngName, void *data)
{
	_asm { mov eax, 0x555E80 }
	_asm { jmp eax }
}

static
__declspec(naked)
void SetUILabelByHashFormattedString(char *fngName, unsigned int hash, char *format, void *par)
{
	_asm { mov eax, 0x537B80 }
	_asm { jmp eax }
}

static
__declspec(naked)
void UILabel__setString(struct UILabel *this, char *str)
{
	_asm { pop eax }
	_asm { pop ecx }
	_asm { push eax }
	_asm { mov eax, 0x535FE0 }
	_asm { jmp eax }
}

static
__declspec(naked)
struct UIElement* __stdcall GetUIElementByHashInFngName(unsigned int hash, char *fngname)
{
	_asm { mov ecx, 0x8384C4 }
	_asm { mov ecx, [ecx] }
	_asm { mov ecx, [ecx+0x8] }
	_asm { mov eax, 0x52C570 }
	_asm { jmp eax}
}

static
__declspec(naked)
void *__stdcall Markers__FindAtPosWithTypeAfterIndex(void *_near, int type, void *after)
{
	_asm { mov ecx, 0x88F298 }
	_asm { mov eax, 0x5D9BF0 }
	_asm { jmp eax }
}

static
__declspec(naked)
void __stdcall PCHelpBarFNGObject__SyncByMask(int someMask, char *fngName)
{
	_asm { mov ecx, 0x8384D8 }
	_asm { mov ecx, [ecx] }
	_asm { mov eax, 0x54E6E0 }
	_asm { jmp eax }
}

static
__declspec(naked)
void __stdcall PCHelpBarFNGObject__Show()
{
	_asm { mov ecx, 0x8384D8 }
	_asm { mov ecx, [ecx] }
	_asm { mov eax, 0x54E6A0 }
	_asm { jmp eax }
}

static
__declspec(naked)
void __stdcall PCHelpBarFNGObject__Hide()
{
	_asm { mov ecx, 0x8384D8 }
	_asm { mov ecx, [ecx] }
	_asm { mov eax, 0x54E6B0 }
	_asm { jmp eax }
}

static
__declspec(naked)
struct NeighbourhoodName *__cdecl GetNeighbourhoodNameForHash(unsigned int hash)
{
	_asm { mov eax, 0x5D3DE0 }
	_asm { jmp eax }
}
