static
void cb_handler_ida_start()
{
	handler_pre_calc_whitespace();
	append("<pre class='ida'>", 17);
}

static
enum DIR_CONTENT_ACTION cb_handler_ida_directive_start(char** to, char *from, struct DIRECTIVE *dir)
{
	if (!strcmp(dir->name, "text")) {
		*to += sprintf(*to, "<span class='text'>");
	} else if (!strcmp(dir->name, "num")) {
		*to += sprintf(*to, "<span class='num'>");
	} else if (!strcmp(dir->name, "str")) {
		*to += sprintf(*to, "<span class='str'>");
	} else if (!strcmp(dir->name, "hi")) {
		*to += sprintf(*to, "<span class='hi'>");
	} else if (!strcmp(dir->name, "comment")) {
		*to += sprintf(*to, "<span class='comment'>");
	} else if (!strcmp(dir->name, "ident")) {
		*to += sprintf(*to, "<span class='ident'>");
	} else {
		return cb_handler_normal_directive_start(to, from, dir);
	}
	strcpy(next_close_tag(), "</span>");
	return LEAVE_CONTENT;
}

struct HANDLER handler_ida = {
	cb_handler_ida_start,
	cb_handler_pre_text,
	cb_handler_pre_end,
	cb_handler_ida_directive_start,
	cb_handler_normal_directive_end,
	"ida"
};

static
void mmparse_ext_init_handler_ida()
{
	MMPARSE_EXT_INIT();

	mmparse_register_handler(&handler_ida);
}

#undef MMPARSE_EXT_INIT
#define MMPARSE_EXT_INIT mmparse_ext_init_handler_ida
