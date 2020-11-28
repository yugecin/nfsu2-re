static
void mkjmp(int at, void *to)
{
	/*at -= 0x400000; at += base;*/
	*((unsigned char*) at) = 0xE9;
	at++;
	*((int*) at) = (int) to - (at + 4);
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