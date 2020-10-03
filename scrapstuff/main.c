#include <stdio.h>

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
__declspec(naked) int jp41(int a, int b)
{
	_asm {
		fld [esp+0x4]
		fcomp [esp+0x8]
		fnstsw ax
		fstp [esp+0x4]
		test ah, 0x41
		jp p
		xor eax, eax
		ret
p:
		mov eax, 1
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

static
void testjp()
{
	printf("test fnstsw test 41h jp\n");
	printf("10 & 5: %d\n", jp41(0x41200000, 0x40a00000));
	printf("5 & 10: %d\n", jp41(0x40a00000, 0x41200000));
	printf("10 & 10: %d\n", jp41(0x41200000, 0x41200000));
	printf("-10 & -5: %d\n", jp41(0xC1200000, 0xc0a00000));
	printf("-5 & -10: %d\n", jp41(0xc0a00000, 0xC1200000));
	printf("-10 & -10: %d\n", jp41(0xC1200000, 0xC1200000));
}

int main()
{
	testsbb();
	testjp();
}
