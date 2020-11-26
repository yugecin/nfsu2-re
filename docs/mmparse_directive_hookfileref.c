static
enum DIR_CONTENT_ACTION directive_hookfileref(char **to, char *from, struct DIRECTIVE *dir)
{
	char filename[100];

	get_directive_text(dir, filename, 0);
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
