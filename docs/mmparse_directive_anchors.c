#define MAX_ANCHORS 200
#define MAX_ANCHOR_NAME 100
char anchor_names[MAX_ANCHORS][MAX_ANCHOR_NAME];
int num_anchors;

static
void anchor_sanitize(char *name)
{
	char c;

	while ((c = *name) != 0) {
		if (!(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9'))) {
			*name = '_';
		}
		name++;
	}
}

static
void cb_placeholder_refanchor(FILE *out, struct PLACEHOLDER *placeholder)
{
	char *name = placeholder->data;
	char buf[100];
	int len;
	int i;

	for (i = 0; i < num_anchors; i++) {
		if (!strcmp(anchor_names[i], name)) {
			len = sprintf(buf, "<a href='#%s'>", name);
			fwrite(buf, len, 1, out);
			return;
		}
	}

	printf("line %d: unresolved anchor to %s\n", placeholder->line_number, name);
	len = sprintf(buf, "<a>");
	fwrite(buf, len, 1, out);
}

static
enum DIR_CONTENT_ACTION directive_mkref(char **to, char *from, struct DIRECTIVE *dir)
{
	char name[MAX_ANCHOR_NAME];

	strcpy(next_close_tag(), "");

	if (num_anchors == MAX_ANCHORS) {
		printf("MAX_ANCHORS reached\n");
		return LEAVE_CONTENT;
	}

	get_directive_text(dir, name, 0);
	anchor_sanitize(name);
	strcpy(anchor_names[num_anchors++], name);
	*to += sprintf(*to, "<span id='%s'></span>", name);
	return LEAVE_CONTENT;
}

static
enum DIR_CONTENT_ACTION directive_refto(char **to, char *from, struct DIRECTIVE *dir)
{
	char *name;

	name = next_placeholder(cb_placeholder_refanchor);
	get_directive_text(dir, name, 0);
	anchor_sanitize(name);
	strcpy(next_close_tag(), "</a>");
	return LEAVE_CONTENT;
}

static
void mmparse_ext_init_directive_anchors()
{
	MMPARSE_EXT_INIT();

	mmparse_register_directive("refto", directive_refto);
	mmparse_register_directive("mkref", directive_mkref);
}

#undef MMPARSE_EXT_INIT
#define MMPARSE_EXT_INIT mmparse_ext_init_directive_anchors
