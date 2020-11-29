static
void *__stdcall GetFNGInfoByName_Post(struct FNGInfo *info, char *name)
{
	if (info) {
		log(buf, sprintf(buf,
			"GetFNGInfoByName_Post %p hash %08X somestring '%s' name '%s'",
			info,
			info->hash,
			info->someString,
			name));
	}
	return info;
}

static
__declspec(naked) struct FNGInfo *GetFNGInfoByNameHook(char *name)
{
	_asm {
		mov eax, 0x8384D0
		mov eax, [eax]
		push [esp+4]
		mov ecx, 0x52CEF5
		call ecx
		add esp, 0x4
		push [esp+4]
		push eax
		call GetFNGInfoByName_Post
		ret
	}
}

static
void initHookGetFNGInfoByName()
{
	mkjmp(0x52CEF0, &GetFNGInfoByNameHook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHookGetFNGInfoByName
}
