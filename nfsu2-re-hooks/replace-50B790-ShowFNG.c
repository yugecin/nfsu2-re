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

	fng = NULL;
	result = 0;
	for (i = 0; i < 180; i++) {
		if (cihash(fngdata[i].name) == fnghash) {
			fng = fngdata + i;
			if (fng->initializeHandler) {
				data.fngname = fngdata[i].name;
				data.arg1 = arg1;
				data.fngdata_field8 = fng->field_8;
				data.fngdata_fieldC = fng->field_C;
				data.arg2 = arg2;
				result = fng->initializeHandler(&data);
			}
			break;
		}
	}

	log(buf, sprintf(buf,
		"ShowFNG(%p (\"%s\"), %p, %p) = %p",
		fnghash,
		fng == NULL ? "" : fng->name,
		arg1,
		arg2,
		result));

	return result;
}

static
void initReplaceShowFNG()
{
	mkjmp(0x50B790, &ShowFNG);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initReplaceShowFNG
}
