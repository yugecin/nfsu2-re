static
enum DIR_CONTENT_ACTION directive_hookfileref(char **to, char *from, struct DIRECTIVE *dir)
{
	FILE *f;
	char filename[100];
	char relfile[110];

	get_directive_text(dir, filename);
	sprintf(relfile, "../%s", filename);
	f = fopen(relfile, "r");
	if (f) {
		fclose(f);
	} else {
		printf("line %d: file '%s' not found\n", current_line, filename);
	}
	*to += sprintf(*to,
		"<code><a href='"
		"https://github.com/yugecin/nfsu2-re/blob/master/%s"
		"'>", filename);
	strcpy(next_close_tag(), "</a></code>");
	return LEAVE_CONTENT;
}

static
void mmparse_ext_init_directive_hookfileref()
{
	MMPARSE_EXT_INIT();

	mmparse_register_directive("hookfileref", directive_hookfileref);
}

#undef MMPARSE_EXT_INIT
#define MMPARSE_EXT_INIT mmparse_ext_init_directive_hookfileref
