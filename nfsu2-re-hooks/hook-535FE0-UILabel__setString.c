static
void UILabelSetStringLog(struct UILabel *this, char *str)
{
	log(buf, sprintf(buf, "setting UILabel@%p (hash %08X) string to: %s",
			this,
			this->__parent.hash,
			str));
}

static
__declspec(naked) int UILabelSetStringHook()
{
	_asm {
		push [esp+0x4] // char *str
		push ecx // this
		call UILabelSetStringLog
		pop ecx
		add esp, 0x4
		push ebx
		push esi
		push edi
		mov edi, ecx
		mov eax, 0x535FE5
		jmp eax
	}
}

static
void initHookUILabelSetString()
{
	mkjmp(0x535FE0, &UILabelSetStringHook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHookUILabelSetString
}
