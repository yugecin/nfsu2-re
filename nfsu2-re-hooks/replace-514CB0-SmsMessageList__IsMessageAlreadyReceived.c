static
int __stdcall SmsMessageList__IsMessageAlreadyReceived(struct SmsData *smsdata)
{
	struct SmsMessageList *this;

	_asm { mov this, ecx }

	return 0;
}

static
void initReplaceSmsMessageList__IsMessageAlreadyReceived()
{
	mkjmp(0x514CB0, &SmsMessageList__IsMessageAlreadyReceived);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initReplaceSmsMessageList__IsMessageAlreadyReceived
}
