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

#define DOCGEN_WARN_UNKNOWN_STRUCT_REFERENCES

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
};

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

/*jeanine:p:i:14;p:0;a:b;x:-78.78;y:11.75;*/
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
			return b;
		}
	}
	assert(((void) "failed to allocate tmpbuf, try increasing DOCGEN_MAX_TMPBUFS", 0));
}

static
void docgen_free_tmpbuf(struct docgen_tmpbuf *buf)
{
	buf->used = 0;
}
/*jeanine:p:i:12;p:11;a:r;x:6.67;y:24.00;*/
/**
@param tmpbuf might get swapped with a different buffer
@return nonzero if there were unknown structs
*/
static
int docgen_mark_unknown_structs(struct docgen_tmpbuf **tmpbuf)
{
	char *bufp, *sub, *end, *strp, c;
	struct docgen_tmpbuf *newbuf, *original;

	original = *tmpbuf;
	sub = strstr(original->data, "struct #");
	if (sub) {
		newbuf = docgen_get_tmpbuf(original->size);
		bufp = newbuf->data;
		strp = original->data;
		do {
			memcpy(bufp, strp, sub - strp);
			bufp += sub - strp;
			memcpy(bufp, "<strong>", 8);
			bufp += 8;
			end = sub + 8;
			do {
				end++;
				c = *end;
			} while ('0' <= c && c <= '9');
			memcpy(bufp, sub, end - sub);
			bufp += end - sub;
			memcpy(bufp, "</strong>", 9);
			bufp += 9;
			strp = end;
			sub = strstr(strp, "struct #");
		} while (sub);
		*bufp = 0;
		strcat(bufp, strp);
		*tmpbuf = newbuf;
		docgen_free_tmpbuf(original);
		return 1;
	}
	return 0;
}
/*jeanine:p:i:6;p:5;a:r;x:287.66;*/
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
/*jeanine:p:i:1;p:7;a:r;x:6.67;y:-42.00;*/
static
void docgen_parseidc(struct idcparse *idcp)
{
	FILE *in;
	char *buf;
	int size;

	in = fopen(IDCPATH, "rb");
	assert(((void)"failed to open file "IDCPATH" for reading", in));
	fseek(in, 0l, SEEK_END);
	size = ftell(in);
	rewind(in);
	buf = (char*) malloc(size);
	assert(buf);
	fread(buf, size, 1, in);
	fclose(in);
	if (size > 2 && buf[0] == (char) 0xEF) {
		/*assume UTF8 BOM*/
		idcparse(idcp, buf + 3, size - 3);
	} else {
		idcparse(idcp, buf, size);
	}
	FREE(buf);
}
/*jeanine:p:i:8;p:7;a:r;x:6.67;y:-17.00;*/
static
char* docgen_readcss()
{
	FILE *in;
	char *buf, *a, *b, *end;
	register char c;
	int size;

	in = fopen("style.css", "rb");
	assert(((void)"failed to open file style.css for reading", in));
	fseek(in, 0l, SEEK_END);
	size = ftell(in);
	rewind(in);
	buf = (char*) malloc(size);
	assert(buf);
	fread(buf, size, 1, in);
	a = b = buf;
	end = buf + size;
	while (b != end) {
		c = *b;
		if (c != '\n' && c != '\t') {
			if (!c) {
				break;
			}
			*a = c;
			a++;
		}
		b++;
	}
	*a = 0;
	fclose(in);
	return buf;
}
/*jeanine:p:i:4;p:7;a:r;x:6.67;y:12.00;*/
static
int docgen_struct_sort_compar(const void *_a, const void *_b)
{
	register const struct idcp_struct *a = ((struct docgen_structinfo*) _a)->struc;
	register const struct idcp_struct *b = ((struct docgen_structinfo*) _b)->struc;

	return docgen_stricmp(a->name, b->name);
}
/*jeanine:p:i:9;p:11;a:r;x:6.67;y:-62.00;*/
static
void docgen_format_struct_member_typeandname_when_enum(char *buf, struct idcparse *idcp, struct idcp_struct_member *mem)
{
	/*TODO: what if it's an array*/

	struct idcp_enum *en;

	assert(0 <= mem->typeid && mem->typeid < idcp->num_enums);
	en = idcp->enums + mem->typeid;
	buf += sprintf(buf, "<a href='enums.html#enum_%s'>enum %s</a> %s", en->name, en->name, mem->name);
}
/*jeanine:p:i:13;p:11;a:r;x:6.67;y:-49.00;*/
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
	assert((struc_size = docgen_get_struct_size(idcp, struc)));/*jeanine:s:a:r;i:6;*/
	if (mem->nbytes > struc_size) {
		assert(!(mem->nbytes % struc_size));
		sprintf(buf, "[%d]", mem->nbytes / struc_size);
	}
}
/*jeanine:p:i:10;p:11;a:r;x:6.67;y:-28.00;*/
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
void docgen_print_struct_member(FILE *f, struct idcparse *idcp, struct idcp_struct *struc, int offset, int size, struct idcp_struct_member *mem)
{
	int offsetstrlen, isplaceholder, member_struc_size;
	struct docgen_tmpbuf *typeandname;
	struct idcp_struct *member_struc;
	char offsetstr[16];

	memset(offsetstr, ' ', 8);
	offsetstrlen = sprintf(offsetstr, "/*%X*/", offset);
	if (offsetstrlen < 8) {
		offsetstr[offsetstrlen] = '\t';
		offsetstr[offsetstrlen+1] = 0;
	}

	typeandname = docgen_get_tmpbuf(1000);
	if (mem) {
		isplaceholder = !strncmp(mem->name, "field_", 6) || !strncmp(mem->name, "floatField_", 11);
		if (IDC_is_enum(mem->flag)) {
			docgen_format_struct_member_typeandname_when_enum(typeandname->data, idcp, mem);/*jeanine:r:i:9;*/
		} else if (IDC_is_struct(mem->flag)) {
			docgen_format_struct_member_typeandname_when_struct(typeandname->data, idcp, mem);/*jeanine:r:i:13;*/
		} else if (mem->type) {
			docgen_format_struct_member_typeandname_when_type(typeandname->data, mem);/*jeanine:r:i:10;*/
			if (docgen_mark_unknown_structs(&typeandname)) {/*jeanine:r:i:12;*/
#ifdef DOCGEN_WARN_UNKNOWN_STRUCT_REFERENCES
				printf("warn: struct '%s' member '%s' references an unknown struct\n", struc->name, mem->name);
#endif
			}
			/*TODO: make struct/enum references into links*/
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
				strcat(typeandname->data, " <em>/*unk type*/</em>");
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
/*jeanine:p:i:5;p:7;a:r;x:6.67;y:13.00;*/
static
void docgen_print_struct(FILE *f, struct idcparse *idcp, struct idcp_struct *struc)
{
	int tmp, num_members, size, membersleft, lastoffset, nextoffset;
	struct idcp_struct_member *mem;
	char offsetstr[16];

	num_members = struc->end_idx - struc->start_idx;
	size = docgen_get_struct_size(idcp, struc);/*jeanine:r:i:6;*/

	/*TODO: check if struct name is valid to use as anchor, or replace invalid chars?*/
	fprintf(f, "<a id='struc_%s'></a><div class='struc'><h3>%s</h3>",
		struc->name,
		struc->name
	);
	assert (!struc->comment);
	if (struc->rep_comment) {
		/*TODO: mmparse*/
		fprintf(f, "<p>%s</p>", struc->rep_comment);
	}
	fprintf(f, "<pre>%s %s%s%d%s%X%s",
		struc->is_union ? "union" : "struct",
		struc->name,
		" { <i>/*",
		num_members,
		" members, size ",
		size,
		"h*/</i>\n"
	);
	mem = idcp->struct_members + struc->start_idx;
	membersleft = struc->end_idx - struc->start_idx;
	lastoffset = 0;
	while (membersleft) {
		if (mem->offset > lastoffset) {
			docgen_print_struct_member(f, idcp, struc, lastoffset, mem->offset - lastoffset, NULL);/*jeanine:s:a:r;i:11;*/
		}
		docgen_print_struct_member(f, idcp, struc, mem->offset, mem->nbytes, mem);/*jeanine:r:i:11;*/
		lastoffset = mem->offset + mem->nbytes;
		membersleft--;
		mem++;
	}
	fprintf(f, "};%s", "</pre></div>");
}
/*jeanine:p:i:7;p:0;a:b;x:52.00;y:11.88;*/
int main(int argc, char **argv)
{
	struct docgen_structinfo *structinfos, *structinfo;
	struct idcp_struct **struc;
	struct idcparse *idcp;
	FILE *f_structs;
	char *css, *name;
	int i;

	idcp = malloc(sizeof(struct idcparse));
	assert(((void)"failed to malloc for idcparse", idcp));
	docgen_parseidc(idcp);/*jeanine:r:i:1;*/

	css = docgen_readcss();/*jeanine:r:i:8;*/

	/*structs*/
	structinfos = malloc(sizeof(struct docgen_structinfo) * idcp->num_structs);
	assert(structinfos);
	for (i = 0; i < idcp->num_structs; i++) {
		structinfos[i].struc = idcp->structs + i;
		structinfos[i].name_len = strlen(idcp->structs[i].name);
	}
	qsort((void*) structinfos, idcp->num_structs, sizeof(struct docgen_structinfo), docgen_struct_sort_compar);/*jeanine:r:i:4;*/
	f_structs = fopen("structs.html", "wb");
	assert(((void)"failed to open file structs.html for writing", f_structs));
	fprintf(f_structs,
		"%s%s%s",
		"<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'/><title>nfsu2-re/structs</title><style>",
		css,
		"</style></head><body><header><h1>nfsu2-re</h1><p>link</p></header><nav><p>Home</p></nav>"
	);
	fprintf(f_structs, "%s%d%s", "<div><h2>Structs (", idcp->num_structs, ")</h2><ul>");
	for (i = 0, structinfo = structinfos; i < idcp->num_structs; i++, structinfo++) {
		name = structinfo->struc->name;
		fprintf(f_structs, "<li><a href='#struc_%s'>%s</a></li>", name, name);
		/*TODO: anchor link*/
	}
	fprintf(f_structs, "%s", "</ul></div>");
	for (i = 0, structinfo = structinfos; i < idcp->num_structs; i++, structinfo++) {
		docgen_print_struct(f_structs, idcp, structinfo->struc);/*jeanine:r:i:5;*/
	}
	fprintf(f_structs, "%s", "</body></html>");
	fclose(f_structs);

	return 0;
}
