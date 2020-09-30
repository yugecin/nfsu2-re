/*
55DC20 SendSomethingToFNG?
*/

static
void SendSomethingToFNG(
      int a, int b, int c, int d,
      int e, int f, int g, int h,
      int ignore_ret_adds,
      char *fng, int message)
{
	log(buf, sprintf(buf, "SendSomethingToFNG \"%s\", %p", fng, message));
}

static
__declspec(naked) void SendSomethingToFNGHook()
{
	_asm {
		pushad
		call SendSomethingToFNG
		popad
		mov eax, [esp+4]
		push eax
		push 0x55DC25
		ret
	}
}

static
void initSendSomethingToFNGHook()
{
	mkjmp(0x55DC20, &SendSomethingToFNGHook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initSendSomethingToFNGHook
}
