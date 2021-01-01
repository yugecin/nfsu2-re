static const char *buttonmasknames[32] = {
	"BUTTON_PC_NAV_QUIT",
	"BUTTON_PC_NAV_LOGOFF",
	"BUTTON_PC_NAV_CONTINUE",
	"BUTTON_PC_NAV_INSTALL_PART",
	"BUTTON_PC_NAV_INSTALL_VINYL",
	"BUTTON_PC_NAV_INSTALL_DECAL",
	"BUTTON_PC_NAV_INSTALL_PACKAGE",
	"BUTTON_PC_NAV_INSTALL_PAINT",
	"BUTTON_PC_NAV_READ_MESSAGE",
	"BUTTON_CANCEL_CHANGES",
	"BUTTON_PC_NAV_BACK",
	"BUTTON_DYNO_RESET_OR_PC_NAV_RESET_KEYS",
	"BUTTON_PC_NAV_CUSTOMIZE",
	"BUTTON_PC_NAV_DELETE",
	"BUTTON_PC_NAV_EA_MESSENGER",
	"LAYOUT_WORLDMAP_POSITION",
	"LAYOUT_ONE_LINE_NO_BG",
	"LAYOUT_PAUSEMENU_POSITION",
	"BUTTON_DELETE_TUNED_CAR",
	"BUTTON_ICE_TEST_NOS_PURGE",
	"BUTTON_ICE_OPEN_CLOSE_HOOD",
	"BUTTON_ICE_OPEN_CLOSE_DOORS",
	"BUTTON_DYNO_TIP",
	"BUTTON_DECAL_COLOR_OR_BUTTON_FE_REPAINT",
	"BUTTON_MU_PAUSE_RESTART",
	"BUTTON_STRING_WORLD_MAP_SELECT",
	"BUTTON_FILTER_EVENT_OFF",
	"BUTTON_FILTER_EVENT_ON",
	"BUTTON_ACTIVATE_GPS",
	"BUTTON_DEACTIVATE_GPS",
	"BUTTON_OL_HOST_LAN_SERVER",
	"LAYOUT_RIGHT_POSITION",
};

static
void initGenerateFngdataList()
{
	struct FNGData *data;
	int i, j;
	char *sep;

	data = (void*) 0x7F7DC8;
	for (i = 0; i < 180; i++) {
		printf("/*%X*/{ \"%s\", // %d\n", data, data->name, i);
		printf("            0x%X,\n", data->initializeHandler);
		if (data->helpTextLanguageString) {
			printf("            %s,\n", languagelabel(data->helpTextLanguageString));
		} else {
			printf("            0,\n");
		}
		printf("            ");
		if (data->helpBarMask) {
			sep = "";
			for (j = 0; j < 32; j++) {
				if (data->helpBarMask & (1 << j)) {
					printf("%s%s", sep, buttonmasknames[j]);
					sep = " | ";
				}
			}
		} else {
			printf("0");
		}
		printf(",\n");
		printf("            0x%X, 0x%X, 0x%X },\n\n",
			data->field_10, data->field_14, data->field_18);
		data++;
	}

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initGenerateFngdataList
}
