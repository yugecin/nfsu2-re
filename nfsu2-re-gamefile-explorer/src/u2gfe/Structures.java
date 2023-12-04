package u2gfe;

import static u2gfe.Enum.*;
import static u2gfe.Structures.*;
import static u2gfe.Util.*;
import static java.lang.String.format;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Set;

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
static final int T_LITSTR = 0xE000000;
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
private static ParseState ps3 = new ParseState();
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
			int hash = i32(data, ps0.offset + 0x30);
			String name = format("marker %8Xh (%s)", hash, stype);
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
			Symbol.put(file, ps1.offset, ps1.name); // TODO: this might not be marker hash but a hashref
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
			int hash = i32(data, ps0.offset + 8);
			String name = format("career race %8Xh (stage %d)", hash, stageIdx);
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
			Symbol.reference(name, file, ps1.offset);
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
				Symbol.reference(name + " field 38." + i, file, ps2.offset);
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
			String n = cstr(data, ps0.offset, offset + size);
			String name = format("career shop %s (stage %d)", n, stageIdx);
			ps1.start(name, ps0.offset, entrySize);
			ps1.put("name (and then some)", T_STR, 32, null);
			ps1.put("movie name", T_STR, 24, null);
			Symbol.put(file, ps1.offset, ps1.name);
			ps1.put("hash", T_HASH, 4, null);
			Symbol.reference(name, file, ps1.offset);
			ps1.put("marker", T_HASHREF, 4, null);
			ps1.put("platform bin name", T_STR, 16, null);
			ps1.put(null, T_UNK, 1, null);
			ps1.put("is hidden shop", T_INT, 1, null);
			ps1.put(null, T_UNK, 0x22, null);
			int type = i8(data, ps1.offset + 0x28);
			if (type == 1) {
				Symbol.reference(name, file, ps1.offset);
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
	case 0x34A14: {
		int entrySize = 0x44;
		int numEntries = size / entrySize;
		ps0.start(format("bin34A14 (%d)", numEntries), offset, size);
		for (; numEntries > 0; numEntries--) {
			ps1.start("entry", ps0.offset, entrySize);
			ps1.put(null, T_UNK, entrySize, null);
			ps1.finish();
			ps0.add(ps1);
		}
		ps0.finish();
		break;
	}
	case 0x34A15: {
		int entrySize = 0x17C;
		int numEntries = size / entrySize;
		ps0.start(format("bin34A15 (%d)", numEntries), offset, size);
		for (; numEntries > 0; numEntries--) {
			ps1.start("entry", ps0.offset, entrySize);
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put("num entries in field C", T_INT, 4, null);
			for (int i = 0; i < 3; i++) {
				ps2.start("field C entry " + i, ps1.offset, 0x5C);
				ps2.put("index", T_INT, 4, null);
				ps2.put(null, T_UNK, 0x3C, null);
				ps3.start("more indices 40", ps2.offset, 5 * 4);
				for (int j = 0; j < 5; j++) {
					ps3.put("0", T_INT, 4, null);
				}
				ps3.finish();
				ps2.add(ps3);
				ps3.start("more indices 54", ps2.offset, 2 * 4);
				for (int j = 0; j < 2; j++) {
					ps3.put("0", T_INT, 4, null);
				}
				ps3.finish();
				ps2.add(ps3);
				ps2.finish();
				ps1.add(ps2);
			}
			ps1.put(null, T_UNK, 0x17C - 0x120, null);
			ps1.finish();
			ps0.add(ps1);
		}
		ps0.finish();
		break;
	}
	case 0x34A16: {
		int entrySize = 0x40;
		int numEntries = size / entrySize;
		ps0.start(format("career dvds/zines (%d)", numEntries), offset, size);
		for (; numEntries > 0; numEntries--) {
			String name = format("34A16 entry %s", cstr(data, ps0.offset, offset + size));
			ps1.start(name, ps0.offset, entrySize);
			ps1.put("name", T_STR, 32, null);
			ps1.put(null, T_UNK, 4, null);
			ps1.put(null, T_UNK, 8, null);
			Symbol.reference(name, file, ps1.offset);
			ps1.put("marker", T_HASHREF, 4, null);
			ps1.put(null, T_UNK, 16, null);
			ps1.finish();
			ps0.add(ps1);
		}
		ps0.finish();
		break;
	}
	case 0x34A17: {
		int entrySize = 0x14;
		int numEntries = size / entrySize;
		ps0.start(format("sms data (%d entries)", numEntries), offset, size);
		for (; numEntries > 0; numEntries--) {
			int type = i8(data, ps0.offset + 2);
			String smsname = CareerStringPool.get(i16(data, ps0.offset));
			String name = format("sms (type %d): %s", type, smsname);
			ps1.start(name, ps0.offset, entrySize);
			ps1.put("name", T_CAREERSTRPOOLREF, 2, null);
			ps1.put("type", T_ENUM, 1, E_SMS_TYPE);
			ps1.put("mailbox", T_ENUM, 1, E_MAILBOX);
			ps1.put(null, T_UNK, 1, null);
			ps1.put(null, T_UNK, 3, null);
			switch (type) {
			case 1:
				Symbol.reference(name, file, ps1.offset);
				ps1.put("(type specific data): race", T_HASHREF, 4, null);
				break;
			case 9:
				ps1.put("(type specific data): stage index", T_INT, 2, null);
				ps1.put("(type specific data): unk", T_UNK, 2, null);
				break;
			case 0xA:
				ps1.put("(type specific data): stage index", T_INT, 2, null);
				ps1.put("(type specific data): unk", T_UNK, 2, null);
				break;
			case 0xC:
				ps1.put("(type specific data): stage index", T_INT, 2, null);
				ps1.put("(type specific data): unk", T_UNK, 2, null);
				break;
			case 0xF:
				Symbol.reference(name, file, ps1.offset);
				ps1.put("(type specific data): marker", T_HASHREF, 4, null);
				break;
			default:
				ps1.put("(type specific data): unknown", T_UNK, 4, null);
				break;
			}
			ps1.put("money reward", T_INT, 4, null);
			String sender = LanguageStringPool.get(i32(data, ps1.offset));
			ps1.put(format("sender (resolved to %s)", sender), T_INT, 4, null);
			{
				if (smsname.startsWith("SMS_CAR_UNLOCK_")) {
					// game does this too
					smsname = "SMS_CAR_UNLOCK_1";
				}
				ps2.start("sms subject/body", ps1.offset, 0);
				String subjkey = format("%s_SUBJECT", smsname);
				String bodykey = format("%s_BODY", smsname);
				String subject = LanguageStringPool.get(cihash(subjkey));
				String body = LanguageStringPool.get(cihash(bodykey));
				ps2.put(subjkey, T_LITSTR, 0, subject);
				ps2.put(bodykey, T_LITSTR, 0, body);
				ps2.finish();
				ps1.add(ps2);
			}
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
			Symbol.reference(name, file, ps1.offset);
			ps1.put("sponsor 1", T_HASHREF, 4, null);
			Symbol.reference(name, file, ps1.offset);
			ps1.put("sponsor 2", T_HASHREF, 4, null);
			Symbol.reference(name, file, ps1.offset);
			ps1.put("sponsor 3", T_HASHREF, 4, null);
			Symbol.reference(name, file, ps1.offset);
			ps1.put("sponsor 4", T_HASHREF, 4, null);
			Symbol.reference(name, file, ps1.offset);
			ps1.put("sponsor 5", T_HASHREF, 4, null);
			ps1.put(null, T_UNK, 10, null);
			ps1.put(null, T_PADDING, 2, null);
			Symbol.reference(name, file, ps1.offset);
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
			int hash = i32(data, ps0.offset + 8);
			String name = format("career sponsor %8Xh %s (reqavgrep %s)", hash, sponsorname, reqavgrep);
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
	case 0x34A1A: {
		int entrySize = 0x18;
		int numEntries = size / entrySize;
		ps0.start(format("34A1A (%d)", numEntries), offset, size);
		for (; numEntries > 0; numEntries--) {
			ps1.start("entry", ps0.offset, entrySize);
			ps1.put(null, T_UNK, entrySize, null);
			ps1.finish();
			ps0.add(ps1);
		}
		ps0.finish();
		break;
	}
	case 0x34A1B: {
		int entrySize = 0x18;
		int numEntries = size / entrySize;
		ps0.start(format("34A1B (%d)", numEntries), offset, size);
		for (; numEntries > 0; numEntries--) {
			String name = format("entry %s", CareerStringPool.get(i16(data, ps0.offset + 2)));
			ps1.start(name, ps0.offset, entrySize);
			ps1.put(null, T_UNK, 2, null);
			ps1.put(null, T_CAREERSTRPOOLREF, 2, null);
			ps1.put(null, T_UNK, 0x14, null);
			ps1.finish();
			ps0.add(ps1);
		}
		ps0.finish();
		break;
	}
	case 0x34A1C: {
		int entrySize = 0x28;
		int numEntries = size / entrySize;
		ps0.start(format("34A1C (%d)", numEntries), offset, size);
		for (; numEntries > 0; numEntries--) {
			ps1.start("entry", ps0.offset, entrySize);
			ps1.put(null, T_UNK, entrySize, null);
			ps1.finish();
			ps0.add(ps1);
		}
		ps0.finish();
		break;
	}
	case 0x34A1E: {
		int entrySize = 0xC;
		int numEntries = size / entrySize;
		ps0.start(format("34A1E (%d)", numEntries), offset, size);
		for (; numEntries > 0; numEntries--) {
			ps1.start("entry", ps0.offset, entrySize);
			ps1.put(null, T_UNK, entrySize, null);
			ps1.finish();
			ps0.add(ps1);
		}
		ps0.finish();
		break;
	}
	case 0x34A1F: {
		int entrySize = 0xC;
		int numEntries = size / entrySize;
		ps0.start(format("car unlock entries (%d)", numEntries), offset, size);
		for (; numEntries > 0; numEntries--) {
			String car = Common.carNameByHash(i32(data, ps1.offset));
			String name = format("car unlock entry %s", car);
			ps1.start("entry", ps0.offset, entrySize);
			ps1.put(format("car name hash (resolves to %s)", car), T_INT, 4, null);
			Symbol.reference(name, file, ps1.offset);
			ps1.put("unlocking race (US region)", T_HASHREF, 4, null);
			Symbol.reference(name, file, ps1.offset);
			ps1.put("unlocking race (EU region)", T_HASHREF, 4, null);
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
	case 0x39000: {
		ps0.start("language data", offset, size);

		ps1.start("header", ps0.offset, 4 * 4);
		ps1.put("char conversion table offset", T_INT, 4, null);
		int numStrings = i32(data, ps1.offset);
		ps1.put("number of strings", T_INT, 4, null);
		int tableOffset = offset + i32(data, ps1.offset);
		ps1.put("string table offset", T_INT, 4, null);
		int strBlock = offset + i32(data, ps1.offset);
		ps1.put("strings offset", T_INT, 4, null);
		ps1.finish();
		ps0.add(ps1);

		int charConversionEntries = i32(data, ps0.offset);
		ps1.start("char conversion table", ps0.offset, 4 + 2 * charConversionEntries);
		ps1.put("numEntries", T_INT, 4, null);
		ps1.put("table", T_UNK, 2 * charConversionEntries, null);
		ps1.finish();
		ps0.add(ps1);

		// remove this when the previous block is figured out because this shouldn't be here
		int pad = tableOffset - ps0.offset;
		ps1.start("pad", ps0.offset, pad);
		ps1.put(null, T_UNK, pad, null);
		ps1.finish();
		ps0.add(ps1);

		ps1.start("string table", ps0.offset, 8 * numStrings);
		for (int i = 0; i < numStrings; i++) {
			int hash = i32(data, ps1.offset);
			int stroff = i32(data, ps1.offset + 4);
			LanguageStringPool.put(hash, cstr(data, strBlock + stroff, offset + size));
			ps1.put("hash", T_INT, 4, null);
			ps1.put("offset in strings block", T_INT, 4, null);
		}
		ps1.finish();
		ps0.add(ps1);

		ps1.start("strings block", ps0.offset, ps0.sizeLeft);
		ps1.put(null, T_UNK, ps1.sizeLeft, null);
		ps1.finish();
		ps0.add(ps1);
		ps0.finish();
		break;
	}
	case 0xE34009:
	case 0xE34010: {
		ps0.start(null, offset, size);
		ps0.put("preamble?", T_PADDING, 8, null);
		ELFData.readIntoParseState(ps1, data, ps0.offset, ps0.sizeLeft);
		ps0.add(ps1);
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
/** absolute offset in data where this instance starts */
int startOffset;
/** absolute offset in data where the next field would start */
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

class ELFData
{
// I probably don't even need all this, but this was fun to make
static void readIntoParseState(ParseState ps, byte[] data, int offset, int sizeLeft)
{
	ParseState psA = new ParseState(), psB = new ParseState();

	ps.start("ELF binary", offset, sizeLeft);
	ps.put("e_ident[EI_MAG0-3]", T_INT, 4, null);
	ps.put("e_ident[EI_CLASS]", T_INT, 1, null);
	ps.put("e_ident[EI_DATA]", T_INT, 1, null);
	ps.put("e_ident[EI_VERSION]", T_INT, 1, null);
	ps.put("e_ident[EI_OSABI]", T_INT, 1, null);
	ps.put("e_ident[EI_ABIVERSION]", T_INT, 1, null);
	ps.put("e_ident[EI_PAD]", T_PADDING, 7, null);
	ps.put("e_type", T_INT, 2, null);
	ps.put("e_machine", T_INT, 2, null);
	ps.put("e_version", T_INT, 4, null);
	ps.put("e_entry", T_INT, 4, null);
	ps.put("e_phoff", T_INT, 4, null);
	int shoff = i32(data, ps.offset);
	ps.put("e_shoff", T_INT, 4, null);
	ps.put("e_flags", T_INT, 4, null);
	ps.put("e_ehsize", T_INT, 2, null);
	ps.put("e_phentsize", T_INT, 2, null);
	ps.put("e_phnum", T_INT, 2, null);
	ps.put("e_shentsize", T_INT, 2, null);
	int shnum = i16(data, ps.offset);
	ps.put("e_shnum", T_INT, 2, null);
	ps.put("e_shstrndx", T_INT, 2, null);
	// there never seem to be no program headers only section headers, so skip program headers.

	// the section header is at the bottom, so skip ahead so we know what sections we're going through
	// (we also first need to find the shstring section anyways so we have the section names)
	// (also find string section for other data while we're at it)
	// k=abs_dataoff v=abs_sectionoff
	HashMap<Integer, Integer> sections = new HashMap<>();
	int shstroffs = 0, stroffs = 0;
	{
		int offs = offset + shoff;
		for (int i = 0; i < shnum; i++) {
			int abs_sh_offset = offset + i32(data, offs + 0x10 /*sh_offset*/);
			// note: sometimes there are multiple string tables, but we can't
			//       know which one is .shstrtab.... because we'd need the name
			//       but that name is also just set in the .shstrtab..... grrr
			//       just assume the first one is the .shstrtab
			if (i32(data, offs + 4 /*sh_type*/) == 3 /*SHT_STRTAB*/) {
				if (shstroffs == 0) {
					shstroffs = abs_sh_offset;
				} else if (stroffs == 0) {
					stroffs = abs_sh_offset;
				}
			}
			sections.put(abs_sh_offset, offs);
			offs += 0x28; /*section header size*/
		}
	}

	// fields for each section
	Set<Integer> addrs = sections.keySet();
	while (!addrs.isEmpty()) {
		int dataoff = Integer.MAX_VALUE;
		for (int a : addrs) {
			dataoff = Math.min(dataoff, a);
		}
		int hoff = sections.remove(dataoff);
		if (ps.offset < dataoff) {
			ps.put(null, T_PADDING, dataoff - ps.offset, null);
		}
		String name = null;
		if (shstroffs > 0) {
			int sh_name = i32(data, hoff + 0 /*sh_name*/);
			name = cstr(data, shstroffs + sh_name, offset + sizeLeft);
		}
		int sh_type = i32(data, hoff + 0x4 /*sh_type*/);
		int sh_size = i32(data, hoff + 0x14 /*sh_size*/);
		int sh_entsize = i32(data, hoff + 0x24 /*sh_entsize*/);
		if (sh_type == 2 /*SHT_SYMTAB*/ && sh_entsize != 0 && sh_size != 0) {
			// structures symtab section
			psA.start(format("section %s", name), ps.offset, sh_size);
			for (int i = 0; i < sh_size / sh_entsize; i++) {
				String symname = null;
				if (stroffs > 0) {
					int st_name = i32(data, psA.offset);
					symname = cstr(data, stroffs + st_name, offset + sizeLeft);
				}
				psB.start(format("symbol %s", symname), psA.offset, sh_entsize);
				psB.put("st_name", T_INT, 4, null);
				psB.put("st_value", T_INT, 4, null);
				psB.put("st_size", T_INT, 4, null);
				psB.put("st_info", T_INT, 1, null);
				psB.put("st_other", T_INT, 1, null);
				psB.put("st_shndx", T_INT, 2, null);
				psB.finish();
				psA.add(psB);
			}
			psA.finish();
			ps.add(psA);
		} else {
			ps.put(format("section %s", name), T_UNK, sh_size, null);
		}
	}

	// and the section header now
	if (ps.offset - ps.startOffset < shoff) {
		ps.put(null, T_PADDING, shoff - (ps.offset - ps.startOffset), null);
	}
	psA.start("section header table", ps.offset, shnum * 0x28);
	for (int i = 0; i < shnum; i++) {
		String name = null;
		if (shstroffs > 0) {
			int sh_name = i32(data, psA.offset);
			name = cstr(data, shstroffs + sh_name, offset + sizeLeft);
		}
		psB.start(format("section header (%s)", name), psA.offset, 0x28);
		psB.put("sh_name", T_INT, 4, name);
		psB.put("sh_type", T_INT, 4, null);
		psB.put("sh_flags", T_INT, 4, null);
		psB.put("sh_addr", T_INT, 4, null);
		psB.put("sh_offset", T_INT, 4, null);
		psB.put("sh_size", T_INT, 4, null);
		psB.put("sh_link", T_INT, 4, null);
		psB.put("sh_info", T_INT, 4, null);
		psB.put("sh_addralign", T_INT, 4, null);
		psB.put("sh_entsize", T_INT, 4, null);
		psB.finish();
		psA.add(psB);
	}
	psA.finish();
	ps.add(psA);

	ps.put(null, T_PADDING, ps.sizeLeft, null);
	ps.finish();
}
} /*ELFData*/
