static FILE *logfile;
static char buf[4096], buf2[4096];

static char newline = 10;

static
void log(char *msg, int len)
{
#ifdef LOG_TO_LOGFILE
	fwrite(msg, len, 1, logfile);
	fwrite(&newline, 1, 1, logfile);
	fflush(logfile);
#endif
#ifdef LOG_TO_DEBUGSTRING
	OutputDebugString(msg);
#endif
}

static
void hash_func_log(char *arg, int result, int address)
{
	if (arg != NULL) {
		log(buf, sprintf(buf, "hash\t%s\t%p\t%X", arg, result, address));
	}
}

static
void logvalue(char *lineprefix, char *varname, int value)
{
	float floatval;
	int derefval;

	floatval = *((float*) &value);
	log(buf, sprintf(buf, "%s   value '%s'", lineprefix, varname));
	log(buf, sprintf(buf, "%s     char %d 0x%X short 0x%d %X int %d 0x%X float %.1f",
		lineprefix, value & 0xFF, value & 0xFF, value & 0xFFFF, value & 0xFFFF,
		value, value, floatval));
	if (!IsBadReadPtr((void*) value, 4)) {
		log(buf, sprintf(buf, "%s      is valid pointer to:", lineprefix));
		derefval = *((int*) value);
		floatval = *((float*) &derefval);
		log(buf, sprintf(buf, "%s        char %d 0x%X short %d 0x%X int %d 0x%X float %.1f",
			lineprefix, value & 0xFF, value & 0xFF, value & 0xFFFF, value & 0xFFFF,
			value, value, floatval));
	}
	if (!IsBadStringPtrA((char*) value, sizeof(buf) - 10)) {
		log(buf, sprintf(buf, "%s      strptr: '%s'", lineprefix, value));
	}
}