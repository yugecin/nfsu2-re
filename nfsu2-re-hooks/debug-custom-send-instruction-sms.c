static
void debug_custom_send_instruction_sms()
{
	struct SmsData *smsdata;

	if (wparam == 121) { // y
		smsdata = ((struct SmsData*(__cdecl *)(unsigned int))0x501310)
					(cshash("SMS_INSTRUCTION"));
		log(buf, sprintf(buf, "smsdata %p", smsdata));
		if (smsdata) {
			SmsMessageList__SendMessageEvenIfAlreadyReceived(smsdata);
		}
	}

	DEBUG_WMCHAR_FUNC();
}
#undef DEBUG_WMCHAR_FUNC
#define DEBUG_WMCHAR_FUNC debug_custom_send_instruction_sms
