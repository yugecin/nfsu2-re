static
void HookAddFNGToUIObject(
	void *pUIData,
	int returnAddressIgnore,
	int unk,
	char *fngName,
	char *parentFngName,
	int controlMask,
	char *data)
{
	log(buf, sprintf(buf,
		"AddFNGToUIObject unk: %08X "
		"fngName '%s' parentFngName '%s' controlMask %d data %p",
		unk,
		fngName == 0 ? "itsNULL" : fngName,
		parentFngName == 0 ? "itsNULL" : parentFngName,
		controlMask,
		data));
}

static
__declspec(naked) void AddFNGToUIObjectHook()
{
	_asm {
		push ecx
		call HookAddFNGToUIObject
		pop ecx
		mov eax, 0x8384B0
		mov eax, [eax]
		push 0x555D05
		ret
	}
}

static
void initHookAddFNGToUIObject()
{
	mkjmp(0x555D00, &AddFNGToUIObjectHook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHookAddFNGToUIObject
}
