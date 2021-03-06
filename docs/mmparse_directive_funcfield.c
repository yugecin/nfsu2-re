#define MAX_FUNCFIELDS 1000
#define MAX_FUNCFIELD_NAME 100
char funcfield_names[MAX_FUNCFIELDS][MAX_FUNCFIELD_NAME];
char funcfield_addresses[MAX_FUNCFIELDS][20];
char funcfield_types[MAX_FUNCFIELDS];
int num_funcfields;

struct PLACEHOLDER_FFREF_DATA {
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
					funcfield_types[i] ? "funcref" : "fieldref", data->address,
					funcfield_names[i]);
				if (buf[len - 2] == '(') {
					len -= 2;
				}
				fwrite(buf, len, 1, out);
				fwrite("</a>", 4, 1, out);
			} else {
				len = sprintf(buf,
					"<a class='%s' href='#%s'>%s:%s</a>",
					funcfield_types[i] ? "funcref" : "fieldref",
					data->address, data->address,
					funcfield_names[i]);
				fwrite(buf, len, 1, out);
			}
			return;
		}
	}

	printf("line %d: unresolved ref to %s\n", placeholder->line_number, data->address);
	len = sprintf(buf, "<span class='unresolved'>0x%s</span>", data->address);
	fwrite(buf, len, 1, out);
}

static
enum DIR_CONTENT_ACTION directive_funcfield(char **to, char *from, struct DIRECTIVE *dir, char type)
{
	char *address;
	char buf[500];
	int i;
	char c;
	char *target;
	char *from_end;
	int has_inline_address;
	int is_func_ptr_type;

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
	for (i = 0; i < num_funcfields; i++) {
		if (!strcmp(address, funcfield_addresses[i])) {
			printf("line %d: func/field %s was already registered\n", current_line, address);
			next_close_tag()[0] = 0;
			return LEAVE_CONTENT;
		}
	}
	get_directive_text(dir, buf);
	is_func_ptr_type = 0;
	target = funcfield_names[num_funcfields];
	funcfield_types[num_funcfields] = type;
	i = 0;
	for (;;) {
		c = buf[i];
		*target = c;
		if (c == '[') {
			*target = 0;
			break;
		} else if (c == 0) {
			break;
		} else if (c == '(') {
			if (target == funcfield_names[num_funcfields]) {
				/*name is empty, so must be a function ptr type*/
				is_func_ptr_type = 1;
			} else {
				*(++target) = ')';
				*(++target) = 0;
				break;
			}
		} else if (c == ')' && is_func_ptr_type) {
			*target = 0;
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
			*target = 0;
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
enum DIR_CONTENT_ACTION directive_field(char **to, char *from, struct DIRECTIVE *dir)
{
	return directive_funcfield(to, from, dir, 0);
}

static
enum DIR_CONTENT_ACTION directive_func(char **to, char *from, struct DIRECTIVE *dir)
{
	return directive_funcfield(to, from, dir, 1);
}

static
enum DIR_CONTENT_ACTION directive_funcfieldref(char **to, struct DIRECTIVE *dir, int plain)
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
	get_directive_text(dir, data->address);
have_address:
	data->plain = plain;
	return DELETE_CONTENT;
}

static
enum DIR_CONTENT_ACTION directive_ref(char **to, char *from, struct DIRECTIVE *dir)
{
	return directive_funcfieldref(to, dir, 0);
}

static
enum DIR_CONTENT_ACTION directive_refplain(char **to, char *from, struct DIRECTIVE *dir)
{
	return directive_funcfieldref(to, dir, 1);
}

static
void mmparse_ext_init_directive_func()
{
	MMPARSE_EXT_INIT();

	mmparse_register_directive("func", directive_func);
	mmparse_register_directive("field", directive_field);
	mmparse_register_directive("ref", directive_ref);
	mmparse_register_directive("refplain", directive_refplain);
}

#undef MMPARSE_EXT_INIT
#define MMPARSE_EXT_INIT mmparse_ext_init_directive_func
