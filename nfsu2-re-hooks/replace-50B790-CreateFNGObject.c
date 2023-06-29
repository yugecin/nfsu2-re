/***********************************************************************************************
50B790 CreateFNGObject
*/

static
struct FNGObject* CreateFNGObject(int fnghash, struct FNGInfo *fngInfo, int arg2)
{
	struct FNGShowData data;
	struct FNGData *fng;
	struct FNGObject *result;
	int i;

	log(buf, sprintf(buf, "CreateFNGObject showing for hash: %p", fnghash));
	//logvalue("CreateFNGObject", "arg1", fngInfo);
	logvalue("CreateFNGObject", "arg2", arg2);

	fng = NULL;
	for (i = 0; i < 180; i++) {
		if (cihash(fngdata[i].name) == fnghash) {
			fng = fngdata + i;
			log(buf, sprintf(buf, "CreateFNGObject matching fng name: %s", fng->name));
			if (fng->initializeHandler) {
				data.fngname = fngdata[i].name;
				data.fngInfo = fngInfo;
				data.fngdata_field8 = fng->field_8;
				data.fngdata_fieldC = fng->field_C;
				data.arg2 = arg2;
				log(buf, sprintf(buf, "CreateFNGObject calling initializeHandler"));
				result = fng->initializeHandler(&data);
				logvalue("CreateFNGObject", "result", result);
			} else {
				result = 0;
				log(buf, sprintf(buf, "CreateFNGObject initializeHandler is NULL"));
			}
			return result;
		}
	}

	log(buf, sprintf(buf, "CreateFNGObject no matching fng found?"));
	return 0;
}

static
void initReplaceCreateFNGObject()
{
	mkjmp(0x50B790, &CreateFNGObject);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initReplaceCreateFNGObject
}
