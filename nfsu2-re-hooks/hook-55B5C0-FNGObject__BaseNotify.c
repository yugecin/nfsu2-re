static
void doBaseNotifyHook(void *ignore, unsigned int message, int arg4, int arg8, struct FNGInfo *fngInfo)
{
	_asm { pushad }
	if (message != 0xC98356BA /*frame update/tick*/ &&
		message != 0x9803F6E2 /*cursor activity*/ &&
		message != 0x32C72D8F /*?*/ &&
		message != 0xFC946BFA /*?*/)
	{
		log(buf, sprintf(buf, "message %08X args %x %x %s", message, arg4, arg8, fngInfo->fngName));
	}
	_asm { popad }
}

static
__declspec(naked) void baseNotifyHook()
{
	_asm {
		call doBaseNotifyHook
		mov eax, 0x838521
		mov al, byte ptr [eax]
		push 0x55B5C5
		ret
	}
}

static
void initBaseNotifyHook()
{
	mkjmp(0x55B5C0, &baseNotifyHook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initBaseNotifyHook
}
