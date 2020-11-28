struct DialogInfo {
	char text[768];
	int unused300;
	int unused304;
	int unused308;
	int unused30C;
	int unused310;
	int unused314;
	int unused318;
	int unused31C;
	char *pParentFNG; /*read in 5580EB*/
	char *pMyFNG;
	int unused_328;
	char unused_32C;
	char isHelpDialog;
	char unused_32E;
	char something32F; /*set to 0 in 55806D*/
	int something330; /*set to 0 in 558074*/
	int unused334;
};
EXPECT_SIZE(struct DialogInfo, 0x338);
ASSERT_OFFSET(struct DialogInfo, pMyFNG, 0x324);
ASSERT_OFFSET(struct DialogInfo, isHelpDialog, 0x32D);

struct CarModelInfo {
	char brand[0x20];
	char brand2[0x20];
	char geometry_bin[0x20];
	char geometry_lzc[0x20];
	char unk80[0x20];
	char unkA0[0x20];
	char manufacturer[0x40];
	char pad[0x790];
};
EXPECT_SIZE(struct CarModelInfo, 0x890);
ASSERT_OFFSET(struct CarModelInfo, manufacturer, 0xC0);

typedef int (fnginithandler)(struct FNGShowData *msg);

struct FNGData {
	char *name;
	fnginithandler *initializeHandler;
	int field_8;
	int field_C;
	int field_10;
	int field_14;
	int field_18;
};
EXPECT_SIZE(struct FNGData, 0x1C);

struct FNGShowData {
	char *fngname;
	int arg1;
	int fngdata_field8;
	int fngdata_fieldC;
	int arg2;
};
EXPECT_SIZE(struct FNGShowData, 0x14);

struct FNGInfo {
	int field_0;
	int field_4;
	int field_8;
	int field_C;
	int field_10;
	int field_14;
	int field_18;
	int controlMask;
};

struct MouseData {
	void /*DInputDevice8*/ *dinputdevice;
	int cursorX; // on 640x480 canvas
	int cursorY; // on 640x480 canvas
	int previousCursorX; // on 640x480 canvas
	int previousCursorY; // on 640x480 canvas
	int deltaCursorX; // on 640x480 canvas
	int deltaCursorY; // on 640x480 canvas
	int mousestate_lZ; // scrollwheel data
	char areMouseButtonsSwapped; // result of GetSystemMetrics(SM_SWAPBUTTON);
	char button0State; // left mouse button
	char button0JustPressed;
	char button0JustReleased;
	char button1State; // right mouse button
	char button1JustPressed;
	char button1JustReleased;
	char button2State; // middle mouse button
	char button2JustPressed;
	char button2JustReleased;
};

struct LanguageTableEntry {
	unsigned int hash;
	char *string;
};

struct LoadedLanguage {
        int numStrings;
        char **ptrStrings;
        struct LanguageTableEntry *ptrTable;
        int field_C;
};

struct SmsMessage {
	short *careerTextLanguageTableOffset;
	int field_4;
	int field_8;
	unsigned int subjectParameterLanguageLabel;
	unsigned int subjectFormatLanguageLabel;
};