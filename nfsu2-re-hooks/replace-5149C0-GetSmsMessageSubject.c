/***********************************************************************************************
5149C0 GetSmsMessageSubject
*/

#undef SMS_SUBJECT_PRINT

static
void __stdcall GetSmsMessageSubject(struct SmsMessage *this, char *dest, int maxlen)
{
	char *format;
	char *parameter;
	char *str;
	char buffer[128];

	if (this->subjectFormatLanguageLabel) {
		format = GetLanguageString(this->subjectFormatLanguageLabel);
		parameter = GetLanguageString(this->subjectAndBodyParameterLanguageLabel);
		sprintf(dest, format, parameter);
#ifdef SMS_SUBJECT_PRINT
		log(buf, sprintf(buf, "SMS_SUBJECT: format %s par %s", format, parameter));
		// example format: %s cover opportunity
		// example param: AutoMaxx Magazine
#endif
		return;
	}

	str = getCareerString(this->careerTextLanguageTableOffset);
	if (!strncmp(str, "SMS_CAR_UNLOCK_", strlen("SMS_CAR_UNLOCK_"))) {
#ifdef SMS_SUBJECT_PRINT
		log(buf, sprintf(buf, "SMS_SUBJECT: %s", str));
		// example: SMS_CAR_UNLOCK_17
		// example: SMS_CAR_UNLOCK_16
#endif
		str = GetLanguageString(0xF81E6031/*SMS_CAR_UNLOCK_1_SUBJECT*/);
		strcpy(dest, str);
		return;
	}

	sprintf(buffer, "%s_SUBJECT", str);
	str = GetLanguageString(cihash(buffer));
	strcpy(dest, str);
#ifdef SMS_SUBJECT_PRINT
	log(buf, sprintf(buf, "SMS_SUBJECT: %s", buffer));
	// example: ENGAGE_TIP_CAREER_PROG_SUBJECT
	// example: SMS_DVD_COVER_10_SUBJECT
#endif
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
