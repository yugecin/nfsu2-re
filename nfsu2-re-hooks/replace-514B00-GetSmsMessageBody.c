/***********************************************************************************************
514B00 GetSmsMessageBody
*/

#define SMS_BODY_PRINT

static
void __stdcall GetSmsMessageBody(struct SmsMessage *this, char *dest, int maxlen)
{
	char *format;
	char *parameter;
	char *str;
	char buffer[128];

	if (this->bodyFormatLanguageLabel) {
		format = GetLanguageString(this->bodyFormatLanguageLabel);
		parameter = GetLanguageString(this->subjectAndBodyParameterLanguageLabel);
		sprintf(dest, format, parameter);
#ifdef SMS_BODY_PRINT
		log(buf, sprintf(buf, "SMS_BODY: format %s par %s", format, parameter));
		/*
		example format:
			One of my contacts has just called to let me know that a photographer
			from %s is in town looking for hot cars for next month's cover.
			Head to the star on the map and I'll call you with more details when
			you get there.
		example param:
			Street Tuner
		*/
#endif
		return;
	}

	str = getCareerString(this->data->careerTextLanguageTableOffset);
	if (!strncmp(str, "SMS_CAR_UNLOCK_", strlen("SMS_CAR_UNLOCK_"))) {
#ifdef SMS_BODY_PRINT
		log(buf, sprintf(buf, "SMS_BODY: %s", str));
		// example: SMS_CAR_UNLOCK_17
		// example: SMS_CAR_UNLOCK_16
#endif
		str = GetLanguageString(cihash("SMS_CAR_UNLOCK_1_BODY"));
		strcpy(dest, str);
		return;
	}

	if (!strncmp(str, "ENGAGE_TIPS_AUTO", strlen("ENGAGE_TIPS_AUTO"))) {
#ifdef SMS_BODY_PRINT
		log(buf, sprintf(buf, "SMS_BODY: %s", str));
		// Could not find any of these?
		// This is a single message about the autosave feature
		// This func probably prevents ENGAGE_TIPS_AUTO_XBOX from showing
#endif
		str = GetLanguageString(cihash("ENGAGE_TIPS_AUTO_BODY"));
		strcpy(dest, str);
		return;
	}

	sprintf(buffer, "%s_BODY", str);
	str = GetLanguageString(cihash(buffer));
	strcpy(dest, str);
#ifdef SMS_BODY_PRINT
	log(buf, sprintf(buf, "SMS_BODY: %s", buffer));
	// example: SMS_OUTRUN_DEFEAT_BODY
	// example: SMS_END_OF_GAME_MESSAGE_BODY
#endif
}

static
__declspec(naked)
void GetSmsMessageBodyWrapper()
{
	_asm {
		pop eax
		push ecx
		push eax
		jmp GetSmsMessageBody
	}
}

static
void initReplaceGetSmsMessageBody()
{
	mkjmp(0x514B00, &GetSmsMessageBodyWrapper);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initReplaceGetSmsMessageBody
}
