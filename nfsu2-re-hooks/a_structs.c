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
	int numButtons;
	char unused_32C;
	char isHelpDialog;
	char unused_32E;
	char textNeedsSomeWcharConversion; /*set to 0 in 55806D*/
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

struct U2RECT {
	float left, top, right, bottom;
};

struct UIData_Field8 {
	char pad[0xE4];
	struct FNGInfo *topPackage;
};

struct UIData {
	int f0;
	int f4;
	struct UIData_Field8 *field_8;
};

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
        struct FNGInfo *child;
        int field_8;
        char *fngName;
        unsigned int hash;
        int field_14;
        int field_18;
        int controlMask;
        int field_20;
        int field_24;
        int field_28;
        int field_2C;
        int field_30;
        int field_34;
        int field_38;
        int field_3C;
        int field_40;
        int field_44;
        int field_48;
        struct UIElement *rootUIElement;
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

#define SMS_TYPE_END_OF_GAME 1
#define SMS_TYPE_CAREER_START 2
#define SMS_TYPE_INSTRUCTIONS 3
#define SMS_TYPE_OUTRUN_INFO 4
#define SMS_TYPE_OUTRUN_VICTORY 5
#define SMS_TYPE_OUTRUN_DEFEAT 6
#define SMS_TYPE_UNLOCK_9 9
#define SMS_TYPE_UNLOCK_10 10
#define SMS_TYPE_UNLOCK_12 12
#define SMS_TYPE_MAGS_AND_UNIQUES 13
#define SMS_TYPE_DVD_COVER 14
#define SMS_TYPE_ENGAGE_TIP 15

struct SmsData {
	short careerTextLanguageTableOffset;
	char type;
	char mailboxId;
	int field_4;
	int field_8;
	int moneyReward;
	unsigned int senderLanguageLabel;
};

struct SmsMessage {
	struct SmsData *data;
	char read;
	char field_5;
	char deleted;
	char movedToCorrectInbox;
	unsigned int bodyFormatLanguageLabel;
	unsigned int subjectAndBodyParameterLanguageLabel;
	unsigned int subjectFormatLanguageLabel;
};

struct SmsMessageList {
	struct SmsMessage messages[256];
	int numMessages;
	int numUnreadMessages;
	int field_1408;
	char field_140C;
	char field_140D;
	char field_140E;
	char field_140F;
	int field_1410;
};

struct WideCharString {
	wchar_t *ptrString;
	int allocatedWcharLength; /*amount of wchars that can fit, excluding zero term*/
};

struct UIPos {
	int field_0;
	int field_4;
	int field_8;
	int field_C;
	int field_10;
	int field_14;
	int field_18;
	float leftOffset;
	float topOffset;
	int field_24;
	int field_28;
	int field_2C;
	int field_30;
	int field_34;
	float field_38;
	float field_3C;
};

#define UIELEMENT_FLAG_HIDDEN 1
#define UIELEMENT_FLAG_USE_CUSTOM_TEXT 2

struct UIElement {
	int vtable;
	struct UIElement *nextSibling;
	int field_8;
	int field_C;
	unsigned int hash;
	int field_14;
	int type;
	int someFlags;
	int field_20;
	int field_24;
	int field_28;
	struct UIPos *pos;
};

struct UILabel {
	struct UIElement __parent;
	int field_30;
	int field_34;
	int field_38;
	int field_3C;
	int field_40;
	int field_44;
	int field_48;
	int field_4C;
	int field_50;
	int field_54;
	int field_58;
	int field_5C;
	unsigned int textLanguageString;
	struct WideCharString string;
	int field_6C;
	int field_70;
};
EXPECT_SIZE(struct UILabel, 0x74);
ASSERT_OFFSET(struct UILabel, string, 0x64);

struct UIContainer {
	struct UIElement __parent;
	int field_30;
	int field_34;
	int field_38;
	int field_3C;
	int field_40;
	int field_44;
	int field_48;
	int field_4C;
	int field_50;
	int field_54;
	int field_58;
	int field_5C;
	int numChildren;
	struct UIElement *children;
};
EXPECT_SIZE(struct UIContainer, 0x68);
ASSERT_OFFSET(struct UIContainer, children, 0x64);

struct PoolLink {
        struct PoolLink *prev;
        struct PoolLink *next;
};
EXPECT_SIZE(struct PoolLink, 0x8);

struct PoolControl {
        int inited;
        struct PoolLink link;
};
EXPECT_SIZE(struct PoolControl, 0xC);

#define POOL_FLAG_EXTENDABLE 1

struct Pool {
	struct PoolLink __parent;
	struct Pool *nextLinkedPool;
	char* name;
	struct PoolEntry *firstAvailableElement;
	int flagsMaybe;
	int availableElements_;
	int maxAllocatedElements_;
	int field_20;
	int elementAmount;
	int elementSize;
	int elementAmountOverAllLinkedPools;
        /*pool entries come here*/
};
EXPECT_SIZE(struct Pool, 0x30);

struct PoolEntry {
        struct PoolEntry *nextEntry;
        struct PoolEntry *prevEntry_;
};
EXPECT_SIZE(struct PoolEntry, 0x8);

struct Marker {
        char type;
        char field_1;
        char field_2;
        char field_3;
        int field_4;
        int field_8;
        int field_C;
        int field_10;
        int field_14;
        int field_18;
        int field_1C;
        int field_20;
        int field_24;
        int field_28;
        int field_2C;
        unsigned int hash; /*for neighbourhood: hash of name, for engage: hash of sms name*/
        int field_34;
        int field_38;
        int field_3C;
        short radius;
        short markerStructSize;
        float pos_x;
        float pos_y;
};
