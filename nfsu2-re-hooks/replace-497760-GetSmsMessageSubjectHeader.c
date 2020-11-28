/***********************************************************************************************
497760 GetSmsMessageSubjectHeader
*/

static
void GetSmsMessageSubjectHeader(struct SmsMessage *sms, char *dest, int maxlen)
{
	char *subjectFormat;
	char subject[100];

	subjectFormat = GetLanguageStringOrNull(0x4B5DE3E9/*SMS_SUBJECT_HEADER*/);
	GetSmsMessageSubject(sms, subject, sizeof(subject));
	sprintf(dest, subjectFormat, subject);
}

static
void initReplaceGetSmsMessageSubjectHeader()
{
	mkjmp(0x497760, &GetSmsMessageSubjectHeader);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initReplaceGetSmsMessageSubjectHeader
}
