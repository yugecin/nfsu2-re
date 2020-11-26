/**
margin markup parser

Some markup language I suppose. I hope this will stay relatively simple.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TAG_STACK_SIZE 100
/*Max amount of { or } on a single line.*/
#define MAX_MARKS 20
#define HANDLER_STACK_SIZE 20

#define DIR_NAME_LEN 30
#define MAX_DIR_ARGS 4
#define DIR_ARGN_LEN 15
#define DIR_ARGV_LEN 200
struct DIRECTIVE {
	char name[DIR_NAME_LEN];
	int num_args;
	char argn[MAX_DIR_ARGS][DIR_ARGN_LEN];
	char argv[MAX_DIR_ARGS][DIR_ARGV_LEN];
};

enum DIR_CONTENT_ACTION {
	DELETE_CONTENT,
	LEAVE_CONTENT,
};

struct HANDLER {
	void (*start)(); /*called after handler is pushed*/
	void (*text)();
	void (*end)(); /*called after handler is popped*/
	enum DIR_CONTENT_ACTION (*directive_start)(char**,char*,struct DIRECTIVE*);
	void (*directive_end)(char**);
	char name[12];
};

/*Yes, global variables.*/
struct HANDLER *current_handler;
struct HANDLER *handler[HANDLER_STACK_SIZE];
int num_handlers;
char last_handler[100];
char line_raw[2000]; /*Lines should be way shorter than this.*/
char line_expanded[20000];
char *line;
int line_len;
int current_line;
char tag_stack[TAG_STACK_SIZE][25];
int tag_stack_pos;
int open_mark_position[MAX_MARKS];
int num_open_marks;
int close_mark_position[MAX_MARKS];
int num_close_marks;
struct DIRECTIVE directive[MAX_MARKS];
int num_directives;
#define MAX_HEADERS 100
char header_id[MAX_HEADERS][50];
char header_name[MAX_HEADERS][200];
char header_level[MAX_HEADERS];
int num_headers;
char outbuffer[2000000];
char *out = outbuffer;
int current_line_offset; /*used for placeholders*/
int extra_line_offset; /*used for placeholders, when handlers prepend stuff*/
#define MAX_PLACEHOLDER_DATA_LENGTH 200
struct PLACEHOLDER {
	int position;
	int line_number;
	void (*cb)(FILE*,struct PLACEHOLDER*);
	void *data;
	char needs_adjustment;
	char _data[MAX_PLACEHOLDER_DATA_LENGTH];
};
#define MAX_PLACEHOLDERS 500
struct PLACEHOLDER placeholders[MAX_PLACEHOLDERS];
int num_placeholders;
int num_placeholders_before_this_line;
int placeholder_needs_adjustment;
struct PLACEHOLDER dummyplaceholder;

#define PLACEHOLDER_INDEX 0

struct PLACEHOLDER_BREADCRUMB_DATA {
	int continuation_index_offset;
	int is_continuation;
};

#define MAX_REGISTERED_HANDLERS 20
struct HANDLER *registered_handlers[MAX_REGISTERED_HANDLERS];
int num_registered_handlers;

#define MAX_REGISTERED_DIRS 50
char registered_dir_names[MAX_REGISTERED_DIRS][50];
enum DIR_CONTENT_ACTION (*registered_dir_cbs[MAX_REGISTERED_DIRS])(char**,char*,struct DIRECTIVE*);
int num_registered_dirs;

/*Returns ptr to placeholder data.*/
static
void *next_placeholder(void (*cb)(FILE*,struct PLACEHOLDER*))
{
	if (num_placeholders == MAX_PLACEHOLDERS) {
		printf("MAX_PLACEHOLDERS reached\n");
		return dummyplaceholder.data;
	}
	placeholders[num_placeholders].position = out - outbuffer + current_line_offset;
	placeholders[num_placeholders].line_number = current_line;
	placeholders[num_placeholders].needs_adjustment = placeholder_needs_adjustment;
	placeholders[num_placeholders].data = placeholders[num_placeholders]._data;;
	placeholders[num_placeholders].cb = cb;
	num_placeholders++;
	return placeholders[num_placeholders - 1].data;
}

static
void cb_placeholder_section_anchor(FILE *out, struct PLACEHOLDER *placeholder)
{
	static int index = 0;

	char buf[100];
	int len;

	len = sprintf(buf, "<span id='%s'></span>", header_id[index]);
	fwrite(buf, len, 1, out);
	index++;
}

static
void cb_placeholder_breadcrumbs(FILE *out, struct PLACEHOLDER *placeholder)
{
	static int absolute_index = 0;

	struct PLACEHOLDER_BREADCRUMB_DATA *placeholder_data = placeholder->data;
	char buf[1000];
	int len;
	int i;
	int index;
	int next_level;

	index = absolute_index - placeholder_data->continuation_index_offset;
	absolute_index++;
	if (header_level[index] == 1 && !placeholder_data->is_continuation) {
		/*Root level, no breadcrumb.*/
		return;
	}
	len = sprintf(buf, "<p style='margin-top:0'><small>");
	next_level = 1;
	while (next_level < header_level[index]) {
		for (i = index; i >= 0; i--) {
			if (header_level[i] == next_level) {
				len += sprintf(buf + len,
					"<a href='#%s'>%s</a> &gt; ",
					header_id[i],
					header_name[i]);
				break;
			}
		}
		next_level++;
	}
	if (placeholder_data->is_continuation) {
		absolute_index++;
		len += sprintf(buf + len, "<a href='#%s'>%s</a> (continuation)",
			header_id[index],
			header_name[index]);
	} else {
		len += sprintf(buf + len, "%s", header_name[index]);
	}
	len += sprintf(buf + len, "</small></p>");

	fwrite(buf, len, 1, out);
	index++;
}

static
void cb_placeholder_index(FILE *out, struct PLACEHOLDER *placeholder)
{
	int i;
	int last_level;
	int this_level;
	char buf[2000];
	int len;

	last_level = 1;
	fwrite("<ul>", 4, 1, out);
	for (i = 0; i < num_headers; i++) {
		this_level = header_level[i];
		if (i) {
			if (this_level > last_level) {
				fwrite("<ul>", 4, 1, out);
			} else {
				while (last_level > this_level) {
					fwrite("</li>", 5, 1, out);
					fwrite("</ul>", 5, 1, out);
					last_level--;
				}
				fwrite("</li>", 5, 1, out);
			}
		}
		len = sprintf(buf, "<li><a href=\"#%s\">%s</a>", header_id[i], header_name[i]);
		fwrite(buf, len, 1, out);
		last_level = this_level;
	}
	while (last_level > 1) {
		fwrite("</ul></li>", 10, 1, out);
		last_level--;
	}
	fwrite("</ul>", 5, 1, out);
}

static
void cb_placeholder_href(FILE *out, struct PLACEHOLDER *placeholder)
{
	char *id = placeholder->data;
	char buf[300];
	int len;
	int i;

	for (i = 0; i < num_headers; i++) {
		if (!strcmp(header_id[i], id)) {
			len = sprintf(buf, "<a href='#%s'>%s</a>", id, header_name[i]);
			goto write;
		}
	}

	printf("line %d: href to '%s' not found\n", placeholder->line_number, id);
	len = sprintf(buf, "MISSING HREF:%s", id);
write:
	fwrite(buf, len, 1, out);
}

static
int get_section_depth()
{
	int section_depth;
	int i;

	section_depth = 0;
	for (i = 0; i < num_handlers; i++) {
		if (!strcmp(handler[i]->name, "section")) {
			section_depth++;
		}
	}
	return section_depth;
}

static
int line_can_have_paragraph()
{
	char *tag;

	if (line[0] != '<') {
		return 1;
	}
	tag = line + 1;
	return !(line[1] == 'h' ||
		!strncmp(tag, "table", 5) ||
		!strncmp(tag, "ul", 2) ||
		!strncmp(tag, "p", 1));
}

/*
This is kind of disgusting, but will do for now (the only problem really is nested stuff).
Will not work for multi-line directives,
but those don't need the text for now.*/
static
void get_directive_text(struct DIRECTIVE *dir, char *dest, int *out_length)
{
	int dir_index;
	int offset;
	int length;
	char c;

	dir_index = dir - directive;
	offset = open_mark_position[dir_index] + 1;
	for (;;) {
		c = line_raw[offset];
		if (c == '}' || c == 0) {
			*dest = 0;
			break;
		}
		*dest = c;
		dest++;
		offset++;
		length++;
	}
	if (out_length) {
		*out_length = length;
	}
}

static
void mmparse_read_directives(FILE *in)
{
	struct DIRECTIVE *dir;
	char c;
	int dir_name_len;
	int arg_name_len;
	int arg_val_len;
	int val_uses_quotes;

next_directive:
	do {
		c = fgetc(in);
		if (c == EOF || c == '\n') {
			return;
		}
	} while (c == ' ');

	if (num_directives >= MAX_MARKS) {
		printf("line %d: too many directives\n", current_line);
		return;
	}

	dir = &directive[num_directives];
	dir->num_args = 0;

	/* directive name */
	dir_name_len = 0;
	dir->name[0] = 0;
	for (;;) {
		if (c == EOF || c == '\n') {
			if (dir_name_len) {
				num_directives++;
			}
			return;
		}
		if (c == ',') {
			num_directives++;
			goto next_directive;
		}
		if (c == ' ') {
			num_directives++;
			break;
		}
		if (dir_name_len >= DIR_NAME_LEN - 1) {
			printf("line %d: directive name too long\n", current_line);
			num_directives++;
			goto ret_eol;
		}
		dir->name[dir_name_len++] = c;
		dir->name[dir_name_len] = 0;
		c = fgetc(in);
	}

next_argument:
	if (dir->num_args >= MAX_DIR_ARGS) {
		printf("line %d: too many directive arguments\n", current_line);
		return;
	}

	/* argument name */
	arg_name_len = 0;
	dir->argn[dir->num_args][0] = 0;
	dir->argv[dir->num_args][0] = 0;
	for (;;) {
		c = fgetc(in);
		if (c == EOF || c == '\n') {
			dir->num_args++;
			return;
		}
		if (c == ',') {
			dir->num_args++;
			goto next_directive;
		}
		if (c == '=') {
			break;
		}
		if (arg_name_len == DIR_ARGN_LEN - 1) {
			printf("line %d: directive argument name too long\n", current_line);
			goto ret_eol;
		}
		dir->argn[dir->num_args][arg_name_len++] = c;
		dir->argn[dir->num_args][arg_name_len] = 0;
	}

	/* argument value */
	val_uses_quotes = 0;
	arg_val_len = 0;
	for (;;) {
		c = fgetc(in);
		if (c == EOF || c == '\n') {
			dir->num_args++;
			return;
		}
		if (val_uses_quotes) {
			if (c == '"') { /*TODO: escapes?*/
				c = fgetc(in);
				if (c == EOF || c == '\n') {
					dir->num_args++;
					return;
				}
				goto donewith; /*this is incorrect when input is invalid*/
			}
		} else {
donewith:
			if (c == ',') {
				dir->num_args++;
				goto next_directive;
			}
			if (c == ' ') {
				dir->num_args++;
				goto next_argument;
			}
		}
		if (arg_val_len == DIR_ARGV_LEN - 1) {
			printf("line %d: directive argument value too long\n", current_line);
			goto ret_eol;
		}
		if (arg_val_len == 0 && c == '"') {
			val_uses_quotes = 1;
			continue;
		}
		dir->argv[dir->num_args][arg_val_len++] = c;
		dir->argv[dir->num_args][arg_val_len] = 0;
	}

	return;
ret_eol:
	do {
		c = fgetc(in);
	} while (c != EOF && c != '\n');
}

static
void mmparse_read_line(FILE *in, int handle_directives)
{
	int c;
	char last_char;
	char num_pipes;

	line = line_raw;
	current_line++;
	line_len = 0;
	last_char = 0;
	num_pipes = 0;
	num_open_marks = 0;
	num_close_marks = 0;
	num_directives = 0;

	for (;;) {
		c = fgetc(in);
		if (c == '|') {
			num_pipes++;
			if (num_pipes == 3) {
				line_len -= 2; /*Because 2 of them are already read.*/
				while (line_len &&
					(line[line_len - 1] == ' ' || line[line_len - 1] == '\t'))
				{
					line_len--;
				}
				line[line_len] = 0;
				mmparse_read_directives(in);
				break;
			}
		} else {
			num_pipes = 0;
		}
		switch (c) {
		case '{':
			if (handle_directives) {
				if (last_char == '\\') {
					line_len--;
					break;
				}
				open_mark_position[num_open_marks++] = line_len;
				last_char = 0;
			}
			break;
		case '}':
			if (handle_directives) {
				if (last_char == '\\') {
					line_len--;
					break;
				}
				close_mark_position[num_close_marks++] = line_len;
				last_char = 0;
			}
			break;
		case EOF:
		case '\n':
			line[line_len] = 0;
			return;
		}

		last_char = (char) c;
		line[line_len++] = (char) c;
	}
}

static
void mmparse_expand_line()
{
	int open_idx;
	int close_idx;
	int open_position;
	int close_position;
	int len;
	enum DIR_CONTENT_ACTION eat_contents;
	int was_contents_eaten;
	char *from, *to;

	line = line_expanded;
	from = line_raw;
	to = line_expanded;
	open_idx = 0;
	close_idx = 0;

	was_contents_eaten = 0;
	eat_contents = LEAVE_CONTENT;

	for (;;) {
		open_position = -1;
		/*Not using num_open_marks, because a line might have no
		directives (but still open marks), but they still need
		to process close marks.*/
		if (open_idx < num_directives) {
			open_position = open_mark_position[open_idx];
		}
		close_position = -1;
		if (close_idx < num_close_marks) {
			close_position = close_mark_position[close_idx];
		}

		if (open_position != -1) {
			if (open_position < close_position || close_position == -1) {
				len = open_position - (from - line_raw);
				if (len) {
					memcpy(to, from, len);
				}
				to += len;
				from += len + 1;
				if (tag_stack_pos < TAG_STACK_SIZE) {
					current_line_offset = to - line_expanded;
					eat_contents =
						current_handler->directive_start(
									&to,
									from,
									&directive[open_idx]);
				} else {
					printf("line %d: tag stack depleted (TAG_STACK_SIZE)\n",
						current_line);
				}
				open_idx++;
				continue;
			}
		}
		if (close_position != -1) {
			if (close_position < open_position || open_position == -1) {
				len = close_position - (from - line_raw);
				if (len) {
					if (eat_contents == DELETE_CONTENT) {
						eat_contents = LEAVE_CONTENT;
						was_contents_eaten = 1;
					} else {
						memcpy(to, from, len);
						to += len;
					}
				}
				from += len + 1;

				close_idx++;
				if (was_contents_eaten) {
					was_contents_eaten = 0;
					continue;
				}

				if (tag_stack_pos) {
					current_line_offset = to - line_expanded;
					current_handler->directive_end(&to);
				} else {
					/*Should we warn? It then requires escaping all closing
					placeholders in 'pre' blocks when tag stack is empty,
					but in the current way it's not consistent because they're
					still needed when the stack is not empty...*/
					/*printf("line %d: tag stack empty\n", current_line);*/
					*to = '}';
					to++;
				}
				continue;
			}
		}
		to += sprintf(to, "%s", from);
		line_len = to - line_expanded;
		break;
	}
	current_line_offset = 0;
}

static
void append(char *text, int len)
{
	memcpy(out, text, len);
	out += len;
	extra_line_offset += len;
}

static
char *next_close_tag()
{
	return tag_stack[tag_stack_pos++];
}

static
void cb_handler_nop()
{
}

static
void cb_handler_normal_text()
{
	if (!line_len) return;
	line[line_len++] = '\n';
	memcpy(out, line, line_len); out += line_len;
}

static
void print_tag_with_directives(char **to, struct DIRECTIVE *dir, char *closestr)
{
	int i;

	*to += sprintf(*to, "<%s", dir->name);
	for (i = 0; i < dir->num_args; i++) {
		if (dir->argn[i][0]) {
			*to += sprintf(*to, " %s=\"%s\"", dir->argn[i], dir->argv[i]);
		}
	}
	*to += sprintf(*to, "%s", closestr);
}

static
enum DIR_CONTENT_ACTION
cb_handler_normal_directive_start(char** to, char *from, struct DIRECTIVE *dir)
{
	int i;

	for (i = 0; i < num_registered_dirs; i++) {
		if (!strcmp(dir->name, registered_dir_names[i])) {
			return registered_dir_cbs[i](to, from, dir);
		}
	}

	print_tag_with_directives(to, dir, ">");
	sprintf(next_close_tag(), "</%s>", dir->name);
	return LEAVE_CONTENT;
}

static
void cb_handler_normal_directive_end(char** to)
{
	--tag_stack_pos;
	*to += sprintf(*to, tag_stack[tag_stack_pos]);
}

static
void section_print_link_to_index()
{
	append("<p style='margin-bottom:0'><a href='#index'>Index</a></p>", 57);
}

static int section_is_open[HANDLER_STACK_SIZE];
static int section_is_continuation[HANDLER_STACK_SIZE];
static int section_has_p;
static int section_was_empty_line;

static
void cb_handler_section_start()
{
	section_was_empty_line = 1;
	/*minus one is the current handler*/
	section_is_continuation[num_handlers - 1] = 0;
}

static
void section_write_div_open(int continuation)
{
	static int number_of_section_continuations = 0;

	struct PLACEHOLDER_BREADCRUMB_DATA *placeholder_data;
	char buf[100];
	int len;
	int section_depth;

	section_depth = get_section_depth() - 1;
	if (section_depth) {
		len = sprintf(buf,
			"<div class='indent' style='margin-left:%dem'>",
			section_depth * 5
		);
		append(buf, len);
	} else {
		append("<div>", 5);
	}

	if (continuation) {
		number_of_section_continuations++;
	}
	placeholder_data = next_placeholder(cb_placeholder_breadcrumbs);
	placeholder_data->continuation_index_offset = number_of_section_continuations * 2;
	placeholder_data->is_continuation = continuation;
}

static
void cb_handler_section_text()
{
	int is_empty_line;
	int cur_idx;
	int prev_idx;

	is_empty_line = !line_len;
	if (section_was_empty_line) {
		if (!is_empty_line) {
			/*minus one is current handler*/
			cur_idx = num_handlers - 1;
			if (!section_is_open[cur_idx]) {
				/*minus two is parent handler*/
				prev_idx = cur_idx - 1;
				if (!strcmp(handler[prev_idx]->name, "section")) {
					if (section_is_open[prev_idx]) {
						section_print_link_to_index();
						append("</div>", 6);
						section_is_open[prev_idx] = 0;
					}
				}
				if (section_is_continuation[cur_idx]) {
					section_write_div_open(1);
				} else {
					next_placeholder(cb_placeholder_section_anchor);
					section_write_div_open(0);
				}
				section_is_open[cur_idx] = 1;
				section_is_continuation[cur_idx] = 1;
			}
			if (line_can_have_paragraph()) {
				append("<p>", 3);
				section_has_p = 1;
			}
		}
	} else {
		if (is_empty_line && section_has_p) {
			append("</p>", 4);
			section_has_p = 0;
		}
	}
	section_was_empty_line = is_empty_line;

	if (line_len) {
		line[line_len++] = '\n';
		memcpy(out, line, line_len); out += line_len;
	}
}

static
void cb_handler_section_end()
{
	/*minus one is current, but handler is already popped at this point*/
	if (section_is_open[num_handlers]) {
		section_print_link_to_index();
		append("</div>", 6);
		section_is_open[num_handlers] = 0;
		section_is_continuation[num_handlers] = 0;
	}
}

static
void cb_handler_pre_start()
{
	append("<pre>", 5);
}

static
void cb_handler_pre_end()
{
	append("</pre>", 6);
}

static
void cb_handler_ul_start()
{
	append("<ul>", 4);
}

static int ul_needs_closing_li;

static
void cb_handler_ul_text()
{
	if (line_len) {
		line[line_len++] = '\n';
		if (line[0] == '-') {
			if (ul_needs_closing_li) {
				append("</li>", 5);
			}
			append("<li>", 4);
			ul_needs_closing_li = 1;
			line_len -= 2;
			line += 2;
			extra_line_offset -= 2; /*because of the line+=2*/
		}
		memcpy(out, line, line_len); out += line_len;
	}
}

static
void cb_handler_ul_end()
{
	if (ul_needs_closing_li) {
		append("</li>", 5);
		ul_needs_closing_li = 0;
	}
	append("</ul>", 5);
}

struct HANDLER handler_nop = {
	cb_handler_nop,
	cb_handler_nop,
	cb_handler_nop,
	(void*) cb_handler_nop,
	(void*) cb_handler_nop,
	"nop"
};

struct HANDLER handler_normal = {
	cb_handler_nop,
	cb_handler_normal_text,
	cb_handler_nop,
	cb_handler_normal_directive_start,
	cb_handler_normal_directive_end,
	"normal"
};

struct HANDLER handler_plain = {
	cb_handler_nop,
	cb_handler_normal_text,
	cb_handler_nop,
	(void*) cb_handler_nop,
	(void*) cb_handler_nop,
	"plain"
};

struct HANDLER handler_section = {
	cb_handler_section_start,
	cb_handler_section_text,
	cb_handler_section_end,
	cb_handler_normal_directive_start,
	cb_handler_normal_directive_end,
	"section"
};

struct HANDLER handler_pre = {
	cb_handler_pre_start,
	cb_handler_normal_text,
	cb_handler_pre_end,
	cb_handler_normal_directive_start,
	cb_handler_normal_directive_end,
	"pre"
};

struct HANDLER handler_ul = {
	cb_handler_ul_start,
	cb_handler_ul_text,
	cb_handler_ul_end,
	cb_handler_normal_directive_start,
	cb_handler_normal_directive_end,
	"ul"
};

static
enum DIR_CONTENT_ACTION mmparse_directive_href(char **to, char *from, struct DIRECTIVE *dir)
{
	get_directive_text(dir, next_placeholder(cb_placeholder_href), 0);
	return DELETE_CONTENT;
}

static
enum DIR_CONTENT_ACTION mmparse_directive_index(char **to, char *from, struct DIRECTIVE *dir)
{
	next_placeholder(cb_placeholder_index);
	next_close_tag()[0] = 0;
	return LEAVE_CONTENT;
}

static
enum DIR_CONTENT_ACTION mmparse_directive_imgcaptioned(char **to, char *from, struct DIRECTIVE *dir)
{
	strcpy(dir->name, "img");
	*to += sprintf(*to, "<p class='center'>");
	print_tag_with_directives(to, dir, "/><br/>");
	strcpy(next_close_tag(), "</p>");
	return LEAVE_CONTENT;
}

static
enum DIR_CONTENT_ACTION mmparse_directive_h(char **to, char *from, struct DIRECTIVE *dir)
{
	int i;
	int section_depth;
	char *id_argument;

	section_depth = get_section_depth();
	/*Overwriting this might not be the best idea but ok*/
	dir->name[1] = '1' + section_depth;
	dir->name[2] = 0;
	id_argument = 0;
	for (i = 0; i < dir->num_args; i++) {
		if (!strcmp(dir->argn[i], "id")) {
			id_argument = dir->argv[i];
			break;
		}
	}
	if (id_argument) {
		if (num_headers < MAX_HEADERS) {
			header_level[num_headers] = section_depth;
			strcpy(header_id[num_headers], id_argument);
			get_directive_text(dir, header_name[num_headers], 0);
			num_headers++;
		} else {
			printf("exceeded MAX_HEADERS\n");
		}
		/*Remove the id attribute because it will be placed by a placeholder.*/
		dir->argn[id_argument - dir->argv[0]][0] = 0;
	} else {
		if (section_depth) {
			printf("line %d: no id for header\n", current_line);
		}
	}
	print_tag_with_directives(to, dir, ">");
	if (id_argument && section_depth) {
		sprintf(next_close_tag(), " <a href=\"#%s\">#</a></%s>", id_argument, dir->name);
	} else {
		sprintf(next_close_tag(), "</%s>", dir->name);
	}
	return LEAVE_CONTENT;
}

static
void mmparse_register_handler(struct HANDLER *handler)
{
	if (num_registered_handlers == MAX_REGISTERED_HANDLERS) {
		printf("MAX_REGISTERED_HANDLERS reached\n");
		return;
	}
	registered_handlers[num_registered_handlers++] = handler;
}

static
void mmparse_register_directive(
	char *name,
	enum DIR_CONTENT_ACTION (*cb)(char**,char*,struct DIRECTIVE*))
{
	if (num_registered_dirs == MAX_REGISTERED_DIRS) {
		printf("MAX_REGISTERED_DIRS reached\n");
		return;
	}
	strcpy(registered_dir_names[num_registered_dirs], name);
	registered_dir_cbs[num_registered_dirs] = cb;
	num_registered_dirs++;
}

static
void mmparse_init()
{
	mmparse_register_handler(&handler_pre);
	mmparse_register_handler(&handler_ul);
	mmparse_register_handler(&handler_section);
	mmparse_register_handler(&handler_plain);
	mmparse_register_handler(&handler_nop);
	mmparse_register_directive("href", mmparse_directive_href);
	mmparse_register_directive("index", mmparse_directive_index);
	mmparse_register_directive("imgcaptioned", mmparse_directive_imgcaptioned);
	mmparse_register_directive("h", mmparse_directive_h);
}
#define MMPARSE_EXT_INIT mmparse_init

#include "mmparse_handler_ida.c"
#include "mmparse_handler_symbols.c"
#include "mmparse_directive_funcfield.c"
#include "mmparse_directive_anchors.c"

char infile[500], outfile[500];

static
int write()
{
	FILE *f;
	struct PLACEHOLDER *placeholder;
	int placeholder_idx;
	int total_length;
	int to;
	int at;

	f = fopen(outfile, "wb");
	if (!f) {
		printf("Can't open '%s' for writing", outfile);
		return 1;
	}

	placeholder_idx = 0;
	at = 0;
	total_length = out - outbuffer;

	do {
		if (placeholder_idx < num_placeholders) {
			placeholder = placeholders + placeholder_idx;
			to = placeholder->position;
			placeholder_idx++;
		} else {
			placeholder = 0;
			to = total_length;
		}
		fwrite(outbuffer + at, to - at, 1, f);
		at = to;
		if (placeholder) {
			placeholder->cb(f, placeholder);
		}
	} while (at < total_length);

	fclose(f);
	return 0;
}

int main(int argc, char **argv)
{
	struct HANDLER *tmp_handler;
	FILE *in;
	int i;
	char *next_arg_location;

	if (argc <= 1) {
		goto printhelp;
	}
	MMPARSE_EXT_INIT();
	next_arg_location = 0;
	for (i = 1; i < argc; i++) {
		if (next_arg_location) {
			strcpy(next_arg_location, argv[i]);
			next_arg_location = 0;
		} else if (!strcmp(argv[i], "--in")) {
			next_arg_location = infile;
		} else if (!strcmp(argv[i], "--out")) {
			next_arg_location = outfile;
		} else {
			printf("unexpected argument: %s\n", argv[i]);
			goto printhelp;
		}
	}
	if (next_arg_location) {
		printf("expected a value for: %s\n", argv[argc - 1]);
		return 1;
	}
	if (!infile[0] || !outfile[0]) {
		goto printhelp;
	}

	in = fopen(infile, "r");
	if (!in) {
		printf("Can't open '%s' for reading", infile);
		return 1;
	}

	current_handler = &handler_normal;
	current_handler->start();
	handler[0] = current_handler;
	num_handlers = 1;

	current_line = 0;
	while (!feof(in)) {
		mmparse_read_line(in, current_handler != &handler_plain);
		if (!line_len) {
			current_handler->text();
			continue;
		}
		if (num_directives) {
			if (num_open_marks != num_directives) {
				printf("line %d: got %d directives but %d marks\n",
					current_line, num_directives, num_open_marks);
				goto ret_err;
			}
		}
		if (!strncmp(line, ".push ", 6)) {
			if (num_handlers == HANDLER_STACK_SIZE) {
				printf("line %d: handler stack is full\n", current_line);
				goto ret_err;
			}
			for (i = 0; i < num_registered_handlers; i++) {
				if (!strcmp(line + 6, registered_handlers[i]->name)) {
					goto have_handler;
				}
			}
			printf("line %d: handler '%s' unknown\n", current_line, line + 6);
			goto ret_err;
have_handler:
			current_handler = registered_handlers[i];
			handler[num_handlers++] = current_handler;
			current_handler->start();
			continue;
		} else if (!strncmp(line, ".pop ", 5)) {
			if (strcmp(line + 5, current_handler->name)) {
				printf("line %d: popping handler '%s' but current is '%s'\n",
					current_line, line + 5, current_handler->name);
			}
			if (num_handlers == 1) {
				printf("line %d: popping handler but stack is empty\n",
					current_line);
				goto ret_err;
			}
			tmp_handler = current_handler;
			num_handlers--;
			current_handler = handler[num_handlers - 1];
			tmp_handler->end();
		} else {
			extra_line_offset = 0;
			num_placeholders_before_this_line = num_placeholders;
			placeholder_needs_adjustment = 1;
			if (num_directives || num_close_marks) {
				mmparse_expand_line();
			}
			placeholder_needs_adjustment = 0;
			current_handler->text();
			for (i = num_placeholders_before_this_line; i < num_placeholders; i++) {
				if (placeholders[i].needs_adjustment) {
					placeholders[i].position += extra_line_offset;
				}
			}
		}
	}

	if (num_handlers != 1) {
		printf("have %d handlers at EOF, should be 1\n", num_handlers);
	}
	current_handler->end();
	if (tag_stack_pos) {
		printf("tag stack not empty (%d)\n", tag_stack_pos);
	}

	fclose(in);
	return write();
ret_err:
	fclose(in);
	return 1;
printhelp:
	puts("Options:");
	puts("--in [filename]      input file");
	puts("--out [filename]     output file");
	return 1;
}
