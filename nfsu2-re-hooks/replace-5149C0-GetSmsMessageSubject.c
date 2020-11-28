/***********************************************************************************************
5149C0 GetSmsMessageSubject
*/

static
void __stdcall GetSmsMessageSubject(struct SmsMessage *this, char *dest, int maxlen)
{
	char *format;
	char *parameter;
	char *str;
	char buffer[50];

	if (this->subjectFormatLanguageLabel) {
		format = GetLanguageString(this->subjectFormatLanguageLabel);
		parameter = GetLanguageString(this->subjectParameterLanguageLabel);
		sprintf(dest, format, parameter);
		return;
	}

	str = getCareerString(this->careerTextLanguageTableOffset);
	if (!strncmp(str, "SMS_CAR_UNLOCK_", strlen("SMS_CAR_UNLOCK_"))) {
		str = GetLanguageString(0xF81E6031/*SMS_CAR_UNLOCK_1_SUBJECT*/);
		strcpy(dest, str);
		return;
	}

	sprintf(buffer, "%s_SUBJECT", str);
	str = GetLanguageString((unsigned int) cihash(buffer));
	strcpy(dest, str);
}

static
__declspec(naked)
void GetSmsMessageSubjectWrapper()
{
	_asm {
		pop eax
		push ecx
		push eax
		jmp GetSmsMessageSubject
	}
}

static
void initReplaceGetSmsMessageSubject()
{
	mkjmp(0x5149C0, &GetSmsMessageSubjectWrapper);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initReplaceGetSmsMessageSubject
}
