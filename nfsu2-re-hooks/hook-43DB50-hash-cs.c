static
void SomeHashCS43DB50Print(char *arg, int *result)
{
	hash_func_log(arg, *result, 0x43DB50);

	if (arg != NULL) {
		if (strcmp(arg, "FIRETRUCK") == 0) {
			//*result = 0x83FBE66E;
		}
		if (strcmp(arg, "TAXI") == 0) {
			//*result = 0x001D1775

			// use ambulance for taxi
			//*result = 0x43A98EA7;
		}
		if (strcmp(arg, "240SX") == 0) {
			//*result = 0x0150FB80;

			// use firetruck for 240sx
			//*result = 0x83FBE66E;

			// use taxi for 240sx
			//*result = 0x001D1775;

			// use mustang for 240sx
			//*result = 0x35165819;
		}
	}
}

static
__declspec(naked) void SomeHashCS43DB50HookPost()
{
	_asm {
		mov edx, [esp+0x4]
		//; result on the stack (so the prnt func can modify it)
		push eax
		pushad
		lea ecx, [esp+0x4*8]
		//; ptr to result
		push ecx
		//; the original arg
		push edx
		call SomeHashCS43DB50Print
		add esp, 0x8
		popad
		//; get result back (in case we modified it)
		pop eax
		ret
	}
}

static
__declspec(naked) void SomeHashCS43DB50HookPre()
{
	_asm {
		//; the overwritten instruction for our jump
		mov edx, [esp+0x4]
		mov cl, [edx]
		//; not pushing the arg because it's only used in the lines above
		//; ret to SomeHashHookPost
		push SomeHashCS43DB50HookPost
		//; call the actual function (using ret...)
		push 0x43DB56
		ret
	}
}

static
void initHashHook43DB50()
{
	mkjmp(0x43DB50, &SomeHashCS43DB50HookPre);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHashHook43DB50
}
