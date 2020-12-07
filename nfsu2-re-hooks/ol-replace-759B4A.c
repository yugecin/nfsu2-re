static
void Ol759B4A(void *arg0, char *ebx)
{
	char data[1024]; /*1012 maybe*/
	struct {
		char b0; /*920*/
		char b1;
		char b2;
		char b3;
		char b4; /*91C*/
		char b5;
		char b6;
		char b7;
		char b8; /*918*/
		char b9;
		char b10; /*916*/
		char b11; /*915*/
		char b12; /*914*/
	} *header = (void*) &data;
	int len;

	printf("hi\n");
	len = *(int*)(ebx + 0x81B4);
	header->b0 = 1;
	header->b1 = 0;
	header->b2 = 2;
	header->b3 = 0;
	header->b4 = 3;
	header->b5 = 0;
	header->b6 = 0;
	header->b7 = len / 256;
	header->b8 = len & 0xFF;
	header->b9 = 1;
	header->b10 = 0;
	header->b11 = 0x80;
	memcpy(&header->b12, ebx + 0x81B8, len);

	((void(__cdecl*)(void*,char*,int))0x7583E0)(arg0, data, len + 0xC);
}

static
__declspec(naked) void Ol759B4Aw()
{
	_asm {
		push ebx
		push esi
		call Ol759B4A
		add esp, 0x8
		mov eax, 0x759BDE
		jmp eax
	}
}

static
void initOl759B4A()
{
	mkjmp(0x759B4A, &Ol759B4Aw);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initOl759B4A
}
