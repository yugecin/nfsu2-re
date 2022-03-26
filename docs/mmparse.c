/**
margin markup parser
*/
#define MMPARSE_MAX_DIRECTIVES 20 /*On a single line*/
#define MMPARSE_LINE_RAW_MAX_LEN 2000
#define MMPARSE_LINE_EXPANDED_MAX_LEN 20000
#define MMPARSE_TAGSTACK_SIZE 100
#define MMPARSE_MAX_PLACEHOLDERS 100
#define MMPARSE_DIRECTIVE_NAME_MAX_LEN 30
#define MMPARSE_DIRECTIVE_MAX_ARGS 4
#define MMPARSE_DIRECTIVE_ARGN_MAX_LEN 15
#define MMPARSE_DIRECTIVE_ARGV_MAX_LEN 200
#define MMPARSE_MODE_STACK_SIZE 20

#define MMPARSE_CAN_APPEND_EXPANDED 1
#define MMPARSE_CAN_APPEND_CLOSING 2
#define MMPARSE_CAN_ALLOCATE_PH 4
#define MMPARSE_CAN_APPEND_PH 8
#define MMPARSE_CAN_APPEND_MAIN 16

struct mmparse;

struct mmp_directive_arg {
	char name[MMPARSE_DIRECTIVE_ARGN_MAX_LEN];
	char value[MMPARSE_DIRECTIVE_ARGV_MAX_LEN];
};

struct mmp_directive {
	char name[MMPARSE_DIRECTIVE_NAME_MAX_LEN];
	unsigned char argc;
	struct mmp_directive_arg args[MMPARSE_DIRECTIVE_MAX_ARGS];
};

enum mmp_directive_content_action {
	DELETE_CONTENTS,
	LEAVE_CONTENTS,
};

struct mmp_directive_content_data {
	struct mmp_directive *directive;
	int content_len;
	char contents[MMPARSE_LINE_RAW_MAX_LEN];
};

struct mmp_directive_handler {
	char *name;
	/**
	Write output by using function 'mmparse_append_to_expanded_line', this output will
	be put where the directive open mark is placed. Use 'mmparse_append_to_closing_tag' to
	write (to a buffer whos contents will be put) at the directive's closing mark position.
	This may use function 'mmparse_allocate_placeholder' to create a placeholder.
	*/
	enum mmp_directive_content_action (*handle)(struct mmparse*, struct mmp_directive_content_data*);
};

enum mmp_mode_line_parsing {
	MMPARSE_DONT_PARSE_LINES,
	MMPARSE_DO_PARSE_LINES,
};

struct mmp_mode {
	/*called after mode is pushed*/
	void (*start)(struct mmparse*);
	/**
	Input is 'mm->pd.line' which has a length of 'mm->pd.line_len'.
	Extra output can be written before and after,
	but the input line must be written without modifications.
	Write output by using function 'mmparse_append_to_main_output'.
	@return number of characters written *before* 'mm->pd.line' was written.
	*/
	int (*println)(struct mmparse*);
	/**called after mode is popped*/
	void (*end)(struct mmparse*);
	/**
	Write output by using function 'mmparse_append_to_expanded_line', this output will
	be put where the directive open mark is placed. Use 'mmparse_append_to_closing_tag' to
	write (to a buffer whos contents will be put) at the directive's closing mark position.
	This may use function 'mmparse_allocate_placeholder' to create a placeholder.
	*/
	enum mmp_directive_content_action (*directive)(struct mmparse*, struct mmp_directive_content_data*);
	char *name;
	enum mmp_mode_line_parsing parse_lines;
};

struct mmp_output_part {
	/**main data of this part, not zero terminated!*/
	char *data0;
	/**length of the main data of this part*/
	int data0_len;
	/**placeholder data of this part, not zero terminated!*/
	char *data1;
	/**length of the placeholder data of this part*/
	int data1_len;
};

struct mmp_placeholder {
	/**used during parsing*/
	int offset;
	/**used for error reporting*/
	int line_number;
	/**custom data that is allocated (from mm->config.data3) during placeholder allocation*/
	void *data;
	int data_size;
	/**Action function that will be called when placeholders are processed.
	Write output by using function 'mmparse_append_to_placeholder_output'.*/
	void (*action)(struct mmparse*,struct mmp_output_part*,void *data,int data_size);
};

struct mmp_config {
	/**anything*/
	void *userdata;
	/**some distinctive name that will be printed when an error message is generated*/
	char *debug_subject;
	/**source text to parse*/
	char *source;
	/**length of string in 'source'*/
	int source_len;
	/**last entry must be NULL, first entry will be active when starting*/
	struct mmp_mode **modes;
	/**last entry's name and cb must be NULL*/
	struct mmp_directive_handler *directive_handlers;
	/**see docs of the 'mmparse' function to see how to use the output data*/
	struct {
		/**primary output buffer to be written to, this should be the largest*/
		char *data0;
		/**length of the primary output buffer*/
		int data0_len;
		/**secondary output buffer to be written to,
		this is allowed to be NULL if no placeholders are being used during parsing*/
		char *data1;
		/**length of the secondary output buffer*/
		int data1_len;
		/**first temp buffer, to be written to,
		will be used to hold text that should be appended when directive close marks are found*/
		char *data2;
		/**length of the first temp buffer*/
		int data2_len;
		/**second temp buffer to be written to,
		this is allowed to be NULL if no placeholders are being used during parsing,
		or no placeholders need to allocate data, or if those placeholders use their
		own allocators.*/
		char *data3;
		/**length of the second temp buffer*/
		int data3_len;
	} dest;
};

struct mmparse {
	struct mmp_config config;
	/**array of struct 'mmp_output_part'.
	An entry with its 'data0' set to NULL denotes the end of the array*/
	struct mmp_output_part *output;
	struct {
		char *charptr;
		int charsleft;
		int current_line;
	} in; /*input*/
	struct {
		char hasmargin;
		char ctrlchar_open;
		char ctrlchar_close;
		unsigned char num_open_marks;
		int open_mark_positions[MMPARSE_MAX_DIRECTIVES];
		unsigned char num_close_marks;
		int close_mark_positions[MMPARSE_MAX_DIRECTIVES];
		unsigned char num_directives;
		struct mmp_directive directives[MMPARSE_MAX_DIRECTIVES];
		/**value to set for 'struct placeholder.offset' when allocating a placeholder*/
		int next_placeholder_offset;
		/**Length of string in 'line' buffer.*/
		int line_len;
		/**Will either point to 'line_raw' or 'line_expanded' after parsing.*/
		char *line;
		/**Raw current input line until the '|||' mark.*/
		char line_raw[MMPARSE_LINE_RAW_MAX_LEN];
		/**Current input line after expanding, if applicable.*/
		char line_expanded[MMPARSE_LINE_EXPANDED_MAX_LEN];
		char *tagbuf;
		int tagbuf_sizeleft;
		int tag_stack_size;
		char tag_length[MMPARSE_TAGSTACK_SIZE];
		int tag_opened_on_line[MMPARSE_TAGSTACK_SIZE];
	} pd; /*parsedata*/
	struct {
		struct mmp_mode *current;
		int num_pushed_modes;
		struct mmp_mode *pushed_modes[MMPARSE_MODE_STACK_SIZE];
	} md; /*mode*/
	struct {
		int size;
		struct mmp_placeholder placeholders[MMPARSE_MAX_PLACEHOLDERS];
		char *databuf;
		int databuf_sizeleft;
	} ph; /*placeholders*/
	struct {
		int data0buf_sizeleft;
		int data1buf_sizeleft;
		/**max+1 because one last dummy entry is always needed, see docs of 'mmparse.output'
		amount of used parts is equal to 'ph.size'+1*/
		struct mmp_output_part parts[MMPARSE_MAX_PLACEHOLDERS + 1];
		struct mmp_output_part *current_part;
	} op; /*output*/
	/**to ensure users don't write to the wrong output at the wrong times, see 'MMPARSE_CAN_*'*/
	int can;
};

static
void mmparse_prefailmsg(struct mmparse *mm)
{
	fprintf(stderr, "mmparse: while parsing target '%s':\n", mm->config.debug_subject);
}

static
void mmparse_failmsg(struct mmparse *mm, char *msg)
{
	mmparse_prefailmsg(mm);
	fprintf(stderr, "line %d: %s\n", mm->in.current_line, msg);
}
#define mmparse_failmsgf(MM,MSG,...) mmparse_prefailmsg(mm);\
                                     fprintf(stderr, "%d: "MSG"\n", MM->in.current_line, __VA_ARGS__)
/*jeanine:p:i:6;p:1;a:t;x:36.32;*/
void mmparse_append_to_expanded_line(struct mmparse *mm, char *from, int len)
{
	assert(((void)"invalid state", mm->can & MMPARSE_CAN_APPEND_EXPANDED));
	if (mm->pd.line_len + len >= MMPARSE_LINE_EXPANDED_MAX_LEN) {
		mmparse_failmsg(mm, "increase MMPARSE_LINE_EXPANDED_MAX_LEN");
		assert(0);
	}
	memcpy(mm->pd.line + mm->pd.line_len, from, len);
	mm->pd.line_len += len;
	mm->pd.line[mm->pd.line_len] = 0;
	mm->pd.next_placeholder_offset += len;
}
/*jeanine:p:i:13;p:6;a:b;y:1.88;*/
void mmparse_append_to_main_output(struct mmparse *mm, char *from, int len)
{
	assert(((void)"invalid state", mm->can & MMPARSE_CAN_APPEND_MAIN));
	if (mm->op.data0buf_sizeleft - len < 0) {
		mmparse_failmsg(mm, "output buffer data0 too small");
		assert(0);
	}
	mm->op.data0buf_sizeleft -= len;
	memcpy(mm->op.current_part->data0 + mm->op.current_part->data0_len, from, len);
	mm->op.current_part->data0_len += len;
}
/*jeanine:p:i:14;p:13;a:b;y:1.88;*/
void mmparse_append_to_placeholder_output(struct mmparse *mm, struct mmp_output_part *output_part, char *from, int len)
{
	assert(((void)"invalid state", mm->can & MMPARSE_CAN_APPEND_PH));
	if (mm->op.data1buf_sizeleft - len < 0) {
		mmparse_failmsg(mm, "output buffer data1 too small");
		assert(0);
	}
	mm->op.data1buf_sizeleft -= len;
	memcpy(output_part->data1 + output_part->data1_len, from, len);
	output_part->data1_len += len;
}
/*jeanine:p:i:20;p:14;a:b;y:1.88;*/
void mmparse_append_to_closing_tag(struct mmparse *mm, char *from, int len)
{
	assert(((void)"invalid state", mm->can & MMPARSE_CAN_APPEND_CLOSING));
	if (mm->pd.tagbuf_sizeleft - len < 0) {
		mmparse_failmsg(mm, "output buffer data2 too small");
		assert(0);
	}
	mm->pd.tagbuf_sizeleft -= len;
	memcpy(mm->pd.tagbuf, from, len);
	mm->pd.tagbuf += len;
	mm->pd.tag_length[mm->pd.tag_stack_size - 1] += len;
}
/*jeanine:p:i:2;p:3;a:r;x:28.67;y:-129.53;*/
/**
Parses the part after the margin (|||).

A directive looks like:
  directive [{argumentlist}]
The argumentlist looks like:
  {argument}[,{argument},...]
An argument looks like:
  name
  name=value
  name="value with whitespace"
*/
static
void mmparse_read_directives(struct mmparse *mm)
{
	int dir_name_len, arg_name_len, arg_val_len;
	struct mmp_directive *dir;
	char c, val_uses_quotes;

next_directive:
	while (mm->in.charsleft && (mm->in.charsleft--, c = *(mm->in.charptr++)) == ' ');
	if (c == '\n') {
		return;
	}

	if (mm->pd.num_directives >= MMPARSE_MAX_DIRECTIVES) {
		mmparse_failmsg(mm, "increase MMPARSE_MAX_DIRECTIVES");
		assert(0);
	}

	dir = mm->pd.directives + mm->pd.num_directives;
	dir->argc = 0;

	/* directive name */
	dir_name_len = 0;
	dir->name[0] = 0;
	for (;;) {
		switch (c) {
		case ',':
			if (!dir_name_len) {
				mmparse_failmsg(mm, "found comma while directive is still empty");
				assert(0);
			}
			mm->pd.num_directives++;
			goto next_directive;
		case ' ':
			if (!dir_name_len) {
				mmparse_failmsg(mm, "found space while directive is still empty");
				assert(0);
			}
			mm->pd.num_directives++;
			goto next_argument;
		}
		if (dir_name_len >= MMPARSE_DIRECTIVE_NAME_MAX_LEN - 1) {
			mmparse_failmsg(mm, "increase MMPARSE_DIRECTIVE_NAME_MAX_LEN");
			assert(0);
		}
		dir->name[dir_name_len++] = c;
		dir->name[dir_name_len] = 0;
		if (!mm->in.charsleft || (mm->in.charsleft--, c = *(mm->in.charptr++)) == '\n') {
			mm->pd.num_directives++;
			return;
		}
	}

next_argument:
	while (mm->in.charsleft && (mm->in.charsleft--, c = *(mm->in.charptr++)) == ' ');
	if (c == '\n') {
		return;
	}

	if (dir->argc >= MMPARSE_DIRECTIVE_MAX_ARGS) {
		mmparse_failmsg(mm, "increase MMPARSE_DIRECTIVE_MAX_ARGS");
		assert(0);
	}

	arg_name_len = 0;
	dir->args[dir->argc].name[0] = 0;
	dir->args[dir->argc].value[0] = 0;
	for (;;) {
		switch (c) {
		case ',':
			if (!arg_name_len) {
				mmparse_failmsg(mm, "found comma while arg name is still empty");
				assert(0);
			}
			dir->argc++;
			goto next_directive;
		case ' ':
			if (!arg_name_len) {
				mmparse_failmsg(mm, "found space while arg name is still empty");
				assert(0);
			}
			dir->argc++;
			goto next_argument;
		case '=':
			if (!arg_name_len) {
				mmparse_failmsg(mm, "found eq while arg name is still empty");
				assert(0);
			}
			goto argument_value;
		}
		if (arg_name_len >= MMPARSE_DIRECTIVE_ARGN_MAX_LEN - 1) {
			mmparse_failmsg(mm, "increase MMPARSE_DIRECTIVE_ARGN_MAX_LEN");
			assert(0);
		}
		dir->args[dir->argc].name[arg_name_len++] = c;
		dir->args[dir->argc].name[arg_name_len] = 0;
		if (!mm->in.charsleft || (mm->in.charsleft--, c = *(mm->in.charptr++)) == '\n') {
			dir->argc++;
			return;
		}
	}

argument_value:
	if (!mm->in.charsleft-- || (c = *(mm->in.charptr++)) == ' ' || c == '\n') {
		mmparse_failmsg(mm, "space or EOL after 'directive argname=' is not allowed");
		assert(0);
	}
	if (c == '"' || c == '\'') {
		val_uses_quotes = c;
		if (!mm->in.charsleft--) {
			assert(0);
		}
		c = *(mm->in.charptr++);
	} else {
		val_uses_quotes = 0;
	}
	arg_val_len = 0;
	for (;;) {
		if (val_uses_quotes) {
			if (c == val_uses_quotes) { /*TODO: escapes?*/
				if (!mm->in.charsleft || (mm->in.charsleft--, c = *(mm->in.charptr++)) == '\n') {
					dir->argc++;
					return;
				}
				goto donewith;
			}
		} else {
donewith:
			switch (c) {
			case ',':
				dir->argc++;
				goto next_directive;
			case ' ':
				dir->argc++;
				goto next_argument;
			}
		}
		if (arg_val_len >= MMPARSE_DIRECTIVE_ARGV_MAX_LEN - 1) {
			mmparse_failmsg(mm, "increase MMPARSE_DIRECTIVE_ARGV_MAX_LEN");
			assert(0);
		}
		dir->args[dir->argc].value[arg_val_len++] = c;
		dir->args[dir->argc].value[arg_val_len] = 0;
		if (!mm->in.charsleft || (mm->in.charsleft--, c = *(mm->in.charptr++)) == '\n') {
			if (val_uses_quotes) {
				mmparse_failmsg(mm, "EOL while in quoted argument value");
				assert(0);
			}
			dir->argc++;
			return;
		}
	}
}
/*jeanine:p:i:5;p:3;a:r;x:27.72;y:32.64;*/
static
void mmparse_expand_line(struct mmparse *mm)
{
	static struct mmp_directive_content_data dir_data;

	int open_pos, close_pos, open_idx, close_idx, len, raw_pos, raw_line_len;
	enum mmp_directive_content_action eat_contents;
	char *raw_line;

	open_idx = 0;
	close_idx = 0;
	raw_pos = 0;
	raw_line = mm->pd.line_raw;
	raw_line_len = mm->pd.line_len;
	mm->pd.line_len = 0;
	mm->pd.line = mm->pd.line_expanded;
	eat_contents = LEAVE_CONTENTS;

	for (;;) {
		open_pos = 0x0FFFFFFF;
		if (open_idx < mm->pd.num_open_marks) {
			open_pos = mm->pd.open_mark_positions[open_idx];
		}
		close_pos = 0x0FFFFFFF;
		if (close_idx < mm->pd.num_close_marks) {
			close_pos = mm->pd.close_mark_positions[close_idx];
		}

		/*The whole 'eat_contents' thing will only work well if
		there is no nested stuff.. Otherwise this could use a
		rework so it handles directives 'inside out'.*/
		/*But then we'd need to take into account that placeholders may not
		be ordered anymore, since right now they always go from left to right,
		but that may be different once the order or handling directives changes.*/
		if (open_pos < close_pos) {
			len = open_pos - raw_pos;
			if (len) {
				mmparse_append_to_expanded_line(mm, raw_line + raw_pos, len);/*jeanine:s:a:r;i:6;*/
			}
			raw_pos += len + 1;
			if (mm->pd.tag_stack_size >= MMPARSE_TAGSTACK_SIZE) {
				mmparse_failmsg(mm, "increase MMPARSE_TAGSTACK_SIZE");
				assert(0);
			}
			mm->pd.next_placeholder_offset = mm->op.current_part->data0_len + mm->pd.line_len;
			if (close_pos == 0x0FFFFFFF) {
				dir_data.content_len = 0;
				dir_data.contents[0] = 0;
			} else {
				dir_data.content_len = len = close_pos - open_pos - 1;
				memcpy(dir_data.contents, raw_line + open_pos + 1,len);
				dir_data.contents[len] = 0;
			}
			mm->pd.tag_opened_on_line[mm->pd.tag_stack_size] = mm->in.current_line;
			mm->pd.tag_length[mm->pd.tag_stack_size] = 0;
			mm->pd.tag_stack_size++;
			dir_data.directive = mm->pd.directives + open_idx;
			mm->can |= MMPARSE_CAN_APPEND_CLOSING | MMPARSE_CAN_ALLOCATE_PH;
			eat_contents = mm->md.current->directive(mm, &dir_data);
			mm->can &= ~(MMPARSE_CAN_APPEND_CLOSING | MMPARSE_CAN_ALLOCATE_PH);
			open_idx++;
		} else if (close_pos < open_pos) {
			len = close_pos - raw_pos;
			if (len && eat_contents == LEAVE_CONTENTS) {
				mmparse_append_to_expanded_line(mm, raw_line + raw_pos, len);/*jeanine:s:a:r;i:6;*/
			}
			raw_pos += len + 1;

			close_idx++;

			if (!mm->pd.tag_stack_size--) {
				mmparse_failmsgf(mm, "tag stack empty at column %d", close_pos + 1);
				assert(0);
			}

			if (eat_contents == LEAVE_CONTENTS) {
				len = mm->pd.tag_length[mm->pd.tag_stack_size];
				mm->pd.tagbuf -= len;
				mmparse_append_to_expanded_line(mm, mm->pd.tagbuf, len);/*jeanine:s:a:r;i:6;*/
			} else {
				eat_contents = LEAVE_CONTENTS;
			}
		} else {
			len = raw_line_len - raw_pos;
			mmparse_append_to_expanded_line(mm, raw_line + raw_pos, len);
			break;
		}
	}
}
/*jeanine:p:i:3;p:12;a:r;x:16.39;y:-111.56;*/
static
void mmparse_read_line(struct mmparse *mm)
{
	char c, *line, last_char, num_consecutive_pipes;
	int line_len;

	mm->in.current_line++;
	mm->pd.hasmargin = 0;
	mm->pd.num_open_marks = 0;
	mm->pd.num_close_marks = 0;
	mm->pd.num_directives = 0;
	mm->pd.line = line = mm->pd.line_raw;
	line_len = 0;
	if (mm->md.current->parse_lines == MMPARSE_DONT_PARSE_LINES) {
		while (mm->in.charsleft && (mm->in.charsleft--, c = *(mm->in.charptr++)) != '\n') {
			line[line_len++] = c;
		}
		line[line_len] = 0;
		mm->pd.line_len = line_len;
		return;
	}
	last_char = 0;
	num_consecutive_pipes = 0;
	while (mm->in.charsleft && (mm->in.charsleft--, c = *(mm->in.charptr++)) != '\n') {
		if (c == '|') {
			num_consecutive_pipes++;
			if (num_consecutive_pipes == 3) {
				mm->pd.hasmargin = 1;
				line_len -= 2; /*Because 2 of them are already read.*/
				while (line_len && (line[line_len - 1] == ' ' || line[line_len - 1] == '\t')) {
					line_len--;
				}
				line[line_len] = 0;
				mm->pd.line_len = line_len;
				mmparse_read_directives(mm);/*jeanine:r:i:2;*/
				if (mm->pd.num_open_marks != mm->pd.num_directives) {
					mmparse_failmsgf(mm,
						"got %d directive(s) but %d mark(s)",
						mm->pd.num_directives,
						mm->pd.num_open_marks
					);
					assert(0);
				}
				mm->can |= MMPARSE_CAN_APPEND_EXPANDED;
				mmparse_expand_line(mm);/*jeanine:r:i:5;*/
				mm->can &= ~MMPARSE_CAN_APPEND_EXPANDED;
				return;
			}
		} else {
			num_consecutive_pipes = 0;
		}
		if (c == mm->pd.ctrlchar_open) {
			if (last_char == '\\') {
				last_char = 0;
				line_len--;
				continue;
			}
			mm->pd.open_mark_positions[mm->pd.num_open_marks++] = line_len;
			last_char = 0;
		} else if (c == mm->pd.ctrlchar_close) {
			if (last_char == '\\') {
				last_char = 0;
				line_len--;
				continue;
			}
			mm->pd.close_mark_positions[mm->pd.num_close_marks++] = line_len;
			last_char = 0;
		} else {
			switch (c) {
			case '&':
				line[line_len++] = '&';
				line[line_len++] = 'a';
				line[line_len++] = 'm';
				line[line_len++] = 'p';
				line[line_len++] = ';';
				last_char = c;
				continue;
			case '>':
				line[line_len++] = '&';
				line[line_len++] = 'g';
				line[line_len++] = 't';
				line[line_len++] = ';';
				last_char = c;
				continue;
			case '<':
				line[line_len++] = '&';
				line[line_len++] = 'l';
				line[line_len++] = 't';
				line[line_len++] = ';';
				last_char = c;
				continue;
			}
		}
		last_char = c;
		line[line_len++] = c;
		if (line_len > MMPARSE_LINE_RAW_MAX_LEN - /*leeway for '&amp;' etc replacements*/6) {
			mmparse_failmsg(mm, "increase MMPARSE_LINE_RAW_MAX_LEN");
			assert(0);
		}
	}
	line[line_len] = 0;
	mm->pd.line_len = line_len;
}
/*jeanine:p:i:15;p:6;a:t;x:42.33;*/
struct mmp_placeholder* mmparse_allocate_placeholder(struct mmparse *mm, void (*action)(struct mmparse*,struct mmp_output_part*,void*,int), int data_size)
{
	struct mmp_placeholder *ph;

	assert(((void)"invalid state", mm->can & MMPARSE_CAN_ALLOCATE_PH));
	if (mm->ph.databuf_sizeleft < data_size) {
		mmparse_failmsg(mm, "output buffer data3 too small");
		assert(0);
	}
	if (mm->ph.size >= MMPARSE_MAX_PLACEHOLDERS) {
		mmparse_failmsg(mm, "increase MMPARSE_MAX_PLACEHOLDERS");
		assert(0);
	}
	ph = mm->ph.placeholders + mm->ph.size++;
	ph->offset = mm->pd.next_placeholder_offset;
	ph->line_number = mm->in.current_line;
	ph->action = action;
	ph->data_size = data_size;
	ph->data = mm->ph.databuf;
	mm->ph.databuf += data_size;
	mm->ph.databuf_sizeleft -= data_size;
	return ph;
}
/*jeanine:p:i:1;p:8;a:t;x:143.93;*/
void mmparse_print_tag_with_directives(struct mmparse *mm, struct mmp_directive *dir, char *tagclose)
{
	static char tmp[MMPARSE_LINE_RAW_MAX_LEN];

	int i, len;

	len = sprintf(tmp, "<%s", dir->name);
	mmparse_append_to_expanded_line(mm, tmp, len);
	for (i = 0; i < dir->argc; i++) {
		if (dir->args[i].name[0]) {
			len = sprintf(tmp, " %s=\"%s\"", dir->args[i].name, dir->args[i].value);
			mmparse_append_to_expanded_line(mm, tmp, len);
		}
	}
	mmparse_append_to_expanded_line(mm, tagclose, strlen(tagclose));
}
/*jeanine:p:i:7;p:8;a:r;x:19.67;y:9.22;*/
static
void mmparse_cb_mode_nop_start_end(struct mmparse *mm)
{
}
/*jeanine:p:i:18;p:8;a:r;x:19.67;*/
static
int mmparse_cb_mode_nop_println(struct mmparse *mm)
{
	return 0;
}
/*jeanine:p:i:19;p:8;a:r;x:19.44;y:50.31;*/
static
enum mmp_directive_content_action mmparse_cb_mode_nop_directive(struct mmparse *mm, struct mmp_directive_content_data *data)
{
	return LEAVE_CONTENTS;
}
/*jeanine:p:i:9;p:8;a:r;x:19.55;y:28.32;*/
static
int mmparse_cb_mode_normal_println(struct mmparse *mm)
{
	register int len;

	if ((len = mm->pd.line_len)) {
		mm->pd.line[len++] = '\n';
		mmparse_append_to_main_output(mm, mm->pd.line, len);/*jeanine:s:a:r;i:13;*/
	}
	return 0;
}
/*jeanine:p:i:10;p:8;a:r;x:19.67;y:4.63;*/
static
enum mmp_directive_content_action mmparse_cb_mode_normal_directive(mm, data)
	struct mmparse *mm;
	struct mmp_directive_content_data *data;
{
	struct mmp_directive_handler *handler;

	for (handler = mm->config.directive_handlers; handler->name; handler++) {
		if (!strcmp(data->directive->name, handler->name)) {
			return handler->handle(mm, data);
		}
	}
	mmparse_print_tag_with_directives(mm, data->directive, ">");/*jeanine:s:a:r;i:1;*/
	mmparse_append_to_closing_tag(mm, "</", 2);/*jeanine:s:a:r;i:20;*/
	mmparse_append_to_closing_tag(mm, data->directive->name, strlen(data->directive->name));/*jeanine:s:a:r;i:20;*/
	mmparse_append_to_closing_tag(mm, ">", 1);/*jeanine:s:a:r;i:20;*/
	return LEAVE_CONTENTS;
}
/*jeanine:p:i:8;p:0;a:t;x:17.22;y:-32.81;*/
struct mmp_mode mmparse_mode_nop = {
	mmparse_cb_mode_nop_start_end,/*jeanine:r:i:7;*/
	mmparse_cb_mode_nop_println,/*jeanine:r:i:18;*/
	mmparse_cb_mode_nop_start_end,/*jeanine:s:a:r;i:7;*/
	mmparse_cb_mode_nop_directive,/*jeanine:r:i:19;*/
	"nop",
	MMPARSE_DONT_PARSE_LINES
};

struct mmp_mode mmparse_mode_normal = {
	mmparse_cb_mode_nop_start_end,/*jeanine:s:a:r;i:7;*/
	mmparse_cb_mode_normal_println,/*jeanine:r:i:9;*/
	mmparse_cb_mode_nop_start_end,/*jeanine:s:a:r;i:7;*/
	mmparse_cb_mode_normal_directive,/*jeanine:r:i:10;*/
	"normal",
	MMPARSE_DO_PARSE_LINES
};

struct mmp_mode mmparse_mode_plain = {
	mmparse_cb_mode_nop_start_end,/*jeanine:s:a:r;i:7;*/
	mmparse_cb_mode_normal_println,/*jeanine:s:a:r;i:9;*/
	mmparse_cb_mode_nop_start_end,/*jeanine:s:a:r;i:7;*/
	mmparse_cb_mode_nop_directive,/*jeanine:s:a:r;i:19;*/
	"plain",
	MMPARSE_DONT_PARSE_LINES
};
/*jeanine:p:i:11;p:12;a:r;x:16.08;y:-7.00;*/
/**
@return 0 when no dotcommands were present
*/
static
int mmparse_handle_dotcommands(struct mmparse *mm)
{
	struct mmp_mode **mode, *tmp_current_mode;
	char *line_start, had_cmds;

	line_start = mm->pd.line;
	while (*line_start == ' ') {
		line_start++;
	}
	had_cmds = 0;
more_dotcommands:
	if (!strncmp(line_start, ".controlchars ", 14)) {
		mm->pd.ctrlchar_open = line_start[14];
		mm->pd.ctrlchar_close = line_start[15];
		if (line_start[16] == ' ' && line_start[17] == '.') {
			line_start += 17;
			had_cmds = 1;
			goto more_dotcommands;
		}
		return 1;
	} else if (!strncmp(line_start, ".pushmode ", 10)) {
		if (mm->md.num_pushed_modes == MMPARSE_MODE_STACK_SIZE) {
			mmparse_failmsg(mm, "increase MMPARSE_MODE_STACK_SIZE");
			assert(0);
		}
		for (mode = mm->config.modes; *mode; mode++) {
			if (!strcmp(line_start + 10, (*mode)->name)) {
				mm->md.pushed_modes[mm->md.num_pushed_modes++] = mm->md.current;
				mm->md.current = *mode;
				mm->can |= MMPARSE_CAN_APPEND_MAIN;
				mm->md.current->start(mm);
				mm->can &= ~MMPARSE_CAN_APPEND_MAIN;
				return 1;
			}
		}
		mmparse_failmsgf(mm, "unknown mode '%s'", line_start + 10);
		assert(0);
	} else if (!strncmp(line_start, ".popmode ", 9)) {
		if (!mm->md.num_pushed_modes) {
			mmparse_failmsg(mm, "trying to pop mode but none is pushed");
			assert(0);
		}
		if (strcmp(line_start + 9, mm->md.current->name)) {
			mmparse_failmsgf(mm,
				"trying to pop mode '%s' but current is '%s'",
				line_start + 9,
				mm->md.current->name
			);
			assert(0);
		}
		tmp_current_mode = mm->md.current;
		mm->md.current = mm->md.pushed_modes[--mm->md.num_pushed_modes];
		mm->can |= MMPARSE_CAN_APPEND_MAIN;
		tmp_current_mode->end(mm);
		mm->can &= ~MMPARSE_CAN_APPEND_MAIN;
		return 1;
	}
	return had_cmds;
}
/*jeanine:p:i:16;p:12;a:r;x:16.01;y:54.91;*/
/**
Split parts based on placeholders.

Since every part contains a buffer of main contents and a buffer of placeholder contents,
The current part needs to be split into 'num_placeholders+1' part(s).

Case study:
part0.data0: this is the previous line
             (stuff inserted by mode 'println' function)this is the current line with a placeholder and stuff
             ^ a                                        ^ extra_line_offset  ^ placeholder.offset            ^ part0.data0_len
extra_line_offset is from 'a'
placeholder.offset is from the start of data0
after:
part0.data0: this is the previous line
             (stuff inserted by mode 'println' function)this is the current l
                                                                             ^ part0.data0_len
part1.data0: ine with a placeholder and stuff
                                             ^ part1.data0_len
*/
static
void mmparse_split_parts(struct mmparse *mm, int extra_line_offset, int prev_ph_size)
{
	int original_part_len, i, last_ph_offset, delta, off;
	struct mmp_output_part *part;
	struct mmp_placeholder *ph;

	part = mm->op.current_part;
	original_part_len = part->data0_len;
	last_ph_offset = 0;
	for (i = 0, ph = mm->ph.placeholders + prev_ph_size; i < mm->ph.size - prev_ph_size; i++, ph++) {
		off = ph->offset + extra_line_offset;
		delta = off - last_ph_offset;
		part->data0_len = delta;
		(part + 1)->data0 = part->data0 + delta;
		part++;
		last_ph_offset = off;
	}
	part->data0_len = original_part_len - last_ph_offset;
	mm->op.current_part = part;
}
/*jeanine:p:i:12;p:0;a:b;y:29.38;*/
/**
When returning, call 'mmparse_process_placeholders' (or not if no placeholders are used),
then the final output can be constructed as following:
    struct mmp_output_part *part;
    for (part = mm->output; part->data0; part++) {
      write(part->data0, part->data0_len);
      write(part->data1, part->data1_len);
    }

@param mm allocated 'struct mmparse' with its 'config' member fully set.
          All other data may be undefined and will be overwritten.
*/
void mmparse(struct mmparse *mm)
{
	int prev_ph_size, extra_line_offset;
	struct mmp_config config_copy;
	char *tag;
	int len;

	config_copy = mm->config;
	memset(mm, 0, sizeof(struct mmparse));
	mm->config = config_copy;

	mm->in.charptr = mm->config.source;
	mm->in.charsleft = mm->config.source_len;
	mm->pd.ctrlchar_open = '{';
	mm->pd.ctrlchar_close = '}';
	mm->md.current = mm->config.modes[0];
	assert(((void)"mmparse: must have at least one mode", mm->md.current));
	mm->op.data0buf_sizeleft = mm->config.dest.data0_len;
	mm->op.data1buf_sizeleft = mm->config.dest.data1_len;
	mm->pd.tagbuf_sizeleft = mm->config.dest.data2_len;
	mm->ph.databuf_sizeleft = mm->config.dest.data3_len;
	mm->pd.tagbuf = mm->config.dest.data2;
	mm->ph.databuf = mm->config.dest.data3;
	mm->op.current_part = mm->op.parts;
	mm->op.current_part->data0 = mm->config.dest.data0;
	mm->op.current_part->data1 = mm->config.dest.data1;
	mm->output = mm->op.current_part;

	mm->can |= MMPARSE_CAN_APPEND_MAIN;
	mm->md.current->start(mm);
	mm->can &= ~MMPARSE_CAN_APPEND_MAIN;
	while (mm->in.charsleft) {
		prev_ph_size = mm->ph.size;
		mmparse_read_line(mm);/*jeanine:r:i:3;*/
		if (mm->pd.hasmargin || !mmparse_handle_dotcommands(mm)) {/*jeanine:r:i:11;*/
			mm->can |= MMPARSE_CAN_APPEND_MAIN;
			extra_line_offset = mm->md.current->println(mm);
			mm->can &= ~MMPARSE_CAN_APPEND_MAIN;
			if (prev_ph_size != mm->ph.size) {
				mmparse_split_parts(mm, extra_line_offset, prev_ph_size);/*jeanine:r:i:16;*/
			}
		}
	}
	if (mm->pd.tag_stack_size) {
		len = mm->pd.tag_length[mm->pd.tag_stack_size - 1];
		tag = alloca(len + 1);
		memcpy(tag, mm->pd.tagbuf - len, len);
		tag[len] = 0;
		mmparse_failmsgf(mm,
			"still have %d entries in tag stack, first is '%s', opened on line %d",
			mm->pd.tag_stack_size,
			tag,
			mm->pd.tag_opened_on_line[mm->pd.tag_stack_size - 1]
		);
		assert(0);
	}
	if (mm->md.num_pushed_modes) {
		mmparse_failmsgf(mm,
			"still have %d pushed mode(s), current is '%s'",
			mm->md.num_pushed_modes,
			mm->md.current->name
		);
		assert(0);
	}
	mm->can |= MMPARSE_CAN_APPEND_MAIN;
	mm->md.current->end(mm);
	mm->can &= ~MMPARSE_CAN_APPEND_MAIN;
}

void mmparse_process_placeholders(struct mmparse *mm)
{
	struct mmp_output_part *lp, *cp;
	struct mmp_placeholder *ph;
	int i;

	lp = NULL;
	for (i = 0, ph = mm->ph.placeholders, cp = mm->op.parts; i < mm->ph.size; i++, ph++, cp++) {
		if (lp) {
			cp->data1 = lp->data1 + lp->data1_len;
		}
		/*Set the current line number to the line where the placeholder was allocated,
		so any possible errors reported by mmparse_failmsg will have a relevant line number.*/
		mm->in.current_line = ph->line_number;
		mm->can |= MMPARSE_CAN_APPEND_PH;
		ph->action(mm, cp, ph->data, ph->data_size);
		mm->can &= ~MMPARSE_CAN_APPEND_PH;
		lp = cp;
	}
}
