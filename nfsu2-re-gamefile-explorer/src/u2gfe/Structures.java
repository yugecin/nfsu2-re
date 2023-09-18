package u2gfe;

import static u2gfe.Util.*;

import java.util.ArrayList;
import java.util.HashMap;

/**
 * structures are Object[] with entries:
 * - name (string, nullable)
 * - field, repeated x times:
 * 	- name (string, nullable)
 * 	- type_size (int, msbyte is a T_* constant, three lowest bytes are size of the field or other data)
 *
 * for all types (T_*), only the most significant byte is used as type id.
 */
class Structures
{
static final int T_UNK = 0x1000000;
static final int T_STR = 0x2000000;
static final int T_I32 = 0x3000000;
static final int T_I16 = 0x4000000;
static final int T_I8 = 0x5000000;
static final int T_F32 = 0x6000000;
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

static HashMap<Integer, Object[]> structures;

static final Object MARK_STRUCT_START = new Object();
static final Object MARK_STRUCT_END = new Object();

static final int PARSE_RESULT_OK = 1;
static final int PARSE_RESULT_UNEXPECTED_END = 2;
static final int PARSE_RESULT_EXTRA_DATA = 3;

/**
 * offset and size should not include header!
 *
 * @return Object[] with entries:
 *         - MARK_STRUCT_START
 *         - (actual size of struct instance)
 *         - name (string, nullable)
 *         - field, repeated x times:
 *            either again MARK_STRUCT_START which starts a nested entry, or:
 *              - name (string, nullable)
 *              - type (int, one of the (non complex) T_* constants)
 *              - start (int, start offset in data)
 *              - end (int, end offset in data)
 *         - MARK_STRUCT_END
 *         - parse result status (int, PARSE_RESULT_* constants)
 */
static ArrayList<Object> parse(BinFile file, int magic, byte data[], int offset, int size)
{
	Object[] struct = structures.get(magic);
	if (struct == null) {
		String name = magic == 0 ? "padding" : null;
		ArrayList<Object> result = new ArrayList<>(10);
		result.add(MARK_STRUCT_START);
		result.add(size);
		result.add(name);
		result.add(null);
		result.add(T_UNK);
		result.add(offset);
		result.add(offset + size);
		result.add(MARK_STRUCT_END);
		result.add(PARSE_RESULT_OK);
		return result;
	}
	ArrayList<Object> result = new ArrayList<>(struct.length * 2);

	result.add(MARK_STRUCT_START);
	result.add(size);
	result.add(struct[0]); // name
	int fieldoffset = 0;
	for (int i = 1; i < struct.length; i += 2) {
		int typeinfo = (int) struct[i + 1];
		int fieldsize = typeinfo & 0xffffff;
		int fieldtype = typeinfo & 0xff000000;
		int arr_entrymagic = 0;
		int arr_numentries = 0;
		int arr_entrysize = 0;
		switch (fieldtype) {
		case T_ARR_REPEATING:
			arr_entrymagic = fieldsize;
			arr_entrysize = struct_size(structures.get(arr_entrymagic));
			arr_numentries = (size - fieldoffset) / arr_entrysize;
			fieldsize = 0; // for the check below
			break;
		case T_ARR:
			arr_entrymagic = (fieldsize >>> 16) & 0xFF;
			arr_numentries = fieldsize & 0xFFFF;
			arr_entrysize = struct_size(structures.get(arr_entrymagic));
			fieldsize = 0; // for the check below
			break;
		}
		if (fieldoffset + fieldsize > size &&
			fieldtype != T_ARR_REPEATING && fieldtype != T_ARR)
		{
			result.add("unexpected end of data");
			result.add(T_UNK);
			result.add(offset + fieldoffset);
			result.add(offset + fieldoffset);
			result.add(MARK_STRUCT_END);
			result.add(PARSE_RESULT_UNEXPECTED_END);
			return result;
		}
		switch (fieldtype) {
		case T_HASH:
			fieldsize = 4;
			int h = i32(data, offset + fieldoffset);
			Symbol.put(h, file, offset + fieldoffset, (String) struct[0]);
		case T_CAREERSTRPOOL:
			if (fieldtype == T_CAREERSTRPOOL) { // if only Java had goto statements...
				fieldsize = size;
				CareerStringPool.put(file, data, offset + fieldoffset, fieldsize);
			}
		case T_UNK:
		case T_STR:
		case T_I32:
		case T_I16:
		case T_I8:
		case T_F32:
		case T_PADDING:
		case T_HASHREF:
		case T_OBJLINK:
		case T_CAREERSTRPOOLREF:
			result.add(struct[i]);
			result.add(fieldtype);
			result.add(offset + fieldoffset);
			result.add(offset + fieldoffset + fieldsize);
			break;
		case T_ARR:
		case T_ARR_REPEATING:
			for (; arr_numentries > 0; arr_numentries--) {
				int offs = offset + fieldoffset;
				result.addAll(parse(file, arr_entrymagic, data, offs, arr_entrysize));
				fieldoffset += arr_entrysize;
			}
			continue;
		default:
			throw new RuntimeException(String.format("unk type: %X", fieldtype));
		}
		fieldoffset += fieldsize;
	}

	if (fieldoffset == size) {
		result.add(MARK_STRUCT_END);
		result.add(PARSE_RESULT_OK);
	} else {
		result.add("unexpected extra data");
		result.add(T_UNK);
		result.add(offset + fieldoffset);
		result.add(offset + size);
		result.add(MARK_STRUCT_END);
		result.add(PARSE_RESULT_EXTRA_DATA);
	}

	return result;
}

static ArrayList<Throwable> get_parse_result_errors(Object fields[])
{
	ArrayList<Throwable> errors = new ArrayList<>();

	for (int i = 0; i < fields.length; i++) {
		Object entry = fields[i];
		if (entry == MARK_STRUCT_START) {
			i += 2;
		} else if (entry == MARK_STRUCT_END) {
			int result = (int) fields[i + 1];
			if (result == Structures.PARSE_RESULT_EXTRA_DATA) {
				int from = (int) fields[i - 2];
				int extrasize = (int) fields[i - 1] - from;
				String msg = String.format("%Xh trailing bytes from offset %Xh", extrasize, from);
				errors.add(new Throwable(msg));
			} else if (result == Structures.PARSE_RESULT_UNEXPECTED_END) {
				errors.add(new Throwable("unexpected end"));
			}
		} else {
			i += 2;
		}
	}

	return errors;
}

static int struct_size(Object struct[])
{
	int size = 0;
	for (int i= 2; i < struct.length; i += 2) {
		int type = (int) struct[i] & 0xff000000;
		switch (type) {
		case T_ARR_REPEATING:
			throw new RuntimeException("can't get size of an arr_repeating struct");
		case T_ARR:
			int entrymagic = ((int) struct[i] >>> 16) & 0xFF;
			int numentries = (int) struct[i] & 0xFF;
			size += struct_size(structures.get(entrymagic)) * numentries;
			break;
		default:
			size += (int) struct[i] & 0xffffff;
		}
	}
	return size;
}

static void validate_size(Object struct[], int expected_size)
{
	int size = struct_size(struct);
	if (size != expected_size) {
		String msg = String.format("%s: expected size %Xh got %Xh", struct[0], expected_size, size);
		throw new RuntimeException(msg);
	}
}

static String get_name(int magic)
{
	if (magic == 0) {
		return "padding";
	}
	Object[] definition = structures.get(magic);
	return definition != null ? (String) definition[0] : null;
}

static void init()
{
	final int BYTE = T_I8 | 1, WORD = T_I16 | 2, DWORD = T_I32 | 4;
	final int HASHREF = T_HASHREF | 4, HASH = T_HASH | 4;
	structures = new HashMap<>();
	int x = 0;

	int _30220_entry = ++x;
	structures.put(_30220_entry, new Object[] {
		"car preset",
		"link", T_OBJLINK | 8,
		"modelName", T_STR | 32,
		"name", T_STR | 32,
		null, DWORD,
		null, T_UNK | 680,
		null, T_UNK | 68,
	});
	validate_size(structures.get(_30220_entry), 0x338);
	structures.put(0x30220, new Object[] {
		"car presets",
		"entries", T_ARR_REPEATING | _30220_entry,
	});

	int _34A11_18 = ++x;
	structures.put(_34A11_18, new Object[] {
		"career race field 18",
		null, WORD,
		null, BYTE,
		null, BYTE,
	});
	int _34A11_38 = ++x;
	structures.put(_34A11_38, new Object[] {
		"career race field 38",
		null, BYTE,
		null, T_PADDING | 3,
		null, HASHREF,
	});
	int _34A11_entry = ++x;
	structures.put(_34A11_entry, new Object[] {
		"career race",
		null, T_UNK | 4,
		"post race movie name", T_CAREERSTRPOOLREF | 2,
		null, T_PADDING | 2,
		"hash", HASH,
		null, T_UNK | 1,
		null, T_UNK | 1,
		null, T_PADDING | 1,
		"race type", BYTE, // TODO: enum types
		null, T_UNK | 4,
		null, T_UNK | 4,
		"field 18", T_ARR | arr_entrytype(_34A11_18) | arr_count(4),
		"marker", HASHREF,
		null, T_UNK | 4,
		"bank reward", T_UNK | 4,
		"bank reward type", BYTE,
		"num entries in 38", BYTE,
		null, T_PADDING | 1,
		"stage index", BYTE,
		"field 38", T_ARR | arr_entrytype(_34A11_38) | arr_count(8),
		null, T_UNK | 4,
		null, BYTE,
		null, T_PADDING | 1,
		"num entries in 18", BYTE,
		null, BYTE,
		null, T_UNK | 8,
	});
	validate_size(structures.get(_34A11_entry), 0x88);
	structures.put(0x34A11, new Object[] {
		"career races",
		"entries", T_ARR_REPEATING | _34A11_entry,
	});

	int _34A18_entry = ++x;
	structures.put(_34A18_entry, new Object[] {
		"career stage settings entry",
		"stage index", BYTE,
		"num sponsors", BYTE,
		"outrun stakes", WORD,
		"showcase reward multiplier", WORD,
		null, T_PADDING | 2,
		"sponsor 1", HASHREF,
		"sponsor 2", HASHREF,
		"sponsor 3", HASHREF,
		"sponsor 4", HASHREF,
		"sponsor 5", HASHREF,
		null, T_UNK | 10,
		null, T_PADDING | 2,
		null, HASHREF,
		null, T_UNK | 4,
		null, T_UNK | 1,
		null, T_UNK | 1,
		null, T_UNK | 1,
		null, T_UNK | 1,
		null, T_UNK | 1,
		null, T_UNK | 2,
		null, T_UNK | 1,
		null, T_UNK | 8,
		"(outrun related)", BYTE,
		null, T_UNK | 1,
		null, T_UNK | 2,
		null, T_UNK | 4,
		null, T_UNK | 4,
		null, T_UNK | 4,
	});
	validate_size(structures.get(_34A18_entry), 0x50);
	structures.put(0x34A18, new Object[] {
		"career stage settings",
		"entries", T_ARR_REPEATING | 0x2,
	});

	int _34A19_entry = ++x;
	structures.put(_34A19_entry, new Object[] {
		"career sponsor",
		"name", T_CAREERSTRPOOLREF | 2,
		"bank per race won", WORD,
		null, T_UNK | 1,
		null, T_UNK | 1,
		null, T_UNK | 1,
		null, T_UNK | 1,
		"hash", HASH,
		"signing bonus", WORD,
		"required average reputation", WORD,
	});
	validate_size(structures.get(_34A19_entry), 0x10);
	structures.put(0x34A19, new Object[] {
		"career sponsors",
		"entries", T_ARR_REPEATING | _34A19_entry,
	});

	structures.put(0x34A1D, new Object[] {
		"career string pool",
		"entries", T_CAREERSTRPOOL,
	});
}
}
