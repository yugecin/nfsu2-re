static char *markerTypeName[21] = {
	"(type 0)",
	"(type 1)",
	"(type 2)",
	"(type 3)",
	"(type 4)",
	"(type 5)",
	"(type 6)",
	"(type 7)",
	"(type 8)",
	"(type 9)",
	"(type 10)",
	"(type 11)",
	"(type 12)",
	"(type 13)",
	"(type 14)",
	"(type 15)",
	"(type 16)",
	"neighbourhood",
	"engage tip sms",
	"money pickup",
	"(type 20)",
};

static
void dump_marker(struct Marker *marker)
{
	struct NeighbourhoodName *nbhn;
	struct SmsData *sms;
	char *name;
	int i;

	log(buf, sprintf(buf, "  structsize %d x %f y %f radius %d",
		marker->markerStructSize, marker->pos_x, marker->pos_y, marker->radius));
	switch (marker->type) {
	case 0x11:
		nbhn = GetNeighbourhoodNameForHash(marker->hash);
		if (nbhn) {
			log(buf, sprintf(buf, "    neighbourhood: %s", nbhn->ptrName));
		} else {
			log(buf, sprintf(buf, "    neighbourhood (not found)"));
		}
		break;
	case 0x12:
		sms = NULL;
		for (i = 0; i < *numSmsDatas; i++) {
			if ((*smsDatas)[i].type == 15 /*ENGAGE TIP SMS TYPE*/ &&
				(*smsDatas)[i].hash == marker->hash)
			{
				sms = *smsDatas + i;
				break;
			}
		}
		if (sms) {
			name = *careerStringPool838428 + sms->careerTextLanguageTableOffset;
			log(buf, sprintf(buf, "    engage tip sms: %s", name));
		} else {
			log(buf, sprintf(buf, "    engage tip sms (not found)"));
		}
		break;
	}
}

static
void debug_custom_dump_markers(int wparam)
{
	struct Marker *marker;
	char *pointer;
	int type, num;
	void *end;

	if (wparam == 121) { // y
		log(buf, sprintf(buf, "%d markers:", pathsData->numMarkers));
		for (type = 0; type <= 20; type++) {
			pointer = (void*) pathsData->markers[type];
			end = pathsData->markers[type + 1];
			num = 0;
			while (pointer < end) {
				marker = (void*) pointer;
				num++;
				pointer += marker->markerStructSize;
			}
			log(buf, sprintf(buf, "  type %d (%s): %d markers", type, markerTypeName[type], num));
		}
		for (type = 0; type <= 20; type++) {
			log(buf, sprintf(buf, "type %d (%s) markers:", type, markerTypeName[type]));
			pointer = (void*) pathsData->markers[type];
			end = pathsData->markers[type + 1];
			while (pointer < end) {
				marker = (void*) pointer;
				dump_marker(marker);
				pointer += marker->markerStructSize;
			}
		}
	}

	DEBUG_WMCHAR_FUNC(wparam);
}
#undef DEBUG_WMCHAR_FUNC
#define DEBUG_WMCHAR_FUNC debug_custom_dump_markers
