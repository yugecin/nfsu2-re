static
void dump_smsdata(int i, struct SmsData *sms)
{
	char *sms_id;
	char *str;
	unsigned int hash;
	int column;
	int len;

	sms_id = (*ptr838428) + sms->careerTextLanguageTableOffset;
	log(buf, sprintf(buf, "\nSMS DATA %d: (%08X) %s", i, cihash(sms_id), sms_id));
	if (!strncmp(sms_id, "SMS_CAR_UNLOCK_", strlen("SMS_CAR_UNLOCK_"))) {
		sms_id = "SMS_CAR_UNLOCK_1"; // see 5149C0-GetSmsMessageSubject
	}
	str = GetLanguageString(sms->senderLanguageLabel);
	log(buf, sprintf(buf, "  type %d mailbox %d f4 %08x f8 %08x moneyReward %d",
		sms->type, sms->mailboxId, sms->field_4, sms->field_8, sms->moneyReward));
	sprintf(buf, "%s_SUBJECT", sms_id);
	hash = cihash(buf);
	log(buf, sprintf(buf, "  subject: (%08X) %s", hash, GetLanguageString(hash)));
	log(buf, sprintf(buf, "   sender: (%08X) %s", sms->senderLanguageLabel, str));
	sprintf(buf, "%s_BODY", sms_id);
	hash = cihash(buf);
	str = GetLanguageString(hash);
	len = sprintf(buf, "     body: (%08X) ", hash);
	do {
		while (len < 80 && *str) {
			len += sprintf(buf + len, "%c", *str);
			str++;
		}
		log(buf, len);
		len = sprintf(buf, "                      ");
	} while (*str);
}

static
void debug_custom_dump_smsdata(int wparam)
{
	int type, i;

	if (wparam == 121) { // y
		for (type = 0; type < 16; type++) {
			for (i = 0; i < *numSmsDatas; i++) {
				if ((*smsDatas)[i].type == type) {
					dump_smsdata(i, *smsDatas + i);
				}
			}
		}
	}

	DEBUG_WMCHAR_FUNC(wparam);
}
#undef DEBUG_WMCHAR_FUNC
#define DEBUG_WMCHAR_FUNC debug_custom_dump_smsdata
