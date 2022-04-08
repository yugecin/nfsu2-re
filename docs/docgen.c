/**
Instructs idcparse.c to parse the IDC file and then uses that to make
html documentation for the structs/enums/functions/data.
*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if 0
#define FREE(X) free(X)
#else
#define FREE(X)
#endif

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

struct docgen_structinfo {
	struct idcp_struct *struc;
	int name_len;
	/**struct is a class when at least one func's 'methodof' refers this struct*/
	char is_class;
};

struct docgen_funcinfo {
	struct idcp_stuff *func;
	int name_len;
	struct docgen_structinfo *methodof;
};

struct docgen_enuminfo {
	struct idcp_enum *enu;
	int name_len;
};

struct docgen_datainfo {
	struct idcp_stuff *data;
};

struct docgen {
	char struct_member_needs_anchor[IDCP_MAX_TOTAL_STRUCT_MEMBERS];
	struct idcparse *idcp;
	int num_structinfos;
	struct docgen_structinfo *structinfos;
	int num_funcinfos;
	struct docgen_funcinfo *funcinfos;
	int num_enuminfos;
	struct docgen_enuminfo *enuminfos;
	int num_datainfos;
	struct docgen_datainfo *datainfos;
};
/*jeanine:p:i:39;p:57;a:r;x:8.33;y:-9.94;*/
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
			printf("bad addr char '%c'\n", c);
			return -1;
		}
	}
	return res;
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

static
struct docgen_enuminfo* docgen_find_enum(struct docgen *dg, char *name)
{
	struct docgen_enuminfo *enuminfo;
	int i, len;

	len = strlen(name);
	for (i = 0, enuminfo = dg->enuminfos; i < dg->num_enuminfos; i++, enuminfo++) {
		if (len == enuminfo->name_len && !strcmp(name, enuminfo->enu->name)) {
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
int docgen_stricmp(char *a, char *b)
{
	register char d;
	char i, j;

nchr:
	i = *a;
	j = *b;
	if ('A' <= i && i <= 'Z') {
		i |= 0x20;
	}
	if ('A' <= j && j <= 'Z') {
		j |= 0x20;
	}
	d = i - j;
	if (d) {
		return d;
	}
	if (!i) {
		return -1;
	}
	if (!j) {
		return 1;
	}
	a++;
	b++;
	goto nchr;
}

static
void docgen_get_func_friendlyname(char *dest, char *name)
{
	char c;

	do {
		c = *(name++);
		if (c == ':' || c == '?' || c == '.') {
			c = '_';
		}
		*(dest++) = c;
	} while (c);
}
/*jeanine:p:i:57;p:56;a:r;x:3.33;*/
struct docgen_ref {
	struct idcp_struct_member *strucmember;
	struct idcp_struct *struc;
	struct idcp_stuff *func;
	struct idcp_stuff *data;
	struct idcp_enum *enu;
};

/**
When returning, none or one of the members of result will be set,
except if the ref is referencing a struct member, the both 'strucmember' and 'struc' will be set.
If none of the members are set, the reference couldn't be resolved.

@param ref should be zero terminated
*/
static
void docgen_resolve_ref(struct docgen *dg, struct docgen_ref *result, char *ref, int len)
{
	int min, max, idx, current, addr, num_members;
	struct docgen_structinfo *structinfo;
	struct docgen_enuminfo *enuminfo;
	struct idcp_struct_member *mem;
	struct idcp_struct *struc;
	struct idcp_stuff *stuff;
	char *plus;

	memset(result, 0, sizeof(struct docgen_ref));

	if (len > 7 && !strncmp("struct ", ref, 7)) {
		plus = strstr(ref + 7, "+");
		if (plus) {
			structinfo = docgen_find_struct(dg, ref + 7, (plus - ref) - 7);
		} else {
			structinfo = docgen_find_struct(dg, ref + 7, len - 7);
		}
		if (structinfo) {
			result->struc = structinfo->struc;
			if (plus) {
				addr = docgen_parse_addr(plus + 1, len - (plus - ref) - 1);/*jeanine:s:a:r;i:39;*/
				if (addr == -1) {
					result->struc = NULL;
					return;
				}
				struc = structinfo->struc;
				num_members = struc->end_idx - struc->start_idx;
				mem = dg->idcp->struct_members + struc->start_idx;
				for (; num_members; num_members--, mem++) {
					if (mem->offset == addr) {
						dg->struct_member_needs_anchor[mem - dg->idcp->struct_members] = 1;
						result->strucmember = mem;
					}
				}
			}
		}
		return;
	} else if (len > 5 && !strncmp("enum ", ref, 5)) {
		enuminfo = docgen_find_enum(dg, ref + 5);
		if (enuminfo) {
			result->enu = enuminfo->enu;
		}
	} else {
		/*Binary search will work as long as the 'new_addr > addr'
		assertion in 'idcp_get_or_allocate_stuff' still stands.*/
		addr = docgen_parse_addr(ref, len);/*jeanine:r:i:39;*/
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
					printf("resolved ref '%s' has no name, ignoring\n", ref);
				} else if (stuff->type == IDCP_STUFF_TYPE_FUNC) {
					result->func = stuff;
				} else if (stuff->type == IDCP_STUFF_TYPE_DATA) {
					result->data = stuff;
				} else {
					printf("unknown type '%d' when resolving ref '%s'\n", stuff->type, ref);
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
/*jeanine:p:i:25;p:0;a:b;x:115.56;y:11.69;*/
#include "mmparse.c"
#include "mmparse_extras.c"

struct docgen_mmparse_userdata {
	struct docgen *dg;
	struct mmpextras_userdata mmpextras;
};

static
struct mmpextras_userdata *mmpextras_get_userdata(struct mmparse *mm)
{
	return &((struct docgen_mmparse_userdata*) mm->config.userdata)->mmpextras;
}
/*jeanine:p:i:12;p:11;a:r;x:70.62;y:-79.59;*/
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
				bufp += sprintf(bufp, "<a href='structs.html#struc_%s'>struct %s</a>", name, name);
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
			if (docgen_find_enum(dg, name)) {
				bufp += sprintf(bufp, "<a href='enums.html#enu_%s'>enum %s</a>", name, name);
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
/*jeanine:p:i:1;p:24;a:r;x:17.96;y:9.75;*/
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
/*jeanine:p:i:24;p:58;a:r;x:15.56;y:-26.00;*/
static
void docgen_parseidc(struct idcparse *idcp)
{
	char *buf;
	int length;

	docgen_readfile(IDCPATH, &buf, &length);/*jeanine:r:i:1;*/
	if (length > 2 && buf[0] == (char) 0xEF) {
		/*assume UTF8 BOM*/
		idcparse(idcp, buf + 3, length - 3);
	} else {
		idcparse(idcp, buf, length);
	}
	FREE(buf);
}
/*jeanine:p:i:8;p:58;a:r;x:15.56;y:-9.00;*/
static
char* docgen_readcss()
{
	char *buf, *a, *b, *end;
	register char c;
	int length;

	docgen_readfile("style.css", &buf, &length);/*jeanine:s:a:r;i:1;*/
	a = b = buf;
	end = buf + length;
	while (b != end) {
		c = *b;
		if (c != '\t') {
			if (!c) {
				break;
			}
			*a = c;
			a++;
		}
		b++;
	}
	*a = 0;
	return buf;
}
/*jeanine:p:i:15;p:58;a:r;x:15.56;y:2.00;*/
static
int docgen_func_sort_compar(const void *_a, const void *_b)
{
	register const struct idcp_stuff *a = ((struct docgen_funcinfo*) _a)->func;
	register const struct idcp_stuff *b = ((struct docgen_funcinfo*) _b)->func;

	return docgen_stricmp(a->name, b->name);
}
/*jeanine:p:i:19;p:17;a:r;x:44.11;y:-62.81;*/
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
			b += sprintf(b, "<a href='structs.html#struc_%s'>%s</a>", classname, classname);
			/*Check that the 'this' argument is of the expected type.*/
			if (func->data.func.type) {
				checkbuf = docgen_get_tmpbuf(10000);
				sprintf(checkbuf->data, "%s(struct %s *this", friendlyname, classname);
				if (!strstr(originalsignature, checkbuf->data)) {
					printf("warn: func %X '%s' has wrong thisarg (searching '%s')\n", func->addr, originalname, checkbuf->data);
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
		/*Quickly check if there should have been a coloncolon*/
		thiscall = strstr(originalsignature, "__thiscall");
		if (thiscall && thiscall - originalsignature < len) {
			printf("warn: func %X is __thiscall but no '::'\n", func->addr);
		}
	}
	b += sprintf(b, "</h3>");
	/*Copy rest of signature*/
	strcpy(b, namepos + funcinfo->name_len);
	docgen_free_tmpbuf(*signaturebuf);
	*signaturebuf = newbuf;

	if (docgen_link_structs_enums(dg, signaturebuf)) {/*jeanine:s:a:r;i:12;*/
		printf("warn: func '%X %s' references an unknown struct/enum\n", func->addr, func->name);
	}
}
/*jeanine:p:i:17;p:58;a:r;x:15.56;y:-72.00;*/
static
void docgen_print_func(FILE *f, struct docgen *dg, struct docgen_funcinfo *funcinfo, struct idcp_stuff *func)
{
	struct docgen_tmpbuf *signaturebuf;

	signaturebuf = docgen_get_tmpbuf(10000);
	docgen_gen_func_signature(&signaturebuf, dg, funcinfo, func);/*jeanine:r:i:19;*/
	fprintf(f, "<pre id='%X'><i>%X</i> %s</pre>", func->addr, func->addr, signaturebuf->data);
	docgen_free_tmpbuf(signaturebuf);

	if (func->comment) {
		/*TODO: mmparse*/
		fprintf(f, "<p>%s</p>\n", func->comment);
	}
	if (func->rep_comment) {
		/*TODO: mmparse*/
		fprintf(f, "<p>%s</p>\n", func->rep_comment);
	}
}
/*jeanine:p:i:4;p:58;a:r;x:15.56;y:4.00;*/
static
int docgen_struct_sort_compar(const void *_a, const void *_b)
{
	register const struct idcp_struct *a = ((struct docgen_structinfo*) _a)->struc;
	register const struct idcp_struct *b = ((struct docgen_structinfo*) _b)->struc;

	return docgen_stricmp(a->name, b->name);
}
/*jeanine:p:i:9;p:11;a:r;x:70.36;y:9.66;*/
static
void docgen_format_struct_member_typeandname_when_enum(char *buf, struct idcparse *idcp, struct idcp_struct_member *mem)
{
	/*TODO: what if it's an array*/

	struct idcp_enum *en;

	assert(0 <= mem->typeid && mem->typeid < idcp->num_enums);
	en = idcp->enums + mem->typeid;
	buf += sprintf(buf, "<a href='enums.html#enum_%s'>enum %s</a> %s", en->name, en->name, mem->name);
}
/*jeanine:p:i:13;p:11;a:r;x:70.61;y:23.33;*/
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
	buf += sprintf(buf, "<a href='#struc_%s'>struct %s</a> %s", struc->name, struc->name, mem->name);
	/*member struct type can't be an empty struct*/
	assert((struc_size = docgen_get_struct_size(idcp, struc)));
	if (mem->nbytes > struc_size) {
		assert(!(mem->nbytes % struc_size));
		sprintf(buf, "[%d]", mem->nbytes / struc_size);
	}
}
/*jeanine:p:i:10;p:11;a:r;x:69.72;y:45.30;*/
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
/*jeanine:p:i:11;p:5;a:r;x:3.33;*/
static
void docgen_print_struct_member(FILE *f, struct docgen *dg, struct idcp_struct *struc, int offset, int size, struct idcp_struct_member *mem)
{
	struct docgen_tmpbuf *typeandname;
	int offsetstrlen, isplaceholder;
	char offsetstr[16];

	memset(offsetstr, ' ', 8);
	offsetstrlen = sprintf(offsetstr, "/*%X*/", offset);
	if (offsetstrlen < 8) {
		offsetstr[offsetstrlen] = '\t';
		offsetstr[offsetstrlen+1] = 0;
	}

	typeandname = docgen_get_tmpbuf(10000);
	if (mem) {
		if (dg->struct_member_needs_anchor[mem - dg->idcp->struct_members]) {
			fprintf(f, "<i id='struc_%s%X'></i>", struc->name, mem->offset);
		}
		isplaceholder = !strncmp(mem->name, "field_", 6) || !strncmp(mem->name, "floatField_", 11);
		if (IDC_is_enum(mem->flag)) {
			docgen_format_struct_member_typeandname_when_enum(typeandname->data, dg->idcp, mem);/*jeanine:r:i:9;*/
		} else if (IDC_is_struct(mem->flag)) {
			docgen_format_struct_member_typeandname_when_struct(typeandname->data, dg->idcp, mem);/*jeanine:r:i:13;*/
		} else if (mem->type) {
			docgen_format_struct_member_typeandname_when_type(typeandname->data, mem);/*jeanine:r:i:10;*/
			if (docgen_link_structs_enums(dg, &typeandname)) {/*jeanine:r:i:12;*/
				printf("warn: struct '%s' member '%s' references an unknown struct/enum\n", struc->name, mem->name);
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
		assert(!mem->comment);
		if (mem->rep_comment) {
			fprintf(f, "<span>/**%s*/</span>\n", mem->rep_comment); /*TODO: mmparse*/
		}
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
/*jeanine:p:i:5;p:58;a:r;x:15.56;y:-78.00;*/
static
void docgen_print_struct(FILE *f, struct docgen *dg, struct docgen_structinfo *structinfo, struct idcp_struct *struc)
{
	int num_members, size, lastoffset, i;
	struct docgen_funcinfo *funcinfo;
	struct idcp_struct_member *mem;
	struct idcp_stuff *func;

	num_members = struc->end_idx - struc->start_idx;
	size = docgen_get_struct_size(dg->idcp, struc);

	fprintf(f, "<pre id='struc_%s'>%s <h3>%s</h3> { <i>/*%d members, size %Xh*/</i>%s\n",
		struc->name,
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
	fprintf(f, "};%s", "</pre>");
	if (struc->comment) {
		/*TODO: mmparse*/
		fprintf(f, "<p>%s</p>", struc->comment);
	}
	if (struc->rep_comment) {
		/*TODO: mmparse*/
		fprintf(f, "<p>%s</p>", struc->rep_comment);
	}
	if (structinfo->is_class) {
		fprintf(f, "<p><strong>Methods:</strong></p><ul>");
		for (i = 0, funcinfo = dg->funcinfos; i < dg->num_funcinfos; i++, funcinfo++) {
			if (funcinfo->methodof == structinfo) {
				func = funcinfo->func;
				fprintf(f, "<li><pre><i>%X</i> <a href='funcs.html#%X' title='%s'>%s</a></pre></li>",
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
/*jeanine:p:i:22;p:58;a:r;x:15.56;y:-44.00;*/
static
void docgen_print_enum(FILE *f, struct docgen *dg, struct docgen_enuminfo *enuminfo, struct idcp_enum *enu)
{
	struct idcp_enum_member *mem;
	int num_members;

	num_members = enu->end_idx - enu->start_idx;

	fprintf(f, "<pre id='enu_%s'>enum <h3>%s</h3> { <i>/*%d members*/</i>\n", enu->name, enu->name, num_members);
	mem = dg->idcp->enum_members + enu->start_idx;
	for (; num_members; num_members--, mem++) {
		if (mem->comment) {
			if (mem->rep_comment) {
				/*TODO: mmparse*/
				fprintf(f, "<span>/**%s\n\n%s*/</span>\n", mem->comment, mem->rep_comment);
			} else {
				/*TODO: mmparse*/
				fprintf(f, "<span>/**%s*/</span>\n", mem->comment);
			}
		} else if (mem->rep_comment) {
			/*TODO: mmparse*/
			fprintf(f, "<span>/**%s*/</span>\n", mem->rep_comment);
		}
		fprintf(f, "\t%s = 0x%X,\n", mem->name, mem->value);
	}
	fprintf(f, "};%s", "</pre>");
	if (enu->comment) {
		/*TODO: mmparse*/
		fprintf(f, "<p>%s</p>", enu->comment);
	}
	if (enu->rep_comment) {
		/*TODO: mmparse*/
		fprintf(f, "<p>%s</p>", enu->rep_comment);
	}
}
/*jeanine:p:i:21;p:58;a:r;x:15.56;y:6.00;*/
static
int docgen_enum_sort_compar(const void *_a, const void *_b)
{
	register const struct idcp_enum *a = ((struct docgen_enuminfo*) _a)->enu;
	register const struct idcp_enum *b = ((struct docgen_enuminfo*) _b)->enu;

	return docgen_stricmp(a->name, b->name);
}
/*jeanine:p:i:23;p:58;a:r;x:15.56;y:-26.00;*/
static
void docgen_print_data(FILE *f, struct docgen *dg, struct docgen_datainfo *datainfo, struct idcp_stuff *data)
{
	struct docgen_tmpbuf *type;
	char unconfirmed_type, *st;

	type = docgen_get_tmpbuf(10000);
	unconfirmed_type = 0;
	if (data->data.data.struct_type) {
		st = data->data.data.struct_type;
		if (docgen_find_struct(dg, st, strlen(st))) {
			sprintf(type->data, "<a href='structs.html#struc_%s'>struct %s</a>", st, st);
		} else {
			printf("warn: cannot find struct '%s' for var %X '%s'\n", st, data->addr, data->name);
			sprintf(type->data, "<strong>struct %s<strong>", st);
		}
	} else if (data->data.data.flags & IDCP_DATA_FLOAT) {
		assert(data->data.data.size == 4);
		strcpy(type->data, "float");
	} else if (data->data.data.flags & IDCP_DATA_DOUBLE) {
		assert(data->data.data.size == 8);
		strcpy(type->data, "double");
	} else {
		switch (data->data.data.size) {
		case 1:
			strcpy(type->data, "char");
			break;
		case 2:
			strcpy(type->data, "short");
			break;
		case 4:
			strcpy(type->data, "int");
			break;
		default:
			sprintf(type->data, "i%d", data->data.data.size * 8);
			break;
		};
		unconfirmed_type = 1;
	}
	fprintf(f, "<pre id='%X'><i>%X</i> %s <h3>%s</h3>", data->addr, data->addr, type->data, data->name);
	docgen_free_tmpbuf(type);
	if (data->data.data.arraysize) {
		fprintf(f, "[%d]", data->data.data.arraysize);
	}
	if (unconfirmed_type) {
		fprintf(f, " <u></u></pre>");
	}
	fprintf(f, "</pre>");
	if (data->comment) {
		/*TODO: mmparse*/
		fprintf(f, "<p>%s</p>", data->comment);
	}
	if (data->rep_comment) {
		/*TODO: mmparse*/
		fprintf(f, "<p>%s</p>", data->rep_comment);
	}
}
/*jeanine:p:i:56;p:37;a:r;x:55.13;y:24.28;*/
/**
@param ref must be zero terminated
*/
static
void docgen_append_ref_text(struct mmparse *mm, void (*append_func)(struct mmparse*,char*,int), char *ref, int reflen)
{
	struct docgen_mmparse_userdata *ud;
	struct docgen_ref res;
	char addr[16];
	int len;

	ud = mm->config.userdata;
	docgen_resolve_ref(ud->dg, &res, ref, reflen);/*jeanine:r:i:57;*/
	if (res.func) {
		len = sprintf(addr, "%X", res.func->addr);
		append_func(mm, "<a class='func' href='funcs.html#", 33);
		append_func(mm, addr, len);
		append_func(mm, "'>", 2);
		append_func(mm, res.func->name, strlen(res.func->name));
		append_func(mm, "</a>", 4);
	} else if (res.data) {
		len = sprintf(addr, "%X", res.data->addr);
		append_func(mm, "<a class='var' href='vars.html#", 31);
		append_func(mm, addr, len);
		append_func(mm, "'>", 2);
		append_func(mm, res.data->name, strlen(res.data->name));
		append_func(mm, "</a>", 4);
	} else if (res.strucmember) {
		append_func(mm, "<a class='struc' href='structs.html#struc_", 42);
		append_func(mm, res.struc->name, strlen(res.struc->name));
		len = sprintf(addr, "%X", res.strucmember->offset);
		append_func(mm, addr, len);
		append_func(mm, "'>", 2);
		append_func(mm, res.strucmember->name, strlen(res.strucmember->name));
		append_func(mm, "</a>", 4);
	} else if (res.struc) {
		len = strlen(res.struc->name);
		append_func(mm, "<a class='struc' href='structs.html#struc_", 42);
		append_func(mm, res.struc->name, len);
		append_func(mm, "'>struct ", 9);
		append_func(mm, res.struc->name, len);
		append_func(mm, "</a>", 4);
	} else if (res.enu) {
		len = strlen(res.enu->name);
		append_func(mm, "<a class='enum' href='enums.html#enu_", 37);
		append_func(mm, res.enu->name, len);
		append_func(mm, "'>enum ", 7);
		append_func(mm, res.enu->name, len);
		append_func(mm, "</a>", 4);
	} else {
		mmparse_failmsgf(mm, "unknown ref '%s'", ref);
		append_func(mm, "<strong class='error' style='font-family:monospace'>?", 53);
		append_func(mm, ref, reflen);
		append_func(mm, "?</strong>", 10);
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
	mmparse_append_to_main_output(mm, "</ul>", 5);
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
/*jeanine:p:i:30;p:58;a:r;x:33.75;y:-120.00;*/
struct mmp_mode mmparse_mode_symbols = {
	mmparse_cb_mode_symbols_start,/*jeanine:r:i:35;*/
	mmparse_cb_mode_symbols_println,/*jeanine:r:i:37;*/
	mmparse_cb_mode_symbols_end,/*jeanine:r:i:36;*/
	mmparse_cb_mode_symbols_directive,/*jeanine:r:i:38;*/
	"symbols",
	MMPARSE_DO_PARSE_LINES
};
/*jeanine:p:i:47;p:58;a:r;x:35.69;y:-38.70;*/
struct mmp_mode mmparse_mode_ida = {
	mmparse_cb_mode_ida_start,/*jeanine:r:i:51;*/
	mmparse_cb_mode_ida_println,/*jeanine:r:i:52;*/
	mmpextras_cb_mode_pre_end,
	mmparse_cb_mode_ida_directive,/*jeanine:r:i:54;*/
	"ida",
	MMPARSE_DO_PARSE_LINES
};
/*jeanine:p:i:59;p:42;a:r;x:31.40;*/
static
void docgen_mmparse_dir_ref_append(struct mmparse *mm, char *buf, int len)
{
	mmparse_append_to_expanded_line(mm, buf, len);
}
/*jeanine:p:i:42;p:58;a:r;x:31.85;y:-73.41;*/
static
enum mmp_dir_content_action mmparse_dir_ref(struct mmparse *mm, struct mmp_dir_content_data *data)
{
	docgen_append_ref_text(mm, docgen_mmparse_dir_ref_append, data->contents, data->content_len);/*jeanine:r:i:59;:s:a:r;i:56;*/
	return DELETE_CONTENTS;
}
/*jeanine:p:i:58;p:25;a:b;y:1.88;*/
int main(int argc, char **argv)
{
	struct mmp_dir_handler mmdirectivehandlers[] = {
		{ "index", mmpextras_dir_index },
		{ "href", mmpextras_dir_href },
		{ "ref", mmparse_dir_ref },/*jeanine:r:i:42;*/
		{ "h", mmpextras_dir_h },
		{ NULL, NULL }
	};
	struct mmp_mode *mmmodes[] = {
		&mmparse_mode_normal, /*must be first*/
		&mmpextras_mode_section,
		&mmparse_mode_symbols,/*jeanine:r:i:30;*/
		&mmparse_mode_plain,
		&mmparse_mode_nop,
		&mmparse_mode_ida,/*jeanine:r:i:47;*/
		&mmpextras_mode_pre,
		&mmpextras_mode_ul,
		NULL
	};
	FILE *f_structs, *f_funcs, *f_enums, *f_datas, *f_index, *f_cheatsheet;
	char *css, *name, *header, *cheatsheet;
	struct docgen_structinfo *structinfo;
	struct docgen_datainfo *datainfo;
	struct docgen_enuminfo *enuminfo;
	struct docgen_funcinfo *funcinfo;
	struct mmp_output_part *mmpart;
	int i, j, cheatsheet_len;
	struct idcparse *idcp;
	struct mmparse *mm;
	struct docgen *dg;

	idcp = malloc(sizeof(struct idcparse));
	assert(((void)"failed to malloc for idcparse", idcp));
	docgen_parseidc(idcp);/*jeanine:r:i:24;*/

	css = docgen_readcss();/*jeanine:r:i:8;*/

	dg = malloc(sizeof(struct docgen));
	assert(((void)"failed to malloc for docgen", dg));
	memset(dg->struct_member_needs_anchor, 0, sizeof(dg->struct_member_needs_anchor));
	dg->idcp = idcp;
	/*read functions*/
	dg->funcinfos = malloc(sizeof(struct docgen_funcinfo) * idcp->num_funcs);
	assert(((void)"failed to malloc for dg->funcinfos", dg->funcinfos));
	for (dg->num_funcinfos = 0, j = 0; j < idcp->num_stuffs; j++) {
		if (idcp->stuffs[j].type == IDCP_STUFF_TYPE_FUNC && idcp->stuffs[j].name) {
			dg->funcinfos[dg->num_funcinfos].func = idcp->stuffs + j;
			dg->funcinfos[dg->num_funcinfos].name_len = strlen(idcp->stuffs[j].name);
			dg->num_funcinfos++;
		}
	}
	qsort((void*) dg->funcinfos, dg->num_funcinfos, sizeof(struct docgen_funcinfo), docgen_func_sort_compar);/*jeanine:r:i:15;*/
	/*read structs*/
	dg->structinfos = malloc(sizeof(struct docgen_structinfo) * idcp->num_structs);
	assert(((void)"failed to malloc for dg->structinfos", dg->structinfos));
	for (i = 0; i < idcp->num_structs; i++) {
		dg->structinfos[i].struc = idcp->structs + i;
		dg->structinfos[i].name_len = strlen(idcp->structs[i].name);
		dg->structinfos[i].is_class = 0;
	}
	dg->num_structinfos = idcp->num_structs;
	qsort((void*) dg->structinfos, dg->num_structinfos, sizeof(struct docgen_structinfo), docgen_struct_sort_compar);/*jeanine:r:i:4;*/
	/*read enums*/
	dg->enuminfos = malloc(sizeof(struct docgen_enuminfo) * idcp->num_enums);
	assert(((void)"failed to malloc for dg->enuminfos", dg->enuminfos));
	for (i = 0; i < idcp->num_enums; i++) {
		dg->enuminfos[i].enu = idcp->enums + i;
		dg->enuminfos[i].name_len = strlen(idcp->enums[i].name);
	}
	dg->num_enuminfos = idcp->num_enums;
	qsort((void*) dg->enuminfos, dg->num_enuminfos, sizeof(struct docgen_enuminfo), docgen_enum_sort_compar);/*jeanine:r:i:21;*/
	/*read datas (not sorting these...)*/
	dg->datainfos = malloc(sizeof(struct docgen_datainfo) * idcp->num_datas);
	assert(((void)"failed to malloc for dg->datainfos", dg->datainfos));
	for (dg->num_datainfos = 0, j = 0; j < idcp->num_stuffs; j++) {
		if (idcp->stuffs[j].type == IDCP_STUFF_TYPE_DATA && idcp->stuffs[j].name) {
			dg->datainfos[dg->num_datainfos].data = idcp->stuffs + j;
			dg->num_datainfos++;
		}
	}

	header = "<header>\n"
		"<h1>nfsu2-re</h1>\n"
		"<p><a href='https://github.com/yugecin/nfsu2-re'>https://github.com/yugecin/nfsu2-re</a></p>\n"
		"<nav><a href='index.html'>Home</a>\n"
		"<a href='funcs.html' class='func'>functions</a>\n"
		"<a href='structs.html' class='struc'>structs</a>\n"
		"<a href='enums.html' class='enum'>enums</a>\n"
		"<a href='vars.html' class='var'>variables</a>\n"
		"<a href='cheatsheet.html'>cheatsheet</a></nav>\n"
		"</header>\n";

	/*mmparse things*/
	mm = malloc(sizeof(struct mmparse));
	assert(((void)"failed to malloc for mm", mm));
	docgen_readfile(mm->config.debug_subject = "index-source.txt", &mm->config.source, &mm->config.source_len);
	mm->config.directive_handlers = mmdirectivehandlers;
	mm->config.modes = mmmodes;
	assert((mm->config.userdata = calloc(1, sizeof(struct docgen_mmparse_userdata))));
	assert((mm->config.dest.data0 = malloc(
		(mm->config.dest.data0_len = 1000000) +
		(mm->config.dest.data1_len = 10000) +
		(mm->config.dest.data2_len = 40000) +
		(mm->config.dest.data3_len = 10000)
	)));
	mm->config.dest.data1 = mm->config.dest.data0 + mm->config.dest.data0_len;
	mm->config.dest.data2 = mm->config.dest.data1 + mm->config.dest.data1_len;
	mm->config.dest.data3 = mm->config.dest.data2 + mm->config.dest.data2_len;
	((struct docgen_mmparse_userdata*) mm->config.userdata)->dg = dg;
	mmparse(mm);
	mmparse_process_placeholders(mm);
	f_index = fopen("index.html", "wb");
	assert(((void)"failed to open file index.html for writing", f_index));
	fprintf(f_index,
		"%s%s%s%s%s",
		"<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'/><title>nfsu2-re</title><style>",
		css,
		"</style></head><body>",
		header,
		"<div class='mm'>\n"
	);
	for (mmpart = mm->output; mmpart->data0; mmpart++) {
		fwrite(mmpart->data0, mmpart->data0_len, 1, f_index);
		fwrite(mmpart->data1, mmpart->data1_len, 1, f_index);
	}
	fprintf(f_index, "%s", "\n</div></body></html>");
	fclose(f_index);

	/*cheatsheet*/
	f_cheatsheet = fopen("cheatsheet.html", "wb");
	assert(((void)"failed to open file cheatsheet.html for writing", f_cheatsheet));
	docgen_readfile("cheatsheet-bare.html", &cheatsheet, &cheatsheet_len);
	fprintf(f_cheatsheet,
		"%s%s%s%s%s",
		"<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'/><title>nfsu2-re/cheatsheet</title><style>",
		css,
		"</style></head><body>",
		header,
		"<div>\n"
	);
	fwrite(cheatsheet, cheatsheet_len, 1, f_cheatsheet);
	fprintf(f_cheatsheet, "%s", "\n</div></body></html>");
	fclose(f_cheatsheet);

	/*funcs*/
	f_funcs = fopen("funcs.html", "wb");
	assert(((void)"failed to open file funcs.html for writing", f_funcs));
	fprintf(f_funcs,
		"%s%s%s%s",
		"<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'/><title>nfsu2-re/funcs</title><style>",
		css,
		"</style></head><body>",
		header
	);
	fprintf(f_funcs, "%s%d%s", "<div class='func'><h2>Functions (", dg->num_funcinfos, ")</h2><ul>\n");
	for (i = 0, funcinfo = dg->funcinfos; i < dg->num_funcinfos; i++, funcinfo++) {
		fprintf(f_funcs, "<li><a href='#%X'>%s</a></li>\n", funcinfo->func->addr, funcinfo->func->name);
	}
	fprintf(f_funcs, "%s", "</ul></div><div class='func'>");
	for (i = 0, funcinfo = dg->funcinfos; i < dg->num_funcinfos; i++, funcinfo++) {
		docgen_print_func(f_funcs, dg, funcinfo, funcinfo->func);/*jeanine:r:i:17;*/
	}
	fprintf(f_funcs, "%s", "</div></body></html>");
	fclose(f_funcs);

	/*structs*/
	f_structs = fopen("structs.html", "wb");
	assert(((void)"failed to open file structs.html for writing", f_structs));
	fprintf(f_structs,
		"%s%s%s%s",
		"<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'/><title>nfsu2-re/structs</title><style>",
		css,
		"</style></head><body>",
		header
	);
	fprintf(f_structs, "%s%d%s", "<div class='struc'><h2>Structs (", dg->num_structinfos, ")</h2><ul>\n");
	for (i = 0, structinfo = dg->structinfos; i < dg->num_structinfos; i++, structinfo++) {
		name = structinfo->struc->name;
		fprintf(f_structs, "<li><a href='#struc_%s'>%s</a>", name, name);
		if (structinfo->struc->is_union) {
			fprintf(f_structs, " <b></b>");
		}
		if (structinfo->is_class) {
			fprintf(f_structs, " <em></em>");
		}
		fprintf(f_structs, "</li>\n");
	}
	fprintf(f_structs, "%s", "</ul></div><div class='struc'>");
	for (i = 0, structinfo = dg->structinfos; i < dg->num_structinfos; i++, structinfo++) {
		docgen_print_struct(f_structs, dg, structinfo, structinfo->struc);/*jeanine:r:i:5;*/
	}
	fprintf(f_structs, "%s", "</div></body></html>");
	fclose(f_structs);

	/*enums*/
	f_enums = fopen("enums.html", "wb");
	assert(((void)"failed to open file enums.html for writing", f_enums));
	fprintf(f_enums,
		"%s%s%s%s",
		"<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'/><title>nfsu2-re/enums</title><style>",
		css,
		"</style></head><body>",
		header
	);
	fprintf(f_enums, "%s%d%s", "<div class='enum'><h2>Enums (", dg->num_enuminfos, ")</h2><ul>\n");
	for (i = 0, enuminfo = dg->enuminfos; i < dg->num_enuminfos; i++, enuminfo++) {
		name = enuminfo->enu->name;
		fprintf(f_enums, "<li><a href='#enu_%s'>%s</a></li>\n", name, name);
	}
	fprintf(f_enums, "%s", "</ul></div><div class='enum'>");
	for (i = 0, enuminfo = dg->enuminfos; i < dg->num_enuminfos; i++, enuminfo++) {
		docgen_print_enum(f_enums, dg, enuminfo, enuminfo->enu);/*jeanine:r:i:22;*/
	}
	fprintf(f_enums, "%s", "</div></body></html>");
	fclose(f_enums);

	/*datas*/
	f_datas = fopen("vars.html", "wb");
	assert(((void)"failed to open file vars.html for writing", f_datas));
	fprintf(f_datas,
		"%s%s%s%s",
		"<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'/><title>nfsu2-re/vars</title><style>",
		css,
		"</style></head><body>",
		header
	);
	fprintf(f_datas, "%s%d%s", "<div class='var'><h2>Variables (", dg->num_datainfos, ")</h2><ul>\n");
	for (i = 0, datainfo = dg->datainfos; i < dg->num_datainfos; i++, datainfo++) {
		fprintf(f_datas, "<li><a href='#%X'>%s</a></li>\n", datainfo->data->addr, datainfo->data->name);
	}
	fprintf(f_datas, "%s", "</ul></div><div class='var'>");
	for (i = 0, datainfo = dg->datainfos; i < dg->num_datainfos; i++, datainfo++) {
		docgen_print_data(f_datas, dg, datainfo, datainfo->data);/*jeanine:r:i:23;*/
	}
	fprintf(f_datas, "%s", "</div></body></html>");
	fclose(f_datas);

	return 0;
}
