package u2gfe;

import static u2gfe.Util.cstr;

class CareerStringPool
{
static final String DEFAULT_STRING = "(no career string pool)";

static BinFile originFile;
static byte pool[];
static int baseOffset;

static void put(BinFile file, byte data[], int from, int len)
{
	if (pool == null) {
		originFile = file;
		baseOffset = from;
		pool = new byte[len];
		System.arraycopy(data, from, pool, 0, len);
	}
}

static String get(int offset)
{
	if (pool == null || offset > pool.length) {
		return DEFAULT_STRING;
	}
	return cstr(pool, offset, pool.length);
}
}
