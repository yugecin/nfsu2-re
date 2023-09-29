package u2gfe;

import java.util.HashMap;

class Enum extends HashMap<Integer, String>
{
static Enum E_RACETYPE;
static Enum E_SHOP_SHOW_CONDITION_TYPE;
static Enum E_MARKER_TYPE;
static Enum E_SMS_TYPE;
static Enum E_MAILBOX;

String name;

static void init()
{
	E_RACETYPE = new Enum("E_RACETYPE");
	E_RACETYPE.put(0, "circuit");
	E_RACETYPE.put(1, "sprint");
	E_RACETYPE.put(2, "streetX");
	E_RACETYPE.put(3, "url");
	E_RACETYPE.put(4, "drag");
	E_RACETYPE.put(5, "drift");

	E_SHOP_SHOW_CONDITION_TYPE = new Enum("E_SHOP_SHOW_CONDITION_TYPE");
	E_SHOP_SHOW_CONDITION_TYPE.put(0, "none");
	E_SHOP_SHOW_CONDITION_TYPE.put(1, "race");
	E_SHOP_SHOW_CONDITION_TYPE.put(2, "stage 9C count <=");
	E_SHOP_SHOW_CONDITION_TYPE.put(3, "stage 0 count <=");
	E_SHOP_SHOW_CONDITION_TYPE.put(4, "stage 8C count <=");

	E_MARKER_TYPE = new Enum("E_MARKER_TYPE");
	E_MARKER_TYPE.put(0x4, "bodyshop area");
	E_MARKER_TYPE.put(0xD, "type D");
	E_MARKER_TYPE.put(0xE, "type E");
	E_MARKER_TYPE.put(0xF, "type F");
	E_MARKER_TYPE.put(0x11, "neighbourhood");
	E_MARKER_TYPE.put(0x12, "engage tip");
	E_MARKER_TYPE.put(0x13, "money pickup");

	E_SMS_TYPE = new Enum("E_SMS_TYPE");
	E_SMS_TYPE.put(0x1, "end of game");
	E_SMS_TYPE.put(0x2, "career start");
	E_SMS_TYPE.put(0x3, "instructions");
	E_SMS_TYPE.put(0x4, "outrun info");
	E_SMS_TYPE.put(0x5, "outrun victory");
	E_SMS_TYPE.put(0x6, "outrun defeat");
	E_SMS_TYPE.put(0x9, "unlock 9");
	E_SMS_TYPE.put(0xA, "unlock 10");
	E_SMS_TYPE.put(0xC, "unlock 12");
	E_SMS_TYPE.put(0xD, "magazines and uniques");
	E_SMS_TYPE.put(0xE, "dvd cover");
	E_SMS_TYPE.put(0xF, "engage tip");

	E_MAILBOX = new Enum("E_MAILBOX");
	E_MAILBOX.put(0x1, "inbox");
	E_MAILBOX.put(0x2, "gametips");
	E_MAILBOX.put(0x3, "special events");
	E_MAILBOX.put(0x4, "rachel");
	E_MAILBOX.put(0x5, "unlocks");
}

private Enum(String name)
{
	this.name = name;
}
}
