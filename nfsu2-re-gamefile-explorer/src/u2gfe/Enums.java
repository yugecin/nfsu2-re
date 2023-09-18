package u2gfe;

import java.util.HashMap;

class Enum extends HashMap<Integer, String>
{
static HashMap<Integer, Enum> registry;
static Enum E_RACETYPE;

String name;

static void init()
{
	registry = new HashMap<>();

	E_RACETYPE = new Enum("E_RACETYPE");
	E_RACETYPE.put(0, "circuit");
	E_RACETYPE.put(1, "sprint");
	E_RACETYPE.put(2, "streetX");
	E_RACETYPE.put(3, "url");
	E_RACETYPE.put(4, "drag");
	E_RACETYPE.put(5, "drift");
}

private Enum(String name)
{
	this.name = name;
}
}
