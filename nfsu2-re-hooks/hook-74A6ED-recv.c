static
__declspec(naked)
int __stdcall recvCall(void *s, char *buf, int len, int flags)
{
	_asm {
		mov eax, 0x75BC4E
		jmp eax
	}
}

static
int RecvHook(void *s, char *buf, int len, int flags)
{
	int result;
	int i;
	char *pos;

	result = recvCall(s, buf, len, flags);
	if (result > 0) {
		printf("got result of len %d\n", len);
		pos = buf;
		i = result;
		while (i--) {
			printf(" %02X", *pos);
			pos++;
		}
		printf("\n");
	}
	return result;
}

static
__declspec(naked) void HookRecvWrapper()
{
	_asm {
		call RecvHook
		add esp, 0x10 // stdcall
		mov ecx, 0x74A6F2
		jmp ecx
	}
}

static
void initHookRecv()
{
	mkjmp(0x74A6ED, &HookRecvWrapper);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHookRecv
}
