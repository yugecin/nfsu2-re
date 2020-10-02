__declspec(naked) int sbb(int val)
{
	_asm {
		mov eax, [esp+0x4]
		dec eax
		neg eax
		sbb eax, eax
		inc eax
		ret
	}
}

static
void testsbb()
{
	int i;

	printf("test sbb\n");
	for (i = -10; i < 10; i++) {
		printf("sbb %d: %d\n", i, sbb(i));
	}
}

int main()
{
	testsbb();
}