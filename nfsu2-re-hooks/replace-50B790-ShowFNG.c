/***********************************************************************************************
50B790 ShowFNG
*/

static
int ShowFNG(int fnghash, int arg1, int arg2)
{
	struct FNGShowData data;
	struct FNGData *fng;
	int result;
	int i;

	log(buf, sprintf(buf, "ShowFNG showing for hash: %p", fnghash));
	logvalue("ShowFNG", "arg1", arg1);
	logvalue("ShowFNG", "arg2", arg2);

	fng = NULL;
	for (i = 0; i < 180; i++) {
		if (cihash(fngdata[i].name) == fnghash) {
			fng = fngdata + i;
			log(buf, sprintf(buf, "ShowFNG matching fng name: %s", fng->name));
			if (fng->initializeHandler) {
				data.fngname = fngdata[i].name;
				data.arg1 = arg1;
				data.fngdata_field8 = fng->field_8;
				data.fngdata_fieldC = fng->field_C;
				data.arg2 = arg2;
				log(buf, sprintf(buf, "ShowFNG calling initializeHandler"));
				result = fng->initializeHandler(&data);
				logvalue("ShowFNG", "result", result);
			} else {
				result = 0;
				log(buf, sprintf(buf, "ShowFNG initializeHandler is NULL"));
			}
			return result;
		}
	}

	log(buf, sprintf(buf, "ShowFNG no matching fng found?"));
	return 0;
}

static
void initReplaceShowFNG()
{
	mkjmp(0x50B790, &ShowFNG);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initReplaceShowFNG
}
