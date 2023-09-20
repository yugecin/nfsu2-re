package u2gfe;

import java.util.HashMap;

class Enum extends HashMap<Integer, String>
{
static Enum E_RACETYPE;
static Enum E_SHOP_SHOW_CONDITION_TYPE;
static Enum E_MARKER_TYPE;

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
}

private Enum(String name)
{
	this.name = name;
}
}
