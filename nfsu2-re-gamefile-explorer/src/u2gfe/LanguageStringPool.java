package u2gfe;

import java.util.HashMap;

class LanguageStringPool
{
static final HashMap<Integer, String> strings = new HashMap<>();

static void put(int hash, String value)
{
	strings.put(hash, value);
}

static String get(int hash)
{
	String value = strings.get(hash);
	if (value == null) {
		return "DEFAULT STRING";
	}
	return value;
}
}
