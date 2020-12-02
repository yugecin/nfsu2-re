/***********************************************************************************************
440B40 AllocateAndInitPool
*/

char *printCreatedPoolsFormat = "created pool@%p; unk %p element amt %p element size %p name '%s'";
char *bufptr = buf;

static
__declspec(naked) void printCreatedPools()
{
	_asm {
		pushad

		push dword ptr [ecx+0xC]
		push dword ptr [ecx+0x28]
		push dword ptr [ecx+0x24]
		push dword ptr [ecx+0x20]
		push ecx
		push dword ptr printCreatedPoolsFormat
		push dword ptr bufptr
		call sprintf
		add esp, 0x1C

	}
	log(buf, strlen(buf));
	_asm {

		popad

		push 0x440B9B // ret addr
		push 0x43CD40 // call to PoolInit?
		ret
	}
}

static
void initPrintCreatedPools()
{
	mkjmp(0x440B96, &printCreatedPools);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initPrintCreatedPools
}
