static
void onCharMessage(int wparam)
{
	struct SmsData *smsdata;

	log(buf, sprintf(buf, "wmchar %d", wparam));

	if (wparam == 121) {
		smsdata = ((struct SmsData*(__cdecl *)(unsigned int))0x501310)
					(cshash("SMS_INSTRUCTION"));
		log(buf, sprintf(buf, "smsdata %p", smsdata));
		if (smsdata) {
			SmsMessageList__SendMessageEvenIfAlreadyReceived(smsdata);
		}
	}
}

static
__declspec(naked) void charMessageHook()
{
	_asm {
		call onCharMessage
		mov eax, 0x55DBD0
		call eax
		mov eax, 0x5CCF5A
		jmp eax
	}
}

static
void initHookCharMessage()
{
	mkjmp(0x5CCF55, &charMessageHook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHookCharMessage
}
