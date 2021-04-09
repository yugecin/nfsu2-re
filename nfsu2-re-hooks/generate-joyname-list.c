static
void initGenerateJoynameList()
{
	struct {
		int id;
		char *name;
	} *joydata = (void*) 0x7FABA8;
	int i;

	for (i = 0; i < 223; i++) {
		printf("%02X %s\n", joydata[i].id, joydata[i].name);
	}

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initGenerateJoynameList
}
