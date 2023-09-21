package u2gfe;

import static u2gfe.Enum.*;
import static u2gfe.Structures.*;
import static u2gfe.Util.*;
import static java.lang.String.format;

import java.util.ArrayList;
import java.util.HashMap;

class Structures
{
static final int T_UNK = 0x1000000;
static final int T_STR = 0x2000000;
static final int T_INT = 0x3000000;
static final int T_FLOAT = 0x6000000;
static final int T_OBJLINK = 0x7000000;
/** data bytes are an id that is the elementtype struct */
static final int T_ARR_REPEATING = 0x8000000;
static final int T_HASHREF = 0x9000000;
/** data bytes are index of the field that contains some name for this struct entry */
static final int T_HASH = 0xA000000;
static final int T_PADDING = 0xB000000;
static final int T_CAREERSTRPOOL = 0xC000000;
static final int T_CAREERSTRPOOLREF = 0xD000000;
static final int T_ARR = 0xE000000;
static final int T_ENUM = 0xF000000;

static final int arr_entrytype(int type)
{
	if (type > 0xFF) {
		throw new RuntimeException();
	}
	return type << 16;
}
static final int arr_count(int count)
{
	if (count > 0xFFFF) {
		throw new RuntimeException();
	}
	return count;
}
static final int enum_type(int type) { return type << 8; }
static final int enum_width(int width) { return width; }

static HashMap<Integer, Object[]> structures;

static final Object MARK_STRUCT_START = new Object();
static final Object MARK_STRUCT_END = new Object();

private static ParseState ps0 = new ParseState(), ps1 = new ParseState(), ps2 = new ParseState();
/**
 * offset and size should not include header!
 *
 * @return Object[] with fields:
 *         - MARK_STRUCT_START
 *         - offset (start offset in data)
 *         - (actual size of struct instance)
 *         - name (string, nullable)
 *         - repeated x times:
 *            either again MARK_STRUCT_START which starts a nested entry, or:
 *              - name (string, nullable)
 *              - type (int, one of the T_* constants)
 *              - free data field for more info tied to type
 *              - startOffset (int, start offset in data)
 *              - endOffset (int, end offset in data)
 *         - MARK_STRUCT_END
 */
static ParseState parse(BinFile file, int magic, byte data[], int offset, int size)
{
	switch (magic) {
	case 0x30220: {
		int entrySize = 0x338;
		int numEntries = size / entrySize;
		ps0.start(format("car presets (%d)", numEntries), offset, size);
		for (; numEntries > 0; numEntries--) {
			String model = cstr(data, ps0.offset + 0x8, Integer.MAX_VALUE);
			String name = cstr(data, ps0.offset + 0x28, Integer.MAX_VALUE);
			ps1.start(format("car preset %s (%s)", name, model), ps0.offset, entrySize);
			ps1.put("link", T_OBJLINK, 8, null);
			ps1.put("model name", T_STR, 32, null);
			ps1.put("preset name", T_STR, 32, null);
			ps1.put(null, T_INT, 4, null);
			ps1.put(null, T_UNK, 680, null);
			ps1.put(null, T_UNK, 68, null);
			ps1.finish();
			ps0.add(ps1);
		}
		ps0.finish();
		break;
	}
	case 0x80034147: {
		ps0.start("paths data", offset, 0);
		ps0.finish();
		break;
	}
	case 0x3414A: {
		ps0.start("to be filled in below", offset, size);
		int entries = 0;
		while (ps0.sizeLeft > 0x43) {
			entries++;
			int markerSize = i16(data, ps0.offset + 0x42);
			int itype = i32(data, ps0.offset);
			String stype = E_MARKER_TYPE.get(itype);
			if (stype == null) {
				stype = format("%Xh", itype);
			}
			String name = format("marker (%s)", stype);
			ps1.start(name, ps0.offset, markerSize);
			ps1.put("type", T_ENUM, 4, E_MARKER_TYPE);
			ps1.put(null, T_FLOAT, 4, null);
			ps1.put(null, T_FLOAT, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_UNK, 4, null);
			Symbol.put(file, ps1.offset, ps1.name);
			ps1.put("hash", T_HASH, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put("radius", T_INT, 2, null);
			ps1.put("marker struct size", T_INT, 2, null);
			ps1.put("pos x", T_FLOAT, 4, null);
			ps1.put("pos y", T_FLOAT, 4, null);
			ps1.put(null, T_UNK, ps1.sizeLeft, null);
			ps1.finish();
			ps0.add(ps1);
		}
		ps0.name = format("markers (%d)", entries);
		ps0.finish();
		break;
	}
	case 0x80034A10:
		ps0.start("career block", offset, 0);
		ps0.finish();
		break;
	case 0x34A11: {
		int entrySize = 0x88;
		int numEntries = size / entrySize;
		ps0.start(format("career races (%d)", numEntries), offset, size);
		for (; numEntries > 0; numEntries--) {
			int stageIdx = i8(data, ps0.offset + 0x37);
			String name = format("career race (stage %d)", stageIdx);
			ps1.start(name, ps0.offset, entrySize);
			ps1.put(null, T_UNK, 4, null);
			ps1.put("post race movie name", T_CAREERSTRPOOLREF, 2, null);
			ps1.put(null, T_PADDING, 2, null);
			Symbol.put(file, ps1.offset, ps1.name);
			ps1.put("hash", T_HASH, 4, null);
			ps1.put(null, T_UNK, 1, null);
			ps1.put(null, T_UNK, 1, null);
			ps1.put(null, T_PADDING, 1, null);
			ps1.put("race type", T_ENUM, 1, E_RACETYPE);
			ps1.put(null, T_INT, 4, null);
			ps1.put(null, T_UNK, 4, null);
			for (int i = 0; i < 4; i++) {
				ps2.start("career race field 18", ps1.offset, 4);
				ps2.put(null, T_INT, 2, null);
				ps2.put(null, T_INT, 1, null);
				ps2.put(null, T_INT, 1, null);
				ps2.finish();
				ps1.add(ps2);
			}
			ps1.put("marker", T_HASHREF, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put("bank reward value", T_UNK, 4, null);
			ps1.put("bank reward type", T_INT, 1, null);
			ps1.put("num entries in 38", T_INT, 1, null);
			ps1.put(null, T_PADDING, 1, null);
			ps1.put("stage index", T_INT, 1, null);
			for (int i = 0; i < 8; i++) {
				ps2.start("career race field 38", ps1.offset, 8);
				ps2.put(null, T_INT, 1, null);
				ps2.put(null, T_PADDING, 3, null);
				ps2.put(null, T_HASHREF, 4, null);
				ps2.finish();
				ps1.add(ps2);
			}
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_INT, 1, null);
			ps1.put(null, T_PADDING, 1, null);
			ps1.put("num entries in 18", T_INT, 1, null);
			ps1.put(null, T_INT, 1, null);
			ps1.put(null, T_UNK, 8, null);
			ps1.finish();
			ps0.add(ps1);
		}
		ps0.finish();
		break;
	}
	case 0x34A12: {
		int entrySize = 0xA0;
		int numEntries = size / entrySize;
		ps0.start(format("career shops (%d)", numEntries), offset, size);
		for (; numEntries > 0; numEntries--) {
			int stageIdx = i8(data, ps0.offset + 0x9D);
			String name = format("career shop (stage %d)", stageIdx);
			ps1.start(name, ps0.offset, entrySize);
			ps1.put(null, T_UNK, 0x38, null);
			Symbol.put(file, ps1.offset, ps1.name);
			ps1.put("hash", T_HASH, 4, null);
			ps1.put("marker", T_HASHREF, 4, null);
			ps1.put(null, T_UNK, 0x11, null);
			ps1.put("is hidden shop", T_INT, 1, null);
			ps1.put(null, T_UNK, 0x22, null);
			int type = i8(data, ps1.offset + 0x28);
			if (type == 1) {
				ps1.put("extra map show condition value (race)", T_HASHREF, 4, null);
			} else {
				ps1.put("extra map show condition value (not a race)", T_INT, 4, null);
			}
			ps1.put(null, T_UNK, 0x24, null);
			ps1.put("extra map show condition type", T_ENUM, 1, E_SHOP_SHOW_CONDITION_TYPE);
			ps1.put("stage index", T_INT, 1, null);
			ps1.put(null, T_UNK, 0x2, null);
			ps1.finish();
			ps0.add(ps1);
		}
		ps0.finish();
		break;
	}
	case 0x34A18: {
		int entrySize = 0x50;
		int numEntries = size / entrySize;
		ps0.start(format("career stage settings (%d)", numEntries), offset, size);
		for (; numEntries > 0; numEntries--) {
			int stageIdx = i8(data, ps0.offset);
			String name = format("career stage settings (stage %d)", stageIdx);
			ps1.start(name, ps0.offset, entrySize);
			ps1.put("stage index", T_INT, 1, null);
			ps1.put("num sponsors", T_INT, 1, null);
			ps1.put("outrun stakes", T_INT, 2, null);
			ps1.put("showcase reward multiplier", T_INT, 2, null);
			ps1.put(null, T_PADDING, 2, null);
			ps1.put("sponsor 1", T_HASHREF, 4, null);
			ps1.put("sponsor 2", T_HASHREF, 4, null);
			ps1.put("sponsor 3", T_HASHREF, 4, null);
			ps1.put("sponsor 4", T_HASHREF, 4, null);
			ps1.put("sponsor 5", T_HASHREF, 4, null);
			ps1.put(null, T_UNK, 10, null);
			ps1.put(null, T_PADDING, 2, null);
			ps1.put(null, T_HASHREF, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_UNK, 1, null);
			ps1.put(null, T_UNK, 1, null);
			ps1.put(null, T_UNK, 1, null);
			ps1.put(null, T_UNK, 1, null);
			ps1.put(null, T_UNK, 1, null);
			ps1.put(null, T_UNK, 2, null);
			ps1.put(null, T_UNK, 1, null);
			ps1.put(null, T_UNK, 8, null);
			ps1.put("(outrun related)", T_INT, 1, null);
			ps1.put(null, T_UNK, 1, null);
			ps1.put(null, T_UNK, 2, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.finish();
			ps0.add(ps1);
		}
		ps0.finish();
		break;
	}
	case 0x34A19: {
		int entrySize = 0x10;
		int numEntries = size / entrySize;
		ps0.start(format("career sponsors (%d)", numEntries), offset, size);
		for (; numEntries > 0; numEntries--) {
			String sponsorname = CareerStringPool.get(i16(data, ps0.offset));
			int reqavgrep = i16(data, ps0.offset + 0xE);
			String name = format("career sponsor %s (reqavgrep %s)", sponsorname, reqavgrep);
			ps1.start(name, ps0.offset, entrySize);
			ps1.put("name", T_CAREERSTRPOOLREF, 2, null);
			ps1.put("bank reward per sponsor race", T_INT, 2, null);
			ps1.put(null, T_UNK, 1, null);
			ps1.put(null, T_UNK, 1, null);
			ps1.put(null, T_UNK, 1, null);
			ps1.put(null, T_UNK, 1, null);
			Symbol.put(file, ps1.offset, ps1.name);
			ps1.put("hash", T_HASH, 4, null);
			ps1.put("signing bonus", T_INT, 2, null);
			ps1.put("required average reputation", T_INT, 2, null);
			ps1.finish();
			ps0.add(ps1);
		}
		ps0.finish();
		break;
	}
	case 0x34A1D: {
		CareerStringPool.put(file, data, offset, size);
		ps0.start("career string pool", offset, size);
		ps0.put("entries", T_CAREERSTRPOOL, size, 0);
		ps0.finish();
		break;
	}
	default: {
		ps0.start(magic == 0 ? "padding" : null, offset, size);
		ps0.put(null, T_UNK, size, null);
		ps0.finish();
		break;
	}
	}
	return ps0;
}
} /*Structures*/

// ---

class ParseState
{
ArrayList<Object> result = new ArrayList<>(1000);
ArrayList<Throwable> errors = new ArrayList<>();
int startOffset;
int offset;
int size;
int sizeLeft;
int expectedSize;
String name;
boolean didAddUnexpectedEndError;

void start(String name, int offset, int size)
{
	this.name = name;
	this.offset = offset;
	this.startOffset = offset;
	this.sizeLeft = size;
	this.expectedSize = size;
	this.size = 0;
	this.errors.clear();
	this.result.clear();
	this.result.add(MARK_STRUCT_START);
	this.result.add(offset);
	this.result.add(size);
	this.result.add(name);
}

void put(String name, int type, int size, Object data)
{
	if (size <= this.sizeLeft) {
		if (size > 0xFFFFFF) {
			throw new RuntimeException("well shit");
		}
		this.size += size;
		this.sizeLeft -= size;
		this.result.add(name);
		this.result.add(type);
		this.result.add(data);
		this.result.add(this.offset);
		this.result.add(this.offset += size);
	} else if (!this.didAddUnexpectedEndError) {
		this.didAddUnexpectedEndError = true;
		this.sizeLeft = 0;
		this.result.add("unexpected end of data");
		this.result.add(T_UNK);
		this.result.add(null);
		this.result.add(offset);
		this.result.add(offset);
		errors.add(new Throwable("unexpected end"));
		errors.add(new Throwable(String.format(
			"unexpected end at offset %Xh (relative %Xh). expected %Xh read %Xh tried reading %Xh",
			this.offset,
			this.offset - this.startOffset,
			this.expectedSize,
			this.size,
			size
		)));
	}
}

void add(ParseState inner)
{
	this.offset = inner.offset;
	this.sizeLeft -= inner.size;
	this.size += inner.size;
	this.result.addAll(inner.result);
	this.errors.addAll(inner.errors);
}

void finish()
{
	this.result.set(3, this.name); // in case it was modified afterwards
	if (this.sizeLeft > 0) {
		errors.add(new Throwable(String.format(
			"%Xh trailing bytes from offset %Xh (relative %Xh). expected %Xh read %Xh trailing %Xh",
			this.sizeLeft,
			this.offset,
			this.offset - this.startOffset,
			this.expectedSize,
			this.size,
			this.sizeLeft
		)));
		this.put("unexpected extra data", T_UNK, this.sizeLeft, null);
	}
	this.result.add(MARK_STRUCT_END);
}
} /*ParseState*/
