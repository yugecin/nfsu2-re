#define MAX_FUNCFIELDS 200
#define MAX_FUNCFIELD_NAME 100
char funcfield_names[MAX_FUNCFIELDS][MAX_FUNCFIELD_NAME];
char funcfield_addresses[MAX_FUNCFIELDS][20];
int num_funcfields;

struct PLACEHOLDER_FFREF_DATA {
	char *type;
	int plain;
	char address[20];
};

static
void cb_placeholder_funcfieldref(FILE *out, struct PLACEHOLDER *placeholder)
{
	struct PLACEHOLDER_FFREF_DATA *data = placeholder->data;
	char buf[300];
	int len;
	int i;

	for (i = 0; i < num_funcfields; i++) {
		if (!strcmp(data->address, funcfield_addresses[i])) {
			if (data->plain) {
				len = sprintf(buf,
					"<a class='%s' href='#%s'>%s",
					data->type, data->address,
					funcfield_names[i]);
				if (buf[len - 2] == '(') {
					len -= 2;
				}
				fwrite(buf, len, 1, out);
				fwrite("</a>", 4, 1, out);
			} else {
				len = sprintf(buf,
					"<a class='%s' href='#%s'>%s:%s</a>",
					data->type, data->address, data->address,
					funcfield_names[i]);
				fwrite(buf, len, 1, out);
			}
			return;
		}
	}

	printf("line %d: unresolved %s to %s\n",
		placeholder->line_number, data->type, data->address);
	len = sprintf(buf, "<span class='unresolved'>0x%s</span>", data->address);
	fwrite(buf, len, 1, out);
}

static
enum DIR_CONTENT_ACTION
directive_funcfield(char **to, char *from, struct DIRECTIVE *dir)
{
	char *address;
	char buf[500];
	int i;
	char c;
	char *target;
	char *from_end;
	int has_inline_address;

	if (num_funcfields == MAX_FUNCFIELDS) {
		printf("MAX_FUNCFIELDS reached\n");
		return DELETE_CONTENT;
	}
	has_inline_address = 0;
	for (i = 0; i < dir->num_args; i++) {
		if (!strcmp(dir->argn[i], "address")) {
			strcpy(funcfield_addresses[num_funcfields], address = dir->argv[i]);
			goto have_address;
		}
	}
	from_end = from;
	for (;;) {
		c = *from_end;
		if (c == ' ' && from_end != from) {
			if (from_end - from < 6) {
				printf("line %d: inline address needs at least 6 digits\n",
					current_line);
				break;
			}
			memcpy(address = funcfield_addresses[num_funcfields], from, from_end - from);
			funcfield_addresses[num_funcfields][from_end - from] = 0;
			/*
			that's disgusting
			from[0] = '<';
			from[1] = '!';
			from[2] = '-';
			from[3] = '-';
			from_end[-2] = '-';
			from_end[-1] = '-';
			from_end[0] = '>';
			*/
			has_inline_address = 1;
			goto have_address;
		}
		if (c == 0 || !(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'))) {
			break;
		}
		from_end++;
	}
	printf("line %d: func/field needs 'address' argument\n", current_line);
	next_close_tag()[0] = 0;
	return LEAVE_CONTENT;
have_address:
	get_directive_text(dir, buf, 0);
	target = funcfield_names[num_funcfields];
	for (;;) {
		c = buf[i];
		*target = c;
		if (c == '[') {
			break;
		}
		if (c == 0) {
			break;
		}
		if (c == '(') {
			*(++target) = ')';
			*(++target) = 0;
			break;
		}
		i++;
		if (i == MAX_FUNCFIELD_NAME) {
			printf("line %d: MAX_FUNCFIELD_NAME reached\n", current_line);
			*target = 0;
			break;
		}
		if (c == ' ' || c == '*') {
			target = funcfield_names[num_funcfields];
		} else {
			target++;
		}
	}
	num_funcfields++;
	*to += sprintf(*to, "<span class='%s' id='%s'>", dir->name, address);
	if (!has_inline_address) {
		*to += sprintf(*to, "%s ", address);
	}
	strcpy(next_close_tag(), "</span>");
	return LEAVE_CONTENT;
}

static
enum DIR_CONTENT_ACTION directive_funcfieldref(char **to, struct DIRECTIVE *dir, char *type, int plain)
{
	struct PLACEHOLDER_FFREF_DATA *data;
	int i;

	data = next_placeholder(cb_placeholder_funcfieldref);
	for (i = 0; i < dir->num_args; i++) {
		if (!strcmp(dir->argn[i], "address")) {
			strcpy(data->address, dir->argv[i]);
			goto have_address;
		}
	}
	get_directive_text(dir, data->address, 0);
have_address:
	data->type = type;
	data->plain = plain;
	return DELETE_CONTENT;
}

static
enum DIR_CONTENT_ACTION directive_funcref(char **to, char *from, struct DIRECTIVE *dir)
{
	return directive_funcfieldref(to, dir, "funcref", 0);
}

static
enum DIR_CONTENT_ACTION directive_fieldref(char **to, char *from, struct DIRECTIVE *dir)
{
	return directive_funcfieldref(to, dir, "fieldref", 0);
}

static
enum DIR_CONTENT_ACTION directive_funcplain(char **to, char *from, struct DIRECTIVE *dir)
{
	return directive_funcfieldref(to, dir, "funcref", 1);
}

static
enum DIR_CONTENT_ACTION directive_fieldplain(char **to, char *from, struct DIRECTIVE *dir)
{
	return directive_funcfieldref(to, dir, "fieldref", 1);
}

static
void mmparse_ext_init_directive_func()
{
	MMPARSE_EXT_INIT();

	mmparse_register_directive("func", directive_funcfield);
	mmparse_register_directive("field", directive_funcfield);
	mmparse_register_directive("funcref", directive_funcref);
	mmparse_register_directive("fieldref", directive_fieldref);
	mmparse_register_directive("funcplain", directive_funcplain);
	mmparse_register_directive("fieldplain", directive_fieldplain);
}

#undef MMPARSE_EXT_INIT
#define MMPARSE_EXT_INIT mmparse_ext_init_directive_func
