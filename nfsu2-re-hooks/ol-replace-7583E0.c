static
void Ol7583E0(int ignore, int arg0, char *buf, int len)
{
	int i;

	printf("7583E0 len %d\n", len);
	for (i = 0; i < len; i++) {
		if (i && !(i % 20)) {
			printf("\n");
		}
		printf("%02X ", ((unsigned int) buf[i]) & 0xFF);
	}
	printf("\n");
}

static
__declspec(naked) void Ol7583E0w()
{
	_asm {
		call Ol7583E0
		sub esp, 0x64
		mov eax, 0x81A1D0
		mov eax, [eax]
		mov ecx, 0x7583E8
		jmp ecx
	}
}

static
void initOl7583E0()
{
	mkjmp(0x7583E0, &Ol7583E0w);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initOl7583E0
}
