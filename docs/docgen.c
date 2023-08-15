/**
Instructs idcparse.c to parse the IDC file and then uses that to make
html documentation for the structs/enums/functions/data.
*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define IDCP_VERBOSE_LEVEL 0
#include "idcparse.c"

/*These definitions are specific to your IDA's version.
See <IDA path>/idc/idc.idc (without the IDC_ prefix in the definitions' names)*/
#define IDC_FF_STRUCT 0x60000000
#define IDC_DT_TYPE   0xF0000000
#define IDC_MS_CLS    0x00000600
#define IDC_FF_DATA   0x00000400
#define MS_0TYPE      0x00F00000
#define FF_0ENUM      0x00800000
#define IDC_is_data(F) ((F & IDC_MS_CLS) == IDC_FF_DATA)
#define IDC_is_struct(F) (IDC_is_data(F) && (F & IDC_DT_TYPE) == IDC_FF_STRUCT)
#define IDC_is_enum(F) ((F & MS_0TYPE) == FF_0ENUM)

#define IDCPATH "../../nfsu2-re-idc/SPEED2.idc"

#include "mmparse.c"

struct docgen_mmp_data {
	char *d0, *d1, *d2, *d3;
	int len0, len1, len2, len3;
};

struct docgen_symbol_comment {
	char *buf;
	int buf_len;
};

struct docgen_symbol_comments {
	struct docgen_symbol_comment normal;
	struct docgen_symbol_comment rep;
};

struct docgen_structinfo {
	struct idcp_struct *struc;
	int name_len;
	/**struct is a class when at least one func's 'methodof' refers this struct*/
	char is_class;
	struct docgen_symbol_comments comments;
};

struct docgen_funcinfo {
	struct idcp_stuff *func;
	int name_len;
	struct docgen_structinfo *methodof;
	struct docgen_symbol_comments comments;
};

struct docgen_enuminfo {
	struct idcp_enum *enu;
	int name_len;
	struct docgen_symbol_comments comments;
};

struct docgen_datainfo {
	struct idcp_stuff *data;
	struct docgen_symbol_comments comments;
	/**type info is missing in IDC (except for non-pointer struct types),
	but using '@docgen:type:xxx' in the symbol comment can be used to set
	this type*/
	char *type;
	int type_len;
};

struct docgen {
	char struct_member_needs_anchor[IDCP_MAX_TOTAL_STRUCT_MEMBERS];
	char enum_member_needs_anchor[IDCP_MAX_TOTAL_ENUM_MEMBERS];
	struct idcparse *idcp;
	int num_structinfos;
	struct docgen_structinfo *structinfos;
	int num_funcinfos;
	struct docgen_funcinfo *funcinfos;
	int num_enuminfos;
	struct docgen_enuminfo *enuminfos;
	int num_datainfos;
	struct docgen_datainfo *datainfos;
	struct docgen_symbol_comments *enum_member_comments;
	struct docgen_symbol_comments *struct_member_comments;
};

#define MMPEXTRAS_MAX_ANCHORS 100
#define MMPEXTRAS_MAX_NESTED_SECTIONS 10
#define MMPEXTRAS_MAX_NESTED_ULS 10
#include "mmparse_extras.c"

struct docgen_mmparse_userdata {
	struct docgen *dg;
	struct {
		char is_blogpost;
		/*AKA id*/
		int index;
		char *htmlfile;
		char *title;
		char *date;
	} blogpost;
	struct mmparse **blogs;
	int num_blogs;
	struct mmpextras_userdata mmpextras;
};

static
struct mmpextras_userdata *mmpextras_get_userdata(struct mmparse *mm)
{
	return &((struct docgen_mmparse_userdata*) mm->config.userdata)->mmpextras;
}
/*jeanine:p:i:39;p:57;a:r;x:8.78;y:-6.44;*/
static
int docgen_parse_addr(char *addr, int len)
{
	register char c;
	int res;

	res = 0;
	while (len--) {
		res <<= 4;
		c = *(addr++);
		if ('0' <= c && c <= '9') {
			res |= c - '0';
		} else if ('a' <= c && c <= 'f') {
			res |= c - 'a' + 10;
		} else if ('A' <= c && c <= 'F') {
			res |= c - 'A' + 10;
		} else {
			fprintf(stderr, "bad addr char '%c'\n", c);
			return -1;
		}
	}
	return res;
}
/*jeanine:p:i:63;p:57;a:r;x:8.89;*/
static
int docgen_parse_val(int *output, char *val, int len)
{
	register char c;

	*output = 0;
	if (len > 2 && val[0] == '0' && (val[1] | 0x20) == 'x') {
		val += 2;
		len -= 2;
		while (len--) {
			*output <<= 4;
			c = *(val++);
			if ('0' <= c && c <= '9') {
				*output |= c - '0';
			} else if ('a' <= c && c <= 'f') {
				*output |= c - 'a' + 10;
			} else if ('A' <= c && c <= 'F') {
				*output |= c - 'A' + 10;
			} else {
				fprintf(stderr, "bad val char '%c'\n", c);
				return 0;
			}
		}
	} else {
		while (len--) {
			*output <<= 4;
			c = *(val++);
			if ('0' <= c && c <= '9') {
				*output |= c - '0';
			} else {
				fprintf(stderr, "bad val char '%c'\n", c);
				return 0;
			}
		}
	}
	return 1;
}
/*jeanine:p:i:20;p:0;a:b;x:-113.33;y:12.10;*/
static
struct docgen_structinfo* docgen_find_struct(struct docgen *dg, char *name, int len)
{
	struct docgen_structinfo *structinfo;
	int i;

	for (i = 0, structinfo = dg->structinfos; i < dg->num_structinfos; i++, structinfo++) {
		if (len == structinfo->name_len && !strncmp(name, structinfo->struc->name, len)) {
			return structinfo;
		}
	}
	return NULL;
}

/*type may be null, may end with any amount of spaces and * symbols*/
static
struct docgen_structinfo* docgen_find_struct_from_text_type(struct docgen *dg, char *type, int len)
{
	register char c;

	if (type && !strncmp(type, "struct ", 7)) {
		type += 7;
		len -= 7;
		for (;; len--) {
			c = type[len - 1];
			if (c != '*' && c != ' ') {
				break;
			}
		}
		return docgen_find_struct(dg, type, len);
	}
	return NULL;
}

static
struct docgen_enuminfo* docgen_find_enum(struct docgen *dg, char *name, int len)
{
	struct docgen_enuminfo *enuminfo;
	int i;

	for (i = 0, enuminfo = dg->enuminfos; i < dg->num_enuminfos; i++, enuminfo++) {
		if (len == enuminfo->name_len && !strncmp(name, enuminfo->enu->name, len)) {
			return enuminfo;
		}
	}
	return NULL;
}

static
int docgen_get_struct_size(struct idcparse *idcp, struct idcp_struct *struc)
{
	struct idcp_struct_member *mem;

	if (struc->end_idx > struc->start_idx) {
		mem = idcp->struct_members + struc->end_idx - 1;
		return mem->offset + mem->nbytes;
	} else {
		return 0;
	}
}

static
void docgen_get_func_friendlyname(char *dest, char *name)
{
	char c;

	do {
		c = *(name++);
		if (c == ':' || c == '?' || c == '.' || c == '[' || c == ']') {
			c = '_';
		}
		*(dest++) = c;
	} while (c);
}

static
void docgen_readfile(char *path, char **buf, int *length)
{
	register int size;
	FILE *in;

	in = fopen(path, "rb");
	if (!in) {
		fprintf(stderr, "failed to open file %s for reading", path);
		assert(0);
	}
	fseek(in, 0l, SEEK_END);
	size = *length = ftell(in);
	rewind(in);
	assert((*buf = (char*) malloc(size)));
	fread(*buf, size, 1, in);
	fclose(in);
}
/*jeanine:p:i:57;p:56;a:r;x:3.33;*/
enum docgen_ref_element_type {
	UNRESOLVED = 0,
	FUNC,
	DATA,
	STRUCTMEMBER,
	STRUCT,
	ENUMMEMBER,
	ENUM,
};

struct docgen_ref_element {
	enum docgen_ref_element_type type;
	union {
		struct idcp_struct *struc;
		struct idcp_stuff *func;
		struct idcp_stuff *data;
		struct idcp_enum *enu;
	} el;
	union {
		struct idcp_struct_member *struc;
		struct idcp_enum_member *enu;
	} member;
	struct docgen_ref_element *next;
	char element_level;
	char is_from_pointer;
};

#define DOCGEN_MAX_REF_ELEMENTS 20

struct docgen_ref {
	struct docgen_ref_element elements[DOCGEN_MAX_REF_ELEMENTS];
};

static
struct docgen_ref_element *docgen_resolve_ref_get_next_ref_element(struct docgen_ref *result, struct docgen_ref_element *current)
{
	assert(((void)"need more DOCGEN_MAX_REF_ELEMENTS", current->element_level < DOCGEN_MAX_REF_ELEMENTS));
	current->next = result->elements + current->element_level;
	current->next->element_level = current->element_level + 1;
	return current->next;
}

/**
@param ref should be zero terminated
*/
static
void docgen_resolve_ref(struct docgen *dg, struct docgen_ref *result, char *ref, int len)
{
	int min, max, idx, current, addr, val, num_members;
	struct docgen_ref_element *ref_element;
	struct docgen_structinfo *structinfo;
	struct idcp_struct_member *strumem;
	struct docgen_datainfo *datainfo;
	struct docgen_enuminfo *enuminfo;
	struct idcp_enum_member *enumem;
	char *plus, *dot, *fslash, *tmp;
	struct idcp_struct *struc;
	struct idcp_stuff *stuff;

	memset(result, 0, sizeof(struct docgen_ref));
	ref_element = result->elements;
	ref_element->element_level = 1;

	if (len > 5 && !strncmp("enum ", ref, 5)) {
		fslash = strstr(ref + 5, "/");
		if (fslash) {
			enuminfo = docgen_find_enum(dg, ref + 5, (fslash - ref) - 5);
		} else {
			enuminfo = docgen_find_enum(dg, ref + 5, len - 5);
		}
		if (enuminfo) {
			ref_element->el.enu = enuminfo->enu;
			ref_element->type = ENUM;
			if (fslash) {
				if (docgen_parse_val(&val, fslash + 1, len - (fslash - ref) - 1)) {/*jeanine:r:i:63;*/
					num_members = enuminfo->enu->end_idx - enuminfo->enu->start_idx;
					enumem = dg->idcp->enum_members + enuminfo->enu->start_idx;
					for (; num_members; num_members--, enumem++) {
						if (enumem->value == val) {
							ref_element->member.enu = enumem;
							ref_element->type = ENUMMEMBER;
							return;
						}
					}
				}
				goto err;
			}
		}
	} else if (len > 7 && !strncmp("struct ", ref, 7)) {
		/*struct+memberoffset => [struct] [member]*/
		/*struct.memberoffset => [member]*/
		/*struct+memberoffset.memberoffset => [struct] [member] [member]*/
		/*struct.memberoffset.memberoffset => [member] [member]*/
		ref += 7;
		len -= 7;
		plus = strstr(ref, "+");
		dot = NULL;
		if (plus) {
			structinfo = docgen_find_struct(dg, ref, plus - ref);
		} else {
			dot = strstr(ref, ".");
			if (dot) {
				structinfo = docgen_find_struct(dg, ref, dot - ref);
				if (!structinfo) {
					return;
				}
				struc = structinfo->struc;
				ref_element->type = STRUCT;
				ref_element->el.struc = struc;
				ref_element = docgen_resolve_ref_get_next_ref_element(result, ref_element);
				ref_element->el.struc = struc;
				dot++;
				len -= dot - ref;
				ref = dot;
				goto finddot;
			} else {
				structinfo = docgen_find_struct(dg, ref, len);
			}
		}
		if (structinfo) {
			ref_element->el.struc = structinfo->struc;
			ref_element->type = STRUCT;
			if (plus) {
				plus++;
				len -= plus - ref;
				ref = plus;
finddot:
				dot = strstr(ref, ".");
				if (dot) {
					addr = docgen_parse_addr(ref, dot - ref);/*jeanine:s:a:r;i:39;*/
				} else {
					addr = docgen_parse_addr(ref, len);/*jeanine:s:a:r;i:39;*/
				}
				if (addr != -1) {
					struc = ref_element->el.struc;
findstrucmember:
					ref_element->type = STRUCTMEMBER;
					num_members = struc->end_idx - struc->start_idx;
					if (num_members) {
						strumem = dg->idcp->struct_members + struc->start_idx;
						for (;;) {
							if (strumem->offset == addr) {
								ref_element->member.struc = strumem;
								if (dot) {
									ref_element = docgen_resolve_ref_get_next_ref_element(result, ref_element);
									if (IDC_is_struct(strumem->flag)) {
										struc = dg->idcp->structs + strumem->typeid;
									} else if (strumem->type && (structinfo = docgen_find_struct_from_text_type(dg, strumem->type, strlen(strumem->type)))) {
										struc = structinfo->struc;
										ref_element->is_from_pointer = 1;
									} else {
										fprintf(stderr, "resolve_ref: trying to go deeper but struct member '%s' is not a struct\n", strumem->name);
										goto err;
									}
									ref_element->el.struc = struc;
									dot++;
									len -= dot - ref;
									ref = dot;
									goto finddot;
								}
								return;
							}
							num_members--;
							if (((!num_members && strumem->offset + strumem->nbytes > addr) || (strumem + 1)->offset > addr) && IDC_is_struct(strumem->flag)) {
								addr -= strumem->offset;
								ref_element->member.struc = strumem;
								ref_element = docgen_resolve_ref_get_next_ref_element(result, ref_element);
								ref_element->el.struc = struc = dg->idcp->structs + strumem->typeid;
								goto findstrucmember;
							}
							if (!num_members) {
								break;
							}
							strumem++;
						}
					}
				}
				goto err;
			}
		}
	} else {
		/*data.memberoffset => [data] [member]*/
		/*data.memberoffset.memberoffset => [data] [member] [member]*/

		/*Binary search will work as long as the 'new_addr > addr'
		assertion in 'idcp_get_or_allocate_stuff' still stands.*/
		dot = strstr(ref, ".");
		if (dot) {
			addr = docgen_parse_addr(ref, dot - ref);/*jeanine:r:i:39;*/
		} else {
			addr = docgen_parse_addr(ref, len);/*jeanine:s:a:r;i:39;*/
		}
		if (addr == -1) {
			return;
		}
		min = 0;
		max = dg->idcp->num_stuffs - 1;
		for (;;) {
			idx = min + (max - min) / 2;
			current = dg->idcp->stuffs[idx].addr;
			if (current == addr) {
				stuff = dg->idcp->stuffs + idx;
				if (!stuff->name) {
					fprintf(stderr, "resolved ref '%s' has no name, ignoring\n", ref);
				} else if (stuff->type == IDCP_STUFF_TYPE_FUNC) {
					ref_element->type = FUNC;
					ref_element->el.func = stuff;
					if (dot) {
						fprintf(stderr, "ref has a dot but it resolved to a function");
						goto err;
					}
				} else if (stuff->type == IDCP_STUFF_TYPE_DATA) {
					ref_element->type = DATA;
					ref_element->el.data = stuff;
					if (dot) {
						if (stuff->data.data.struct_type) {
							tmp = stuff->data.data.struct_type;
							if ((structinfo = docgen_find_struct(dg, tmp, strlen(tmp)))) {
								ref_element = docgen_resolve_ref_get_next_ref_element(result, ref_element);
								goto data_have_structinfo;
							}
							fprintf(stderr, "referenced data struct type is not found: %s\n", tmp);
							goto err;
						}
						/*need to find datainfo.... yay*/
						min = 0;
						max = dg->num_datainfos - 1;
						for (;;) {
							idx = min + (max - min) / 2;
							datainfo = dg->datainfos + idx;
							if (datainfo->data == stuff) {
								if (!datainfo->type) {
									fprintf(stderr, "referenced data has dot but has no type\n");
								} else if (!(structinfo = docgen_find_struct_from_text_type(dg, datainfo->type, datainfo->type_len))) {
									fprintf(stderr, "failed to find referenced data's type: %s\n", datainfo->type);
								} else {
									ref_element = docgen_resolve_ref_get_next_ref_element(result, ref_element);
									ref_element->is_from_pointer = 1;
data_have_structinfo:
									ref_element->el.struc = struc = structinfo->struc;
									dot++;
									len -= dot - ref;
									ref = dot;
									goto finddot;
								}
								goto err;
							}
							if (max <= min) {
								fprintf(stderr, "failed to find datainfo for data %X %s\n", stuff->addr, stuff->name);
								goto err;
							} else if (datainfo->data->addr > stuff->addr) {
								max = idx - 1;
							} else if (datainfo->data->addr < stuff->addr) {
								min = idx + 1;
							}
						}
					}
				} else {
					fprintf(stderr, "unknown type '%d' when resolving ref '%s'\n", stuff->type, ref);
				}
				return;
			} else {
				if (max <= min) {
					return;
				}
				if (current > addr) {
					max = idx - 1;
				} else if (current < addr) {
					min = idx + 1;
				}
			}
		}
	}
	return;
err:
	ref_element = result->elements;
	fprintf(stderr, "have this data before erroring:\n");
	while (ref_element && ref_element->type) {
		switch (ref_element->type) {
		case UNRESOLVED:
			fprintf(stderr, "  (unresolved)\n");
			break;
		case FUNC:
			fprintf(stderr, "  func %s\n", ref_element->el.func->name);
			break;
		case DATA:
			fprintf(stderr, "  data %s\n", ref_element->el.data->name);
			break;
		case STRUCTMEMBER:
			if (ref_element->member.struc) {
				fprintf(stderr, "  struct %s member %s\n", ref_element->el.struc->name, ref_element->member.struc->name);
			} else {
				fprintf(stderr, "  struct %s member (unresolved)\n", ref_element->el.struc->name);
			}
			break;
		case STRUCT:
			fprintf(stderr, "  struct %s\n", ref_element->el.struc->name);
			break;
		case ENUMMEMBER:
			if (ref_element->member.enu) {
				fprintf(stderr, "  enum %s member %s\n", ref_element->el.enu->name, ref_element->member.enu->name);
			} else {
				fprintf(stderr, "  enum %s member (unresolved)\n", ref_element->el.enu->name);
			}
			break;
		case ENUM:
			fprintf(stderr, "  enum %s\n", ref_element->el.enu->name);
			break;
		}
		ref_element = ref_element->next;
	}
	result->elements[0].type = UNRESOLVED;
}
/*jeanine:p:i:14;p:0;a:b;y:12.13;*/
struct docgen_tmpbuf {
	char used;
	char *data;
	int size;
};

/**
This will come bite me in my ass at some point.
*/
static
struct docgen_tmpbuf* docgen_get_tmpbuf(int minsize)
{
#define DOCGEN_MAX_TMPBUFS 20
	static struct docgen_tmpbuf bufs[DOCGEN_MAX_TMPBUFS];

	struct docgen_tmpbuf *b;
	int i;

	if (minsize < 20000) {
		minsize = 20000;
	}
	/*First try to find an already allocated one.*/
	for (i = 0, b = bufs; i < DOCGEN_MAX_TMPBUFS; i++, b++) {
		if (!b->used && b->size >= minsize) {
			b->used = 1;
			b->data[0] = 0;
			return b;
		}
	}
	/*If not, allocate a free one*/
	for (i = 0, b = bufs; i < DOCGEN_MAX_TMPBUFS; i++, b++) {
		if (!b->used && !b->size) {
			b->used = 1;
			b->size = minsize;
			b->data = malloc(minsize);
			assert(b->data);
			b->data[0] = 0;
			return b;
		}
	}
	/*Realloc a free one*/
	for (i = 0, b = bufs; i < DOCGEN_MAX_TMPBUFS; i++, b++) {
		if (!b->used) {
			b->used = 1;
			b->size = minsize;
			b->data = realloc(b->data, minsize);
			assert(b->data);
			b->data[0] = 0;
			return b;
		}
	}
	assert(((void) "failed to allocate tmpbuf, try increasing DOCGEN_MAX_TMPBUFS", 0));
	return NULL; /*make gcc -Wall happy*/
}

static
void docgen_free_tmpbuf(struct docgen_tmpbuf *buf)
{
	buf->used = 0;
}
/*jeanine:p:i:12;p:11;a:r;x:54.67;y:-130.75;*/
/**
@param tmpbuf might get swapped with a different buffer
@return nonzero if there were unknown structs/enums
*/
static
int docgen_link_structs_enums(struct docgen *dg, struct docgen_tmpbuf **tmpbuf)
{
	char *bufp, *sub, *end, *strp, c, name[200], *n, hasunknown;
	struct docgen_tmpbuf *newbuf, *original;

	hasunknown = 0;
	/*structs*/
	original = *tmpbuf;
	sub = strstr(original->data, "struct ");
	if (sub) {
		newbuf = docgen_get_tmpbuf(original->size);
		bufp = newbuf->data;
		strp = original->data;
		do {
			/*Copy segment before struct*/
			memcpy(bufp, strp, sub - strp);
			bufp += sub - strp;
			/*Get struct name*/
			end = sub + 6;
			n = name - 1;
			do {
				end++; n++;
				*n = c = *end;
			} while (c != ' ' && c != ')' && c != '*' && c != '[');
			*n = 0;
			if (name[0] != '#' && docgen_find_struct(dg, name, strlen(name))) {
				bufp += sprintf(bufp, "<a href=structs.html#struc_%s>struct %s</a>", name, name);
			} else {
				hasunknown = 1;
				bufp += sprintf(bufp, "<strong>struct %s</strong>", name);
			}
			strp = end;
			sub = strstr(strp, "struct ");
		} while (sub);
		*bufp = 0;
		strcat(bufp, strp);
		*tmpbuf = newbuf;
		docgen_free_tmpbuf(original);
	}
	/*enums*/
	original = *tmpbuf;
	sub = strstr(original->data, "enum ");
	if (sub) {
		newbuf = docgen_get_tmpbuf(original->size);
		bufp = newbuf->data;
		strp = original->data;
		do {
			/*Copy segment before struct*/
			memcpy(bufp, strp, sub - strp);
			bufp += sub - strp;
			/*Get struct name*/
			end = sub + 4;
			n = name - 1;
			do {
				end++; n++;
				*n = c = *end;
			} while (c != ' ' && c != ')' && c != '*' && c != '[');
			*n = 0;
			if (docgen_find_enum(dg, name, strlen(name))) {
				bufp += sprintf(bufp, "<a href=enums.html#enu_%s>enum %s</a>", name, name);
			} else {
				hasunknown = 1;
				bufp += sprintf(bufp, "<strong>enum %s</strong>", name);
			}
			strp = end;
			sub = strstr(strp, "enum ");
		} while (sub);
		*bufp = 0;
		strcat(bufp, strp);
		*tmpbuf = newbuf;
		docgen_free_tmpbuf(original);
		return hasunknown;
	}
	return hasunknown;
}
/*jeanine:p:i:24;p:90;a:r;x:7.78;y:-47.00;*/
static
void docgen_parseidc(struct idcparse *idcp)
{
	char *buf;
	int length;

	docgen_readfile(IDCPATH, &buf, &length);
	if (length > 2 && buf[0] == (char) 0xEF) {
		/*assume UTF8 BOM*/
		idcparse(idcp, buf + 3, length - 3);
	} else {
		idcparse(idcp, buf, length);
	}
}
/*jeanine:p:i:19;p:17;a:r;x:121.55;y:-45.22;*/
/**
@param tmpbuf might get swapped with a different buffer
*/
static
void docgen_gen_func_signature(struct docgen_tmpbuf **signaturebuf, struct docgen *dg, struct docgen_funcinfo *funcinfo, struct idcp_stuff *func)
{
	char *originalname, friendlyname[200], *namepos, *coloncolon, *b, *originalsignature, classname[200], *thiscall;
	struct docgen_tmpbuf *newbuf, *checkbuf;
	int len, classname_len;

	originalname = func->name;
	if (func->data.func.type) {
		originalsignature = func->data.func.type;
		/*Function names like 'clazz::func?' get 'friendly' names like 'clazz__func_' in their signatures.*/
		docgen_get_func_friendlyname(friendlyname, originalname);
		namepos = strstr(originalsignature, friendlyname);
		if (!namepos) {
			fprintf(stderr, "cannot find func friendlyname '%s' (original '%s') in signature '%s'\n",
				friendlyname,
				originalname,
				originalsignature
			);
			assert(0);
		}
	} else {
		originalsignature = func->name;
		strcpy(friendlyname, func->name);
		namepos = originalsignature;
	}

	newbuf = docgen_get_tmpbuf((*signaturebuf)->size);
	b = newbuf->data;
	/*Copy part until start of name*/
	len = namepos - originalsignature;
	memcpy(b, originalsignature, len);
	b += len;
	/*Copy and format name*/
	b += sprintf(b, "<h3>");
	coloncolon = strstr(originalname, "::");
	if (coloncolon) {
		/*Find the struct class and link it*/
		classname_len = coloncolon - originalname;
		memcpy(classname, originalname, classname_len);
		classname[classname_len] = 0;
		if ((funcinfo->methodof = docgen_find_struct(dg, classname, classname_len))) {
			funcinfo->methodof->is_class = 1;
			b += sprintf(b, "<a href=structs.html#struc_%s>%s</a>", classname, classname);
			/*Check that the 'this' argument is of the expected type.*/
			if (func->data.func.type) {
				checkbuf = docgen_get_tmpbuf(10000);
				sprintf(checkbuf->data, "%s(struct %s *this", friendlyname, classname);
				if (!strstr(originalsignature, checkbuf->data)) {
					fprintf(stderr, "warn: func %X '%s' has wrong thisarg (searching '%s')\n", func->addr, originalname, checkbuf->data);
				}
				docgen_free_tmpbuf(checkbuf);
			}
		} else {
			printf("warn: cannot find struct '%s' for func %X '%s'\n", classname, func->addr, originalname);
			b += sprintf(b, "<strong>%s</strong>", classname);
		}
		b += sprintf(b, "%s", coloncolon);
	} else {
		b += sprintf(b, "%s", originalname);
		/*Quickly check if there should have been a coloncolon (suppressed by starting func name with '[')*/
		if (func->name[0] != '[') {
			thiscall = strstr(originalsignature, "__thiscall");
			if (thiscall && thiscall - originalsignature < len) {
				fprintf(stderr, "warn: func %X is __thiscall but no '::'\n", func->addr);
			}
		}
	}
	b += sprintf(b, "</h3>");
	/*Copy rest of signature*/
	strcpy(b, namepos + funcinfo->name_len);
	docgen_free_tmpbuf(*signaturebuf);
	*signaturebuf = newbuf;

	if (docgen_link_structs_enums(dg, signaturebuf)) {/*jeanine:s:a:r;i:12;*/
		fprintf(stderr, "warn: func '%X %s' references an unknown struct/enum\n", func->addr, func->name);
	}
}
/*jeanine:p:i:69;p:17;a:r;x:32.02;y:70.82;*/
static
void docgen_symbol_print_comments_block(FILE *f, struct docgen_symbol_comments *comments)
{
	if (comments->normal.buf) {
		fwrite("\n<div class=mm>", 15, 1, f);
		fwrite(comments->normal.buf, comments->normal.buf_len, 1, f);
		fwrite("</div>", 6, 1, f);
	}
	if (comments->rep.buf) {
		fwrite("\n<div class=mm>", 15, 1, f);
		fwrite(comments->rep.buf, comments->rep.buf_len, 1, f);
		fwrite("</div>", 6, 1, f);
	}
}
/*jeanine:p:i:71;p:22;a:r;x:330.56;*/
static
void docgen_symbol_print_comments_inline(FILE *f, struct docgen_symbol_comments *comments)
{
	if (comments->normal.buf || comments->rep.buf) {
		fprintf(f, "<span class='mm'>/""**");
		if (comments->normal.buf) {
			if (comments->normal.buf[comments->normal.buf_len - 1] == '\n') {
				comments->normal.buf_len--;
			}
			fwrite(comments->normal.buf, comments->normal.buf_len, 1, f);
		}
		if (comments->rep.buf) {
			if (comments->normal.buf) {
				fprintf(f, "\n\n");
			}
			if (comments->rep.buf[comments->rep.buf_len - 1] == '\n') {
				comments->rep.buf_len--;
			}
			fwrite(comments->rep.buf, comments->rep.buf_len, 1, f);
		}
		fprintf(f, "*/</span>\n");
	}
}
/*jeanine:p:i:17;p:88;a:r;x:35.22;*/
static
void docgen_print_func(FILE *f, struct docgen *dg, struct docgen_funcinfo *funcinfo, struct idcp_stuff *func)
{
	struct docgen_tmpbuf *signaturebuf;

	fprintf(f, "\n<a id=%X></a>", func->addr);
	docgen_symbol_print_comments_block(f, &funcinfo->comments);/*jeanine:r:i:69;*/
	signaturebuf = docgen_get_tmpbuf(10000);
	docgen_gen_func_signature(&signaturebuf, dg, funcinfo, func);/*jeanine:r:i:19;*/
	/*these are 'div' instead of 'pre' because 'h3' can't be inside 'pre'*/
	fprintf(f, "\n<div><i>%X</i> %s</div>", func->addr, signaturebuf->data);
	docgen_free_tmpbuf(signaturebuf);
}
/*jeanine:p:i:9;p:11;a:r;x:58.67;y:-38.50;*/
static
void docgen_format_struct_member_typeandname_when_enum(char *buf, struct idcparse *idcp, struct idcp_struct_member *mem)
{
	/*TODO: what if it's an array*/

	struct idcp_enum *en;

	assert(0 <= mem->typeid && mem->typeid < idcp->num_enums);
	en = idcp->enums + mem->typeid;
	buf += sprintf(buf, "<a href=enums.html#enu_%s>enum %s</a> %s", en->name, en->name, mem->name);
}
/*jeanine:p:i:13;p:11;a:r;x:58.22;y:-24.75;*/
static
void docgen_format_struct_member_typeandname_when_struct(char *buf, struct idcparse *idcp, struct idcp_struct_member *mem)
{
	/*For a member that has a struct type (determined by 'flags'), 'typeid' can be used to resolve the
	struct, then use 'nbytes' div by the struct's size to determine array size. The member type
	will have the struct name, but that will not show as an array.*/

	struct idcp_struct *struc;
	int struc_size;

	assert(0 <= mem->typeid && mem->typeid < idcp->num_structs);
	struc = idcp->structs + mem->typeid;
	buf += sprintf(buf, "<a href=#struc_%s>struct %s</a> %s", struc->name, struc->name, mem->name);
	/*member struct type can't be an empty struct*/
	assert((struc_size = docgen_get_struct_size(idcp, struc)));
	if (mem->nbytes > struc_size) {
		assert(!(mem->nbytes % struc_size));
		sprintf(buf, "[%d]", mem->nbytes / struc_size);
	}
}
/*jeanine:p:i:10;p:11;a:r;x:58.22;*/
static
void docgen_format_struct_member_typeandname_when_type(char *buf, struct idcp_struct_member *mem)
{
	/*Setting a struct member type in IDA to 'void *engageMarkers[128]' will show up as
	type 'void *[128]' in the IDC file.*/
	/*Similary, 'int (__thiscall *proc)(int)' shows as 'int (__thiscall *)(int)'.*/

	char *type, *sub, c;
	register int len;

	type = mem->type;
	len = strlen(type);
	if (type[len - 1] == '*') {
		sprintf(buf, "%s%s", mem->type, mem->name);
	} else if (type[len - 1] == ']') {
		buf[0] = 0;
		sub = type + len - 1;
		for (;;) {
			c = *sub;
			if (c == '[') {
				c = *(sub - 1);
				if (c != ']') {
					break;
				}
				sub -= 2;
			} else {
				sub--;
			}
		}
		len = sub - type;
		memcpy(buf, type, len);
		if (c == '*') {
			/*int *[32]*/
			sprintf(buf + len, "%s", mem->name);
		} else {
			/*int[32]*/
			sprintf(buf + len, " %s", mem->name);
		}
		strcat(buf, sub);
	} else if (strstr(type, "(__") && (sub = strstr(type, "*)("))) {
		len = sub - type + 1;
		memcpy(buf, type, len);
		sprintf(buf + len, "%s", mem->name);
		strcat(buf, sub + 1);
	} else {
		sprintf(buf, "%s %s", mem->type, mem->name);
		return;
	}
}
/*jeanine:p:i:11;p:5;a:r;x:137.58;y:0.55;*/
static
void docgen_print_struct_member(FILE *f, struct docgen *dg, struct idcp_struct *struc, int offset, int size, struct idcp_struct_member *mem)
{
	int offsetstrlen, isplaceholder, idx;
	struct docgen_tmpbuf *typeandname;
	char offsetstr[16];

	memset(offsetstr, ' ', 8);
	offsetstrlen = sprintf(offsetstr, "/*%X*/", offset);
	if (offsetstrlen < 8) {
		offsetstr[offsetstrlen] = '\t';
		offsetstr[offsetstrlen+1] = 0;
	}

	typeandname = docgen_get_tmpbuf(10000);
	if (mem) {
		idx = mem - dg->idcp->struct_members;
		if (dg->struct_member_needs_anchor[idx]) {
			fprintf(f, "<i id=struc_%s@%X></i>", struc->name, mem->offset);
		}
		isplaceholder = !strncmp(mem->name, "field_", 6) || !strncmp(mem->name, "floatField_", 11);
		if (IDC_is_enum(mem->flag)) {
			docgen_format_struct_member_typeandname_when_enum(typeandname->data, dg->idcp, mem);/*jeanine:r:i:9;*/
		} else if (IDC_is_struct(mem->flag)) {
			docgen_format_struct_member_typeandname_when_struct(typeandname->data, dg->idcp, mem);/*jeanine:r:i:13;*/
		} else if (mem->type) {
			docgen_format_struct_member_typeandname_when_type(typeandname->data, mem);/*jeanine:r:i:10;*/
			if (docgen_link_structs_enums(dg, &typeandname)) {/*jeanine:r:i:12;*/
				fprintf(stderr, "warn: struct '%s' member '%s' references an unknown struct/enum\n", struc->name, mem->name);
			}
		} else {
			switch (mem->nbytes) {
			case 1:
				sprintf(typeandname->data, "char %s", mem->name);
				break;
			case 2:
				sprintf(typeandname->data, "short %s", mem->name);
				break;
			case 4:
				sprintf(typeandname->data, "int %s", mem->name);
				break;
			default:
				sprintf(typeandname->data, "char %s[%d]", mem->name, mem->nbytes);
				break;
			};
			if (!isplaceholder) {
				strcat(typeandname->data, " <u></u>");
			}
		}
		docgen_symbol_print_comments_inline(f, dg->struct_member_comments + idx);/*jeanine:s:a:r;i:71;*/
	} else {
		sprintf(typeandname->data, "char _pad%X[0x%X]", offset, size);
		goto yesplaceholder;
	}

	if (isplaceholder) {
yesplaceholder:
		fprintf(f, "<i>%s%s;</i>\n", offsetstr, typeandname->data);
	} else {
		fprintf(f, "<i>%s</i>%s;\n", offsetstr, typeandname->data);
	}
	docgen_free_tmpbuf(typeandname);
}
/*jeanine:p:i:5;p:88;a:r;x:24.09;*/
static
void docgen_print_struct(FILE *f, struct docgen *dg, struct docgen_structinfo *structinfo, struct idcp_struct *struc)
{
	int num_members, size, lastoffset, i;
	struct docgen_funcinfo *funcinfo;
	struct idcp_struct_member *mem;
	struct idcp_stuff *func;

	num_members = struc->end_idx - struc->start_idx;
	size = docgen_get_struct_size(dg->idcp, struc);

	fprintf(f, "\n<a id=struc_%s></a>", struc->name);
	docgen_symbol_print_comments_block(f, &structinfo->comments);/*jeanine:s:a:r;i:69;*/
	/*these are 'div' instead of 'pre' because 'h3' can't be inside 'pre'*/
	fprintf(f, "\n<div>%s <h3>%s</h3> { <i>/*%d members, size %Xh*/</i>%s\n",
		struc->is_union ? "<b>union</b>" : "struct",
		struc->name,
		num_members,
		size,
		structinfo->is_class ? " <em></em>" : ""
	);
	mem = dg->idcp->struct_members + struc->start_idx;
	lastoffset = 0;
	for (; num_members; num_members--, mem++) {
		if (mem->offset > lastoffset) {
			docgen_print_struct_member(f, dg, struc, lastoffset, mem->offset - lastoffset, NULL);/*jeanine:s:a:r;i:11;*/
		}
		docgen_print_struct_member(f, dg, struc, mem->offset, mem->nbytes, mem);/*jeanine:r:i:11;*/
		lastoffset = mem->offset + mem->nbytes;
	}
	fprintf(f, "};\n<i>EXPECT_SIZE(struct %s, 0x%X);</i></div>", struc->name, size);
	if (structinfo->is_class) {
		fwrite("\n<ul>\n", 6, 1, f);
		for (i = 0, funcinfo = dg->funcinfos; i < dg->num_funcinfos; i++, funcinfo++) {
			if (funcinfo->methodof == structinfo) {
				func = funcinfo->func;
				fprintf(f, "<li><!--%X-->\n<pre><i>%X</i> <a href=funcs.html#%X title='%s'>%s</a></pre>\n",
					func->addr,
					func->addr,
					func->addr,
					func->data.func.type ? func->data.func.type : func->name,
					func->name
				);
			}
		}
		fprintf(f, "</ul>");
	}
}
/*jeanine:p:i:22;p:88;a:r;x:24.00;y:93.00;*/
static
void docgen_print_enum(FILE *f, struct docgen *dg, struct docgen_enuminfo *enuminfo, struct idcp_enum *enu)
{
	struct idcp_enum_member *mem;
	int num_members, idx;

	num_members = enu->end_idx - enu->start_idx;

	fprintf(f, "\n<a id=enu_%s></a>", enu->name);
	docgen_symbol_print_comments_block(f, &enuminfo->comments);/*jeanine:s:a:r;i:69;*/
	/*these are 'div' instead of 'pre' because 'h3' can't be inside 'pre'*/
	fprintf(f, "\n<div>enum <h3>%s</h3> { <i>/*%d members*/</i>\n", enu->name, num_members);
	mem = dg->idcp->enum_members + enu->start_idx;
	for (idx = enu->start_idx; num_members; num_members--, mem++, idx++) {
		if (dg->enum_member_needs_anchor[idx]) {
			fprintf(f, "<i id=enu_%s%X></i>", enu->name, mem->value);
		}
		docgen_symbol_print_comments_inline(f, dg->enum_member_comments + idx);/*jeanine:r:i:71;:s:a:r;i:71;*/
		fprintf(f, "\t%s = 0x%X,\n", mem->name, mem->value);
	}
	fwrite("};</div>", 8, 1, f);
}
/*jeanine:p:i:23;p:88;a:r;x:24.00;y:37.00;*/
static
void docgen_print_data(FILE *f, struct docgen *dg, struct docgen_datainfo *datainfo, struct idcp_stuff *data)
{
	struct docgen_structinfo *strucinfo;
	struct docgen_tmpbuf *type;
	char unconfirmed_type, *st;
	int len;

	/*TODO: array stuff*/
	type = docgen_get_tmpbuf(10000);
	unconfirmed_type = 0;
	if (data->data.data.struct_type) {
		st = data->data.data.struct_type;
		if (docgen_find_struct(dg, st, strlen(st))) {
			sprintf(type->data, "<a href=structs.html#struc_%s>struct %s</a> ", st, st);
		} else {
			fprintf(stderr, "warn: cannot find struct '%s' for var %X '%s'\n", st, data->addr, data->name);
			sprintf(type->data, "<strong>struct %s<strong> ", st);
		}
	} else if (datainfo->type) {
		strucinfo = docgen_find_struct_from_text_type(dg, datainfo->type, datainfo->type_len);
		if (strucinfo) {
			st = strucinfo->struc->name;
			sprintf(type->data, "<a href=structs.html#struc_%s>struct %s</a>", st, st);
			len = 7 + strlen(st);
			if (len < datainfo->type_len) {
				strncat(type->data, datainfo->type + len, datainfo->type_len - len);
			}
		} else {
			fprintf(stderr, "warn: var %X '%s' has unresolved @docgen:type:\n%s", data->addr, data->name, datainfo->type);
			memcpy(type->data, "<strong>", 8);
			memcpy(type->data + 8, datainfo->type, datainfo->type_len);
			memcpy(type->data + 8 + datainfo->type_len, "</strong>", 9);
			type->data[8 + datainfo->type_len + 9] = 0;
		}
	} else if (data->data.data.flags & IDCP_DATA_FLOAT) {
		assert(data->data.data.size == 4);
		strcpy(type->data, "float ");
	} else if (data->data.data.flags & IDCP_DATA_DOUBLE) {
		assert(data->data.data.size == 8);
		strcpy(type->data, "double ");
	} else {
		switch (data->data.data.size) {
		case 1:
			strcpy(type->data, "char ");
			break;
		case 2:
			strcpy(type->data, "short ");
			break;
		case 4:
			strcpy(type->data, "int ");
			break;
		default:
			sprintf(type->data, "i%d ", data->data.data.size * 8);
			break;
		};
		unconfirmed_type = 1;
	}
	fprintf(f, "\n<a id=%X></a>", data->addr);
	docgen_symbol_print_comments_block(f, &datainfo->comments);/*jeanine:s:a:r;i:69;*/
	/*these are 'div' instead of 'pre' because 'h3' can't be inside 'pre'*/
	fprintf(f, "\n<div><i>%X</i> %s<h3>%s</h3>", data->addr, type->data, data->name);
	docgen_free_tmpbuf(type);
	if (data->data.data.arraysize) {
		fprintf(f, "[%d]", data->data.data.arraysize);
	}
	if (unconfirmed_type) {
		fwrite(" <u></u>", 8, 1, f);
	}
	fwrite("</div>", 6, 1, f);
}
/*jeanine:p:i:56;p:37;a:r;x:128.71;y:-20.02;*/
/**
@param ref must be zero terminated
*/
static
void docgen_append_ref_text(struct mmparse *mm, void (*append_func)(struct mmparse*,char*,int), char *ref, int reflen)
{
	struct docgen_mmparse_userdata *ud;
	struct docgen_ref_element *element;
	struct docgen_ref res;
	char addr[16], *name;
	int len;

	ud = mm->config.userdata;
	docgen_resolve_ref(ud->dg, &res, ref, reflen);/*jeanine:r:i:57;*/
	element = res.elements;
	for (;;) {
		switch (element->type) {
		case FUNC:
			len = sprintf(addr, "%X", element->el.func->addr);
			append_func(mm, "<a href=funcs.html#", 19);
			append_func(mm, addr, len);
			append_func(mm, ">", 1);
			name = element->el.func->name;
			append_func(mm, name, strlen(name));
			append_func(mm, "</a>", 4);
			break;
		case DATA:
			len = sprintf(addr, "%X", element->el.data->addr);
			append_func(mm, "<a href=vars.html#", 18);
			append_func(mm, addr, len);
			append_func(mm, ">", 1);
			name = element->el.data->name;
			append_func(mm, name, strlen(name));
			append_func(mm, "</a>", 4);
			break;
		case STRUCTMEMBER:
			ud->dg->struct_member_needs_anchor[element->member.struc - ud->dg->idcp->struct_members] = 1;
			append_func(mm, "<a href=structs.html#struc_", 27);
			append_func(mm, element->el.struc->name, strlen(element->el.struc->name));
			len = sprintf(addr, "@%X", element->member.struc->offset);
			append_func(mm, addr, len);
			append_func(mm, ">", 1);
			name = element->member.struc->name;
			append_func(mm, name, strlen(name));
			append_func(mm, "</a>", 4);
			break;
		case STRUCT:
			name = element->el.struc->name;
			len = strlen(name);
			append_func(mm, "<a href=structs.html#struc_", 27);
			append_func(mm, name, len);
			append_func(mm, ">struct ", 8);
			append_func(mm, name, len);
			append_func(mm, "</a>", 4);
			break;
		case ENUMMEMBER:
			ud->dg->enum_member_needs_anchor[element->member.enu - ud->dg->idcp->enum_members] = 1;
			append_func(mm, "<a href=enums.html#enu_", 23);
			append_func(mm, element->el.enu->name, strlen(element->el.enu->name));
			len = sprintf(addr, "%X", element->member.enu->value);
			append_func(mm, addr, len);
			append_func(mm, ">", 1);
			name = element->member.enu->name;
			append_func(mm, name, strlen(name));
			append_func(mm, "</a>", 4);
			break;
		case ENUM:
			name = element->el.enu->name;
			len = strlen(name);
			append_func(mm, "<a href=enums.html#enu_", 23);
			append_func(mm, name, len);
			append_func(mm, ">enum ", 6);
			append_func(mm, name, len);
			append_func(mm, "</a>", 4);
			break;
		default:
			mmparse_failmsgf(mm, "unknown ref '%s'", ref);
			append_func(mm, "<strong class='error' style='font-family:monospace'>?", 53);
			append_func(mm, ref, reflen);
			append_func(mm, "?</strong>", 10);
			return;
		}
		if (!(element = element->next)) {
			break;
		}
		if (element->is_from_pointer) {
			append_func(mm, "->", 2);
		} else {
			append_func(mm, ".", 1);
		}
	}
}
/*jeanine:p:i:35;p:30;a:r;x:6.67;y:-21.00;*/
static
void mmparse_cb_mode_symbols_start(struct mmparse *mm)
{
	mmparse_append_to_main_output(mm, "<p><strong>Symbols:</strong></p>\n<ul>\n", 38);
}
/*jeanine:p:i:60;p:37;a:r;x:29.78;*/
static
void docgen_mmparse_symbols_println_ref_append(struct mmparse *mm, char *buf, int len)
{
	mmparse_append_to_main_output(mm, buf, len);
}
/*jeanine:p:i:37;p:30;a:r;x:6.67;y:-13.00;*/
static
int mmparse_cb_mode_symbols_println(struct mmparse *mm)
{
	if (mm->pd.line_len < 3 || mm->pd.line[0] != '-') {
		mmparse_failmsg(mm, "lines in 'symbols' mode must start with a hyphen");
		assert(0);
	}
	mmparse_append_to_main_output(mm, "<li>", 4);
	docgen_append_ref_text(mm, docgen_mmparse_symbols_println_ref_append, mm->pd.line + 2, mm->pd.line_len - 2);/*jeanine:r:i:56;:r:i:60;*/
	mmparse_append_to_main_output(mm, "</li>\n", 6);
	return 0; /*just returning 0 because no placeholders should've been used*/
}
/*jeanine:p:i:36;p:30;a:r;x:6.67;y:1.00;*/
static
void mmparse_cb_mode_symbols_end(struct mmparse *mm)
{
	mmparse_append_to_main_output(mm, "</ul>\n", 6);
}
/*jeanine:p:i:38;p:30;a:r;x:6.67;y:9.00;*/
static
enum mmp_dir_content_action mmparse_cb_mode_symbols_directive(struct mmparse *mm, struct mmp_dir_content_data *data)
{
	mmparse_failmsg(mm, "don't use directives while in 'symbols' mode");
	assert(0);
	return LEAVE_CONTENTS;
}
/*jeanine:p:i:51;p:47;a:r;x:5.56;y:-31.00;*/
static
void mmparse_cb_mode_ida_start(struct mmparse *mm)
{
	mmparse_append_to_main_output(mm, "<pre class='ida'>\n", 18);
}
/*jeanine:p:i:52;p:47;a:r;x:5.56;y:-23.00;*/
static
int mmparse_cb_mode_ida_println(struct mmparse *mm)
{
	register char c;
	int marginlen;

	mm->pd.line[mm->pd.line_len++] = '\n';
	if (mm->pd.line[0] == '.') {
		marginlen = 0;
		do {
			c = mm->pd.line[++marginlen];
			if (!c) {
				goto nomargin;
			}
		} while (c != ' ');
		if (!strncmp(mm->pd.line + 1, "text", 4)) {
			mmparse_append_to_main_output(mm, "<span class='mt'>", 17);
		} else {
			mmparse_append_to_main_output(mm, "<span class='mc'>", 17);
		}
		mmparse_append_to_main_output(mm, mm->pd.line, marginlen);
		mmparse_append_to_main_output(mm, "</span>", 7);
		mmparse_append_to_main_output(mm, mm->pd.line + marginlen, mm->pd.line_len - marginlen);
		return 24;
	} else {
nomargin:
		mmparse_append_to_main_output(mm, mm->pd.line, mm->pd.line_len);
		return 0;
	}
}
/*jeanine:p:i:54;p:47;a:r;x:5.56;y:8.00;*/
static
enum mmp_dir_content_action mmparse_cb_mode_ida_directive(struct mmparse *mm, struct mmp_dir_content_data *data)
{
	register char *name;

	name = data->directive->name;
	if (!strcmp(name, "num") || !strcmp(name, "str") || !strcmp(name, "hi") ||
		!strcmp(name, "comment") || !strcmp(name, "ident"))
	{
		mmparse_append_to_expanded_line(mm, "<span class='", 13);
		mmparse_append_to_expanded_line(mm, name, strlen(name));
		mmparse_append_to_expanded_line(mm, "'>", 2);
		mmparse_append_to_closing_tag(mm, "</span>", 7);
	} else {
		return mmparse_cb_mode_normal_directive(mm, data);
	}
	return LEAVE_CONTENTS;
}
/*jeanine:p:i:30;p:91;a:r;x:64.70;y:-52.56;*/
struct mmp_mode docgen_mmparse_mode_symbols = {
	mmparse_cb_mode_symbols_start,/*jeanine:r:i:35;*/
	mmparse_cb_mode_symbols_println,/*jeanine:r:i:37;*/
	mmparse_cb_mode_symbols_end,/*jeanine:r:i:36;*/
	mmparse_cb_mode_symbols_directive,/*jeanine:r:i:38;*/
	"symbols",
	MMPARSE_DO_PARSE_LINES
};
/*jeanine:p:i:47;p:91;a:r;x:62.22;y:7.00;*/
struct mmp_mode docgen_mmparse_mode_ida = {
	mmparse_cb_mode_ida_start,/*jeanine:r:i:51;*/
	mmparse_cb_mode_ida_println,/*jeanine:r:i:52;*/
	mmpextras_cb_mode_pre_end,
	mmparse_cb_mode_ida_directive,/*jeanine:r:i:54;*/
	"ida",
	MMPARSE_DO_PARSE_LINES
};
/*jeanine:p:i:59;p:61;a:r;x:96.73;*/
static
void docgen_mmparse_dir_ref_append(struct mmparse *mm, char *buf, int len)
{
	mmparse_append_to_expanded_line(mm, buf, len);
}
/*jeanine:p:i:67;p:65;a:r;x:23.00;*/
static
void docgen_mmparse_img_directive_validate_and_trunc_name(struct mmparse *mm, struct mmp_dir_content_data *data)
{
	struct mmp_dir_arg *src_arg;
	FILE *file;

	mmpextras_require_directive_argument(mm, data->directive, "alt"); /*just check if it's present*/
	src_arg = mmpextras_require_directive_argument(mm, data->directive, "src");
	assert(((void)"increase MMPARSE_DIRECTIVE_ARGV_MAX_LEN", src_arg->value_len < MMPARSE_DIRECTIVE_ARGV_MAX_LEN + 4));
	memcpy(src_arg->value + 4, src_arg->value, src_arg->value_len + 1);
	memcpy(src_arg->value, "img/", 4);
	file = fopen(src_arg->value, "rb");
	if (file) {
		fclose(file);
	} else {
		mmparse_failmsgf(mm, "failed to open img file '%s' for reading", src_arg->value);
	}
	/*cut off the extra part of the tag name (ie make 'imgcaptioned' or 'imgcollapsed' into 'img')*/
	data->directive->name[3] = 0;
}
/*jeanine:p:i:97;p:72;a:r;x:15.22;y:-83.00;*/
static
enum mmp_dir_content_action docgen_mmparse_dir_img(struct mmparse *mm, struct mmp_dir_content_data *data)
{
	struct mmp_dir_arg *alt_arg;
	register char *c;

	if (!data->content_len) {
		mmparse_failmsg(mm, "img needs alt text in the directive's contents");
		assert(0);
	}
	assert(((void) "increase MMPARSE_DIRECTIVE_MAX_ARGS", data->directive->argc < MMPARSE_DIRECTIVE_MAX_ARGS));
	assert(((void) "increase MMPARSE_DIRECTIVE_ARGV_MAX_LEN", data->content_len < MMPARSE_DIRECTIVE_ARGV_MAX_LEN));
	c = data->contents;
	while (*c) {
		if (*(c++) == '"') {
			/*since alt text is put in an attribute (using double quotes), disallow double quotes*/
			mmparse_failmsg(mm, "don't use double quotes in img alt text");
			assert(0);
		}
	}
	alt_arg = data->directive->args + data->directive->argc++;
	alt_arg->name_len = 3;
	alt_arg->value_len = data->content_len;
	memcpy(alt_arg->name, "alt", 4);
	memcpy(alt_arg->value, data->contents, data->content_len + 1);
	docgen_mmparse_img_directive_validate_and_trunc_name(mm, data);/*jeanine:s:a:r;i:67;*/
	mmparse_append_to_expanded_line(mm, "<p class='imgholder'>", 21);
	mmparse_print_tag_with_directives(mm, data->directive, ">");
	mmparse_append_to_expanded_line(mm, "</p>", 4);
	return DELETE_CONTENTS;
}
/*jeanine:p:i:65;p:72;a:r;x:15.03;y:-49.44;*/
static
enum mmp_dir_content_action docgen_mmparse_dir_imgcaptioned(struct mmparse *mm, struct mmp_dir_content_data *data)
{
	docgen_mmparse_img_directive_validate_and_trunc_name(mm, data);/*jeanine:r:i:67;*/
	mmparse_append_to_expanded_line(mm, "<p class='imgholder'>", 21);
	mmparse_print_tag_with_directives(mm, data->directive, "><br>");
	mmparse_append_to_closing_tag(mm, "</p>", 4);
	return LEAVE_CONTENTS;
}
/*jeanine:p:i:66;p:72;a:r;x:15.03;y:-35.91;*/
static
enum mmp_dir_content_action docgen_mmparse_dir_imgcollapsed(struct mmparse *mm, struct mmp_dir_content_data *data)
{
	int expanded_line_len, len;

	docgen_mmparse_img_directive_validate_and_trunc_name(mm, data);/*jeanine:s:a:r;i:67;*/
	mmparse_append_to_expanded_line(mm, "<details><summary>", 18);
	mmparse_append_to_closing_tag(mm, "</summary><p class='center'>", 28);
	/*haxx, I want to use the functionality of 'mmparse_print_tag_with_directives' but that will
	append it to the expanded line while I want it to be in the closing tag.
	So this hax will call it and then remove it from the expanded line and paste it in the closing tag.*/
	expanded_line_len = mm->pd.line_len;
	mmparse_print_tag_with_directives(mm, data->directive, "></p></details>");
	len = mm->pd.line_len - expanded_line_len;
	mm->pd.line_len -= len;
	mm->pd.next_placeholder_offset -= len;
	mmparse_append_to_closing_tag(mm, mm->pd.line + mm->pd.line_len, len);
	mm->pd.line[mm->pd.line_len] = 0;
	/*end haxx*/
	return LEAVE_CONTENTS;
}
/*jeanine:p:i:42;p:72;a:r;x:14.69;y:53.63;*/
static
enum mmp_dir_content_action docgen_mmparse_dir_hookfileref(struct mmparse *mm, struct mmp_dir_content_data *data)
{
	char *filename;
	FILE *file;

	filename = alloca(5 + data->content_len);
	sprintf(filename, "../%s", data->contents);
	file = fopen(filename, "rb");
	if (file) {
		fclose(file);
	} else {
		mmparse_failmsgf(mm, "file '%s' not found", filename);
	}
	mmparse_append_to_expanded_line(mm, "<code><a class='ext' href='https://github.com/yugecin/nfsu2-re/blob/master/", 75);
	mmparse_append_to_expanded_line(mm, data->contents, data->content_len);
	mmparse_append_to_expanded_line(mm, "'>", 2);
	mmparse_append_to_closing_tag(mm, "</a></code>", 11);
	return LEAVE_CONTENTS;
}
/*jeanine:p:i:64;p:72;a:r;x:14.58;y:-11.00;*/
static
enum mmp_dir_content_action docgen_mmparse_dir_dumpfileref(struct mmparse *mm, struct mmp_dir_content_data *data)
{
	char *filename;
	FILE *file;
	int size;

	filename = alloca(20 + data->content_len);
	sprintf(filename, "dumps/%s", data->contents);
	file = fopen(filename, "rb");
	if (file) {
		fseek(file, 0l, SEEK_END);
		size = ftell(file);
		fclose(file);
	} else {
		mmparse_failmsgf(mm, "failed to open dumpfile '%s' for reading", data->contents);
		size = 0;
	}
	mmparse_append_to_expanded_line(mm, "<a href='dumps/", 15);
	mmparse_append_to_expanded_line(mm, data->contents, data->content_len);
	mmparse_append_to_expanded_line(mm, "'>", 2);
	size = sprintf(filename, " (%dKB)", size / 1000);
	mmparse_append_to_closing_tag(mm, "</a>", 4);
	mmparse_append_to_closing_tag(mm, filename, size);
	return LEAVE_CONTENTS;
}
/*jeanine:p:i:95;p:96;a:r;x:3.33;*/
static
void mmpextras_cb_placeholder_nop(struct mmparse *mm, struct mmp_output_part *output, void *data, int data_size)
{
}
/*jeanine:p:i:96;p:72;a:r;x:14.56;y:12.63;*/
static
enum mmp_dir_content_action docgen_mmparse_dir_blog(struct mmparse *mm, struct mmp_dir_content_data *data)
{
	struct docgen_mmparse_userdata *ud, *ud2;
	struct mmp_dir_arg *date_arg;
	char *phdata, buf[200];
	int i, index, len;

	ud = mm->config.userdata;
	if (ud->blogpost.is_blogpost) {
		date_arg = mmpextras_require_directive_argument(mm, data->directive, "date");
		/*Using a nop placeholder to allocate data for title/date*/
		phdata = mmparse_allocate_placeholder(mm, mmpextras_cb_placeholder_nop, date_arg->value_len + 1 + data->content_len + 1)->data;/*jeanine:r:i:95;*/
		memcpy(phdata, date_arg->value, date_arg->value_len + 1);
		ud->blogpost.date = phdata;
		phdata += date_arg->value_len + 1;
		memcpy(phdata, data->contents, data->content_len + 1);
		ud->blogpost.title = phdata;
	} else {
		if (strncmp(data->contents, "blog", 4) || data->content_len < 5) {
			mmparse_failmsg(mm, "blog directive contents should be formatted like 'blog2'");
			assert(0);
		}
		index = atoi(data->contents + 4);
		for (i = 0; i < ud->num_blogs; i++) {
			ud2 = ud->blogs[i]->config.userdata;
			if (ud2->blogpost.is_blogpost && ud2->blogpost.index == index) {
				len = sprintf(buf, "<a href='%s'>blogpost: %s</a>", ud2->blogpost.htmlfile, ud2->blogpost.title);
				mmparse_append_to_expanded_line(mm, buf, len);
				return DELETE_CONTENTS;
			}
		}
		mmparse_failmsgf(mm, "cannot find blog with index %d", index);
		assert(0);
	}
	return DELETE_CONTENTS;
}
/*jeanine:p:i:61;p:72;a:r;x:14.79;y:76.22;*/
static
enum mmp_dir_content_action docgen_mmparse_dir_ref(struct mmparse *mm, struct mmp_dir_content_data *data)
{
	docgen_append_ref_text(mm, docgen_mmparse_dir_ref_append, data->contents, data->content_len);/*jeanine:r:i:59;:s:a:r;i:56;*/
	return DELETE_CONTENTS;
}
/*jeanine:p:i:68;p:70;a:r;x:3.00;y:1.94;*/
static
void docgen_mmparse_symbol_comment(struct mmparse *mm, struct docgen_symbol_comment *comment, char *cmt)
{
	struct mmp_output_part *mmpart;

	mm->config.source = cmt;
	mm->config.source_len = strlen(cmt);
	mmparse(mm);
	mmpart = mm->output;
	assert(mmpart && mmpart->data0); /*expecting at least one output*/
	assert(!mmpart->data1_len); /*not expecting this to use placeholders*/
	comment->buf = mmpart->data0;
	comment->buf_len = mmpart->data0_len;
	mm->config.dest.data0 += mmpart->data0_len;
	mm->config.dest.data0_len -= mmpart->data0_len;
	assert(!(++mmpart)->data0); /*expecting just one output, because not expecting placeholders*/
}
/*jeanine:p:i:70;p:74;a:r;x:12.92;y:26.78;*/
static
void docgen_mmparse_symbol_comments(struct mmparse *mm, struct docgen_symbol_comments *comments, char *cmt, char *rep_cmt, char *subject)
{
	if (cmt) {
		memcpy(subject, "cmt", 3);
		docgen_mmparse_symbol_comment(mm, &comments->normal, cmt);/*jeanine:r:i:68;*/
	}
	if (rep_cmt) {
		memcpy(subject, "rep", 3);
		docgen_mmparse_symbol_comment(mm, &comments->rep, rep_cmt);/*jeanine:s:a:r;i:68;*/
	}
}
/*jeanine:p:i:72;p:75;a:b;y:117.11;*/
struct mmp_dir_handler docgen_mmp_doc_directives[] = {
	{ "img", docgen_mmparse_dir_img },/*jeanine:r:i:97;*/
	{ "imgcaptioned", docgen_mmparse_dir_imgcaptioned },/*jeanine:r:i:65;*/
	{ "imgcollapsed", docgen_mmparse_dir_imgcollapsed },/*jeanine:r:i:66;*/
	{ "dumpfileref", docgen_mmparse_dir_dumpfileref },/*jeanine:r:i:64;*/
	{ "anchor", mmpextras_dir_anchor },
	{ "index", mmpextras_dir_index },
	{ "href", mmpextras_dir_href },
	{ "a", mmpextras_dir_a },
	{ "h", mmpextras_dir_h },
	{ "blog", docgen_mmparse_dir_blog },/*jeanine:r:i:96;*/
	{ "hookfileref", docgen_mmparse_dir_hookfileref },/*jeanine:r:i:42;*/
	{ "ref", docgen_mmparse_dir_ref },/*jeanine:r:i:61;*/
	{ NULL, NULL }
};
/*jeanine:p:i:91;p:92;a:b;y:106.44;*/
struct mmp_mode *docgen_mmp_doc_modes[] = {
	&mmparse_mode_normal, /*must be first*/
	&docgen_mmparse_mode_symbols,/*jeanine:r:i:30;*/
	&docgen_mmparse_mode_ida,/*jeanine:r:i:47;*/
	&mmpextras_mode_section,
	&mmparse_mode_plain,
	&mmpextras_mode_pre,
	&mmpextras_mode_ul,
	&mmparse_mode_nop,
	NULL
};
/*jeanine:p:i:92;p:72;a:b;y:56.12;*/
struct mmp_dir_handler docgen_mmp_sym_directives[] = {
	{ "blog", docgen_mmparse_dir_blog },/*jeanine:s:a:r;i:96;*/
	{ "hookfileref", docgen_mmparse_dir_hookfileref },/*jeanine:s:a:r;i:42;*/
	{ "ref", docgen_mmparse_dir_ref },/*jeanine:s:a:r;i:61;*/
	{ "href", mmpextras_dir_href },
	{ "a", mmpextras_dir_a },
	{ NULL, NULL }
};
/*jeanine:p:i:93;p:91;a:b;y:1.88;*/
struct mmp_mode *docgen_mmp_sym_modes[] = {
	&mmpextras_mode_paragraphed, /*must be first*/
	&docgen_mmparse_mode_ida,/*jeanine:s:a:r;i:47;*/
	&mmparse_mode_plain,
	&mmpextras_mode_pre,
	&mmpextras_mode_ul,
	NULL
};
/*jeanine:p:i:94;p:93;a:b;y:1.88;*/
struct mmp_mode *docgen_mmp_sym_cmtmodes[] = {
	&mmparse_mode_normal,
	NULL
};
/*jeanine:p:i:87;p:90;a:r;x:7.78;y:-30.00;*/
static
void docgen_collect_funcinfos(struct docgen *dg)
{
	struct idcparse *idcp = dg->idcp;
	struct idcp_stuff *stuff;
	register int len;
	int i;

	assert(dg->funcinfos = calloc(1, sizeof(struct docgen_funcinfo) * idcp->num_funcs));
	for (dg->num_funcinfos = 0, i = 0, stuff = idcp->stuffs; i < idcp->num_stuffs; i++, stuff++) {
		if (stuff->type == IDCP_STUFF_TYPE_FUNC && stuff->name) {
			dg->funcinfos[dg->num_funcinfos].func = stuff;
			dg->funcinfos[dg->num_funcinfos].name_len = len = strlen(stuff->name);
			if (!((len == 10 && !strncmp("SEH_", stuff->name, 4)) ||
				(len == 20 && !strncmp("init_function_", stuff->name, 14)) ||
				!strncmp("nullsub_", stuff->name, 8)) || stuff->comment || stuff->rep_comment)
			{
				dg->num_funcinfos++;
			}
		}
	}
}
/*jeanine:p:i:80;p:90;a:r;x:7.78;y:-5.00;*/
static
void docgen_collect_structinfos(struct docgen *dg)
{
	struct idcparse *idcp = dg->idcp;
	int i;

	assert(dg->structinfos = calloc(1, sizeof(struct docgen_structinfo) * idcp->num_structs));
	for (i = 0; i < idcp->num_structs; i++) {
		dg->structinfos[i].struc = idcp->structs + i;
		dg->structinfos[i].name_len = strlen(idcp->structs[i].name);
	}
	dg->num_structinfos = idcp->num_structs;
}
/*jeanine:p:i:81;p:90;a:r;x:7.78;y:12.00;*/
static
void docgen_collect_enuminfos(struct docgen *dg)
{
	struct idcparse *idcp = dg->idcp;
	int i;

	assert(dg->enuminfos = calloc(1, sizeof(struct docgen_enuminfo) * idcp->num_enums));
	for (i = 0; i < idcp->num_enums; i++) {
		dg->enuminfos[i].enu = idcp->enums + i;
		dg->enuminfos[i].name_len = strlen(idcp->enums[i].name);
	}
	dg->num_enuminfos = idcp->num_enums;
}
/*jeanine:p:i:78;p:90;a:r;x:7.78;y:28.00;*/
static
void docgen_collect_datainfos(struct docgen *dg)
{
	struct docgen_datainfo *datainfo;
	struct idcparse *idcp = dg->idcp;
	struct idcp_stuff *data;
	char *lf;
	int i;

	assert(dg->datainfos = calloc(1, sizeof(struct docgen_datainfo) * idcp->num_datas));
	for (dg->num_datainfos = 0, i = 0; i < idcp->num_stuffs; i++) {
		if (idcp->stuffs[i].type == IDCP_STUFF_TYPE_DATA && idcp->stuffs[i].name) {
			data = idcp->stuffs + i;
			if (!strncmp(data->name, "jpt_", 4)) {
				continue;
			}
			datainfo = dg->datainfos + dg->num_datainfos++;
			datainfo->data = data;
			if (data->comment && !strncmp(data->comment, "@docgen:type:", 13)) {
				data->comment += 13;
				datainfo->type = data->comment;
				lf = strstr(data->comment, "\n");
				if (lf) {
					datainfo->type_len = lf - data->comment;
					data->comment = lf + 1;
				} else {
					datainfo->type_len = strlen(data->comment);
					data->comment = NULL;
				}
			}
		}
	}
}
/*jeanine:p:i:76;p:75;a:r;x:14.77;*/
/**
Since all mmparse instance use the same big buffer, some offsets/sizefree need to be adjusted.

data0 is the main output, and must be offset
data1 is placeholder output, this can be overwritten because data in it should be used immediately
data2 is closingtag buffer, this can be overwritten because it's used immediately during parsing
data3 is placeholder data buffer, and must be offset
*/
static
void docgen_adjust_mmp_data_after_mmparse(struct docgen_mmp_data *mmpd, struct mmparse *mm)
{
	struct mmp_output_part *part;
	register int phbuf_used;

	for (part = mm->output; part->data0; part++) {
		mmpd->d0 += part->data0_len;
		mmpd->len0 -= part->data0_len;
	}
	phbuf_used = mm->ph.databuf - mmpd->d3;
	mmpd->d3 += phbuf_used;
	mmpd->len3 -= phbuf_used;
}
/*jeanine:p:i:74;p:83;a:r;x:6.67;y:-36.00;*/
static
void docgen_mmparse_func_comments(struct docgen *dg, struct mmparse *mm, char *subject)
{
	struct docgen_funcinfo *funcinfo;
	int i;

	for (i = 0, funcinfo = dg->funcinfos; i < dg->num_funcinfos; i++, funcinfo++) {
		sprintf(subject, "xxx func %X", funcinfo->func->addr);
		docgen_mmparse_symbol_comments(mm, &funcinfo->comments, funcinfo->func->comment, funcinfo->func->rep_comment, subject);/*jeanine:r:i:70;*/
	}
}
/*jeanine:p:i:84;p:83;a:r;x:6.67;y:-25.00;*/
static
void docgen_mmparse_data_comments(struct docgen *dg, struct mmparse *mm, char *subject)
{
	struct docgen_datainfo *datainfo;
	int i;

	for (i = 0, datainfo = dg->datainfos; i < dg->num_datainfos; i++, datainfo++) {
		sprintf(subject, "xxx var %X", datainfo->data->addr);
		docgen_mmparse_symbol_comments(mm, &datainfo->comments, datainfo->data->comment, datainfo->data->rep_comment, subject);/*jeanine:s:a:r;i:70;*/
	}
}
/*jeanine:p:i:85;p:83;a:r;x:6.67;y:-13.00;*/
static
void docgen_mmparse_enum_comments(struct docgen *dg, struct mmparse *mm, char *subject)
{
	struct docgen_enuminfo *enuminfo;
	struct idcp_enum_member *mem;
	struct idcp_enum *enu;
	int i, j;

	assert(dg->enum_member_comments = calloc(1, sizeof(struct docgen_symbol_comments) * dg->idcp->num_enum_members));
	for (i = 0, enuminfo = dg->enuminfos; i < dg->num_enuminfos; i++, enuminfo++) {
		enu = enuminfo->enu;
		sprintf(subject, "xxx enum %s", enu->name);
		docgen_mmparse_symbol_comments(mm, &enuminfo->comments, enu->comment, enu->rep_comment, subject);/*jeanine:s:a:r;i:70;*/
		mm->config.modes = docgen_mmp_sym_cmtmodes;
		for (j = enu->start_idx; j < enu->end_idx; j++) {
			mem = dg->idcp->enum_members + j;
			sprintf(subject, "xxx enum %s member %s", enu->name, mem->name);
			docgen_mmparse_symbol_comments(mm, dg->enum_member_comments + j, mem->comment, mem->rep_comment, subject);/*jeanine:s:a:r;i:70;*/
		}
		mm->config.modes = docgen_mmp_sym_modes;
	}
}
/*jeanine:p:i:82;p:83;a:r;x:6.67;y:9.00;*/
static
void docgen_mmparse_struct_comments(struct docgen *dg, struct mmparse *mm, char *subject)
{
	struct docgen_structinfo *structinfo;
	struct idcp_struct_member *mem;
	struct idcp_struct *struc;
	int i, j;

	assert(dg->struct_member_comments = calloc(1, sizeof(struct docgen_symbol_comments) * dg->idcp->num_struct_members));
	for (i = 0, structinfo = dg->structinfos; i < dg->num_structinfos; i++, structinfo++) {
		struc = structinfo->struc;
		sprintf(subject, "xxx struct %s", struc->name);
		docgen_mmparse_symbol_comments(mm, &structinfo->comments, struc->comment, struc->rep_comment, subject);/*jeanine:s:a:r;i:70;*/
		mm->config.modes = docgen_mmp_sym_cmtmodes;
		for (j = struc->start_idx; j < struc->end_idx; j++) {
			mem = dg->idcp->struct_members + j;
			sprintf(subject, "xxx struct %s member %s", struc->name, mem->name);
			docgen_mmparse_symbol_comments(mm, dg->struct_member_comments + j, mem->comment, mem->rep_comment, subject);/*jeanine:s:a:r;i:70;*/
		}
		mm->config.modes = docgen_mmp_sym_modes;
	}
}
/*jeanine:p:i:83;p:75;a:r;x:12.89;*/
static
void docgen_mmparse_all_symbol_comments(struct docgen *dg, struct mmparse *mm)
{
	struct docgen_tmpbuf *tmpbuf_subject;
	struct docgen_mmparse_userdata *ud;

	mm->config.debug_subject = (tmpbuf_subject = docgen_get_tmpbuf(10000))->data;
	ud = mm->config.userdata;

	ud->mmpextras.config.target_file_len = strlen(ud->mmpextras.config.target_file = "funcs.html");
	docgen_mmparse_func_comments(dg, mm, mm->config.debug_subject);/*jeanine:r:i:74;*/

	ud->mmpextras.config.target_file_len = strlen(ud->mmpextras.config.target_file = "vars.html");
	docgen_mmparse_data_comments(dg, mm, mm->config.debug_subject);/*jeanine:r:i:84;*/

	ud->mmpextras.config.target_file_len = strlen(ud->mmpextras.config.target_file = "enums.html");
	docgen_mmparse_enum_comments(dg, mm, mm->config.debug_subject);/*jeanine:r:i:85;*/

	ud->mmpextras.config.target_file_len = strlen(ud->mmpextras.config.target_file = "structs.html");
	docgen_mmparse_struct_comments(dg, mm, mm->config.debug_subject);/*jeanine:r:i:82;*/

	docgen_free_tmpbuf(tmpbuf_subject);
}
/*jeanine:p:i:75;p:88;a:r;x:672.62;*/
static
struct mmparse *docgen_new_mmparse(struct docgen *dg, struct docgen_mmp_data *mmpd, char *src, char *target, struct mmparse **blogs, int num_blogs)
{
	static struct mmpextras_shared mmpextras_shared_ud;

	struct docgen_mmparse_userdata *ud;
	struct mmparse *mm;

	if (!mmpextras_shared_ud.config.strpool) {
		assert(mmpextras_shared_ud.config.strpool = malloc(mmpextras_shared_ud.config.strpool_len = 5000));
	}
	assert(mm = malloc(sizeof(struct mmparse)));
	mm->config.directive_handlers = docgen_mmp_doc_directives;
	mm->config.modes = docgen_mmp_doc_modes;
	mm->config.dest.data0 = mmpd->d0;
	mm->config.dest.data1 = mmpd->d1;
	mm->config.dest.data2 = mmpd->d2;
	mm->config.dest.data3 = mmpd->d3;
	mm->config.dest.data0_len = mmpd->len0;
	mm->config.dest.data1_len = mmpd->len1;
	mm->config.dest.data2_len = mmpd->len2;
	mm->config.dest.data3_len = mmpd->len3;
	assert((mm->config.userdata = calloc(1, sizeof(struct docgen_mmparse_userdata))));
	ud = mm->config.userdata;
	ud->dg = dg;
	ud->blogs = blogs;
	ud->num_blogs = num_blogs;
	ud->mmpextras.shared = &mmpextras_shared_ud;
	if (src) {
		docgen_readfile(mm->config.debug_subject = src, &mm->config.source, &mm->config.source_len);
	}
	if (target) {
		ud->mmpextras.config.target_file = target;
		ud->mmpextras.config.target_file_len = strlen(target);
	}
	return mm;
}
static char *blogposts[] = {
	/*don't change the order of these*/
	/*each must start with 'blog-'*/
	"blog-docs-rework",
	"blog-exploring-UI",
	"blog-customizing-sponsor-cars",
	"blog-customizing-preset-cars",
	"blog-use-preset-cars-in-quickrace",
	NULL
};
static
void docgen_mmparse(struct docgen *dg, struct mmparse **out_mm_index, struct mmparse **out_mm_docs, struct mmparse ***out_blogs, int *out_num_blogs)
{
	struct mmparse *mm_index, *mm_docs, *mm_sym, **mm_blogs;
	char tmpbuf[200], **blogname, *blogfile;
	struct docgen_mmparse_userdata *ud;
	struct docgen_mmp_data mmpd;
	int len, num_blogs;

	assert(mmpd.d0 = malloc((mmpd.len0 = 1000000) + (mmpd.len1 = 10000) + (mmpd.len2 = 10000) + (mmpd.len3 = 10000)));
	mmpd.d1 = mmpd.d0 + mmpd.len0;
	mmpd.d2 = mmpd.d1 + mmpd.len1;
	mmpd.d3 = mmpd.d2 + mmpd.len2;

	/*mmparse blogposts, should be done first because blog directive reference doesn't use placeholders (for simplicity)*/
	for (num_blogs = 0, blogname = blogposts; *blogname; blogname++, num_blogs++);
	assert(mm_blogs = *out_blogs = malloc(sizeof(void*) * num_blogs));
	*out_num_blogs = num_blogs;
	for (blogname = blogposts; *blogname; blogname++) {
		blogfile = mmpd.d0;
		len = sprintf(tmpbuf, "BLOGx%02X-%s.html", blogname - blogposts, *blogname + 5) + 1;
		assert(((void) "increase mmpd.len0", mmpd.len0 > len));
		memcpy(mmpd.d0, tmpbuf, len);
		mmpd.d0 += len;
		mmpd.len0 -= len;
		sprintf(tmpbuf, "%s.txt", *blogname);
		*mm_blogs = docgen_new_mmparse(dg, &mmpd, tmpbuf, blogfile, NULL, 0);
		ud = (*mm_blogs)->config.userdata;
		ud->mmpextras.config.section.no_breadcrumbs = 0;
		ud->mmpextras.config.section.no_continuation_breadcrumbs = 0;
		ud->mmpextras.config.section.no_end_index_links = 0;
		ud->mmpextras.config.paragraphed.print_closing_tags = 0;
		ud->mmpextras.config.paragraphed.experimental_remove_last_lf = 1;
		ud->blogpost.htmlfile = blogfile;
		ud->blogpost.is_blogpost = 1;
		ud->blogpost.index = blogname - blogposts;
		mmparse(*mm_blogs);
		assert(ud->blogpost.title);
		assert(ud->blogpost.date);
		docgen_adjust_mmp_data_after_mmparse(&mmpd, *mm_blogs);/*jeanine:r:i:76;*/
		mm_blogs++;
	}
	mm_blogs = *out_blogs;

	/*mmparse index*/
	mm_index = docgen_new_mmparse(dg, &mmpd, "index.txt", "index.html", mm_blogs, num_blogs);
	ud = mm_index->config.userdata;
	ud->mmpextras.config.section.no_breadcrumbs = 1;
	ud->mmpextras.config.section.no_continuation_breadcrumbs = 1;
	ud->mmpextras.config.section.no_end_index_links = 1;
	ud->mmpextras.config.paragraphed.print_closing_tags = 0;
	ud->mmpextras.config.paragraphed.experimental_remove_last_lf = 1;
	mmparse(mm_index);
	docgen_adjust_mmp_data_after_mmparse(&mmpd, mm_index);/*jeanine:s:a:r;i:76;*/
	*out_mm_index = mm_index;

	/*mmparse docs*/
	mm_docs = docgen_new_mmparse(dg, &mmpd, "docs.txt", "docs.html", mm_blogs, num_blogs);
	mmparse(mm_docs);
	docgen_adjust_mmp_data_after_mmparse(&mmpd, mm_docs);/*jeanine:s:a:r;i:76;*/
	*out_mm_docs = mm_docs;

	/*mmparse funcs/structs/enums comments*/
	/*all their docs need to be parsed before those strucs etc are printed,
	because otherwise they may get printed without anchors that we only know are needed
	after parsing their docs.*/
	mm_sym = docgen_new_mmparse(dg, &mmpd, NULL, NULL, mm_blogs, num_blogs);
	mm_sym->config.directive_handlers = docgen_mmp_sym_directives;
	mm_sym->config.modes = docgen_mmp_sym_modes;
	ud = mm_sym->config.userdata;
	ud->mmpextras.config.href_output_immediately = 1;
	ud->mmpextras.config.paragraphed.print_closing_tags = 0;
	ud->mmpextras.config.paragraphed.experimental_remove_last_lf = 1;
	docgen_mmparse_all_symbol_comments(dg, mm_sym);/*jeanine:r:i:83;*/
}
/*jeanine:p:i:90;p:88;a:r;x:83.07;y:-70.73;*/
static
void docgen_readidc(struct docgen *dg)
{
	docgen_parseidc(dg->idcp);/*jeanine:r:i:24;*/
	docgen_collect_funcinfos(dg);/*jeanine:r:i:87;*/
	docgen_collect_structinfos(dg);/*jeanine:r:i:80;*/
	docgen_collect_enuminfos(dg);/*jeanine:r:i:81;*/
	docgen_collect_datainfos(dg);/*jeanine:r:i:78;*/
}
/*jeanine:p:i:88;p:0;a:b;x:136.11;y:14.38;*/
int main(int argc, char **argv)
{
	char *css, *name, *html_skel0, *html_skel1, *html_skel2, *cheatsheet, *cheatsheet_style_close_tag;
	int i, cheatsheet_len, css_len, html_skel0_len, html_skel1_len, html_skel2_len, num_blogs;
	struct mmparse *mm_index, *mm_docs, **mm_blogs;
	struct docgen_structinfo *structinfo;
	struct docgen_mmparse_userdata *ud;
	struct docgen_datainfo *datainfo;
	struct docgen_enuminfo *enuminfo;
	struct docgen_funcinfo *funcinfo;
	struct mmp_output_part *mmpart;
	struct docgen *dg;
	FILE *file;

	assert(dg = calloc(1, sizeof(struct docgen)));
	assert(dg->idcp = malloc(sizeof(struct idcparse)));
	docgen_readidc(dg);/*jeanine:r:i:90;*/
	docgen_mmparse(dg, &mm_index, &mm_docs, &mm_blogs, &num_blogs);/*jeanine:r:i:75;*/
	docgen_readfile("style.css", &css, &css_len);

	html_skel0 = "<!DOCTYPE html><html lang='en'><head><meta charset=utf-8>\n<title>";
	html_skel0_len = strlen(html_skel0);
	html_skel1 = "</title>\n<style>\n";
	html_skel1_len = strlen(html_skel1);
	html_skel2 =
		"</style></head><body>\n"
		"<header>\n"
		"<h1>nfsu2-re</h1>\n"
		"<p><a href='https://github.com/yugecin/nfsu2-re'>https://github.com/yugecin/nfsu2-re</a></p>\n"
		"<nav><a href='index.html'>home</a>\n"
		"<a href='blog.html'>blog</a>\n"
		"<a href='docs.html'>docs</a>\n"
		"<a href='funcs.html' class='func'>functions</a>\n"
		"<a href='structs.html' class='struc'>structs</a>\n"
		"<a href='enums.html' class='enum'>enums</a>\n"
		"<a href='vars.html' class='var'>variables</a>\n"
		"<a href='cheatsheet.html'>cheatsheet</a></nav>\n"
		"</header>\n";
	html_skel2_len = strlen(html_skel2);

	/*index*/
	mmparse_process_placeholders(mm_index);
	assert(file = fopen("index.html", "wb"));
	fwrite(html_skel0, html_skel0_len, 1, file);
	fwrite("nfsu2-re", 8, 1, file);
	fwrite(html_skel1, html_skel1_len, 1, file);
	fwrite(css, css_len, 1, file);
	fwrite(html_skel2, html_skel2_len, 1, file);
	fwrite("<div class='mm'>\n", 17, 1, file);
	for (mmpart = mm_index->output; mmpart->data0; mmpart++) {
		fwrite(mmpart->data0, mmpart->data0_len, 1, file);
		fwrite(mmpart->data1, mmpart->data1_len, 1, file);
	}
	fwrite("\n</div></body></html>\n", 22, 1, file);
	fclose(file);

	/*docs*/
	mmparse_process_placeholders(mm_docs);
	assert(file = fopen("docs.html", "wb"));
	fwrite(html_skel0, html_skel0_len, 1, file);
	fwrite("nfsu2-re/docs", 13, 1, file);
	fwrite(html_skel1, html_skel1_len, 1, file);
	fwrite(css, css_len, 1, file);
	fwrite(html_skel2, html_skel2_len, 1, file);
	fwrite("<div class='mm'>\n", 17, 1, file);
	for (mmpart = mm_docs->output; mmpart->data0; mmpart++) {
		fwrite(mmpart->data0, mmpart->data0_len, 1, file);
		fwrite(mmpart->data1, mmpart->data1_len, 1, file);
	}
	fwrite("\n</div></body></html>\n", 22, 1, file);
	fclose(file);

	/*blogindex*/
	assert(file = fopen("blog.html", "wb"));
	fwrite(html_skel0, html_skel0_len, 1, file);
	fwrite("nfsu2-re/blog", 13, 1, file);
	fwrite(html_skel1, html_skel1_len, 1, file);
	fwrite(css, css_len, 1, file);
	fwrite(html_skel2, html_skel2_len, 1, file);
	fwrite("<div><h2>Blog</h2><ul>\n", 23, 1, file);
	for (i = num_blogs; i-- > 0;) {
		ud = (mm_blogs[i])->config.userdata;
		fprintf(file,
			"<li><a href='%s'>%s (%s)</a></li>\n",
			ud->blogpost.htmlfile,
			ud->blogpost.title,
			ud->blogpost.date
		);
	}
	fwrite("</ul></div></body></html>\n", 26, 1, file);
	fclose(file);

	/*blogposts*/
	for (i = 0; i < num_blogs; i++) {
		mmparse_process_placeholders(mm_blogs[i]);
		ud = (mm_blogs[i])->config.userdata;
		assert(file = fopen(ud->blogpost.htmlfile, "wb"));
		fwrite(html_skel0, html_skel0_len, 1, file);
		fprintf(file, "nfsu2-re/BLOGx%02X: %s", ud->blogpost.index, ud->blogpost.title);
		fwrite(html_skel1, html_skel1_len, 1, file);
		fwrite(css, css_len, 1, file);
		fwrite(html_skel2, html_skel2_len, 1, file);
		fwrite("<div class='mm'>\n", 17, 1, file);
		fprintf(file, "<p><a href='blog.html'>blog</a> &gt; %s (%s)</p>", ud->blogpost.title, ud->blogpost.date);
		for (mmpart = mm_blogs[i]->output; mmpart->data0; mmpart++) {
			fwrite(mmpart->data0, mmpart->data0_len, 1, file);
			fwrite(mmpart->data1, mmpart->data1_len, 1, file);
		}
		fwrite("\n</div></body></html>\n", 22, 1, file);
		fclose(file);
	}

	/*cheatsheet*/
	assert(file = fopen("cheatsheet.html", "wb"));
	docgen_readfile("cheatsheet-bare.html", &cheatsheet, &cheatsheet_len);
	assert(cheatsheet_style_close_tag = strstr(cheatsheet, "</style>"));
	fwrite(html_skel0, html_skel0_len, 1, file);
	fwrite("nfsu2-re/cheatsheet", 19, 1, file);
	fwrite(html_skel1, html_skel1_len, 1, file);
	fwrite(css, css_len, 1, file);
	fwrite(cheatsheet + 7, cheatsheet_style_close_tag - cheatsheet - 7, 1, file);
	fwrite(html_skel2, html_skel2_len, 1, file);
	fwrite("<div>\n", 6, 1, file);
	fwrite(cheatsheet_style_close_tag + 8, cheatsheet_len - (cheatsheet_style_close_tag - cheatsheet) - 8, 1, file);
	fwrite("\n</div></body></html>\n", 22, 1, file);
	fclose(file);

	/*funcs (must be done before structs, so structs can be marked as classes and have member funcs)*/
	assert(file = fopen("funcs.html", "wb"));
	fwrite(html_skel0, html_skel0_len, 1, file);
	fwrite("nfsu2-re/funcs", 14, 1, file);
	fwrite(html_skel1, html_skel1_len, 1, file);
	fwrite(css, css_len, 1, file);
	fwrite(html_skel2, html_skel2_len, 1, file);
	fprintf(file, "%s%d%s", "<div class='func'>\n<h2>Functions (", dg->num_funcinfos, ")</h2>\n<ul>\n");
	for (i = 0, funcinfo = dg->funcinfos; i < dg->num_funcinfos; i++, funcinfo++) {
		fprintf(file, "<li><a href=#%X>\n%s</a>\n", funcinfo->func->addr, funcinfo->func->name);
	}
	fprintf(file, "%s", "</ul></div><div class='func sym'>");
	for (i = 0, funcinfo = dg->funcinfos; i < dg->num_funcinfos; i++, funcinfo++) {
		docgen_print_func(file, dg, funcinfo, funcinfo->func);/*jeanine:r:i:17;*/
	}
	fwrite("\n</div></body></html>\n", 22, 1, file);
	fclose(file);

	/*structs*/
	assert(file = fopen("structs.html", "wb"));
	fwrite(html_skel0, html_skel0_len, 1, file);
	fwrite("nfsu2-re/structs", 16, 1, file);
	fwrite(html_skel1, html_skel1_len, 1, file);
	fwrite(css, css_len, 1, file);
	fwrite(html_skel2, html_skel2_len, 1, file);
	fprintf(file, "%s%d%s", "<div class='struc'>\n<h2>Structs (", dg->num_structinfos, ")</h2>\n<ul>\n");
	for (i = 0, structinfo = dg->structinfos; i < dg->num_structinfos; i++, structinfo++) {
		name = structinfo->struc->name;
		fprintf(file, "<li><a href=#struc_%s>%s</a>", name, name);
		if (structinfo->struc->is_union) {
			fprintf(file, " <b></b>");
		}
		if (structinfo->is_class) {
			fprintf(file, " <em></em>");
		}
		fprintf(file, "\n");
	}
	fprintf(file, "%s", "</ul></div><div class='struc sym'>");
	for (i = 0, structinfo = dg->structinfos; i < dg->num_structinfos; i++, structinfo++) {
		docgen_print_struct(file, dg, structinfo, structinfo->struc);/*jeanine:r:i:5;*/
	}
	fwrite("\n</div></body></html>\n", 22, 1, file);
	fclose(file);

	/*datas*/
	assert(file = fopen("vars.html", "wb"));
	fwrite(html_skel0, html_skel0_len, 1, file);
	fwrite("nfsu2-re/vars", 13, 1, file);
	fwrite(html_skel1, html_skel1_len, 1, file);
	fwrite(css, css_len, 1, file);
	fwrite(html_skel2, html_skel2_len, 1, file);
	fprintf(file, "%s%d%s", "<div class='var'>\n<h2>Variables (", dg->num_datainfos, ")</h2>\n<ul>\n");
	for (i = 0, datainfo = dg->datainfos; i < dg->num_datainfos; i++, datainfo++) {
		fprintf(file, "<li><a href=#%X>\n%s</a>\n", datainfo->data->addr, datainfo->data->name);
	}
	fprintf(file, "%s", "</ul></div><div class='var sym'>");
	for (i = 0, datainfo = dg->datainfos; i < dg->num_datainfos; i++, datainfo++) {
		docgen_print_data(file, dg, datainfo, datainfo->data);/*jeanine:r:i:23;*/
	}
	fwrite("\n</div></body></html>\n", 22, 1, file);
	fclose(file);

	/*enums*/
	assert(file = fopen("enums.html", "wb"));
	fwrite(html_skel0, html_skel0_len, 1, file);
	fwrite("nfsu2-re/enums", 14, 1, file);
	fwrite(html_skel1, html_skel1_len, 1, file);
	fwrite(css, css_len, 1, file);
	fwrite(html_skel2, html_skel2_len, 1, file);
	fprintf(file, "%s%d%s", "<div class='enum'>\n<h2>Enums (", dg->num_enuminfos, ")</h2>\n<ul>\n");
	for (i = 0, enuminfo = dg->enuminfos; i < dg->num_enuminfos; i++, enuminfo++) {
		name = enuminfo->enu->name;
		fprintf(file, "<li><a href=#enu_%s>%s</a>\n", name, name);
	}
	fprintf(file, "%s", "</ul></div><div class='enum sym'>");
	for (i = 0, enuminfo = dg->enuminfos; i < dg->num_enuminfos; i++, enuminfo++) {
		docgen_print_enum(file, dg, enuminfo, enuminfo->enu);/*jeanine:r:i:22;*/
	}
	fwrite("\n</div></body></html>\n", 22, 1, file);
	fclose(file);

	return 0;
}
