/**
mmparse extras, some extra things to be used with mmparse
=========================================================

To use (some of) these things, add 'struct mmpextras_userdata mmpextras' into your userdata struct that
gets assigned to 'mmparse->config.userdata' and define a function 'mmpextras_get_userdata' as follows:

static
struct mmpextras_userdata *mmpextras_get_userdata(struct mmparse *mm)
{
	return &((struct my_userdata_struct*) mm->config.userdata)->mmpextras;
}


function mmpextras_find_directive_argument
------------------------------------------
Finds an argument, so you don't have to loop through all the arguments and match the name yourself


function mmpextras_require_directive_argument
---------------------------------------------
Like mmpextras_find_directive_argument, but aborts if the argument is not found.


a 'a' directive for easier link generation
------------------------------------------
To use,  add '{ "a", mmpextras_dir_a }' in your array of directives.

Links can already be made fairly easily by doing:
{my link text} ||| a href='example.com'
result:
<a href="example.com">my link text</a>

With mmpextras_dir_a, it will also use the link text as destination if there is no href argument:
{http://example.com} ||| a
result:
<a href="http://example.com">http://example.com</a>


a 'ul' mode to output lists
---------------------------
To use, add '&mmpextras_mode_ul' in your array of modes.

Example usage:

.pushmode ul
- this is a listitem
- this is another listitem
  which is declared on two lines
  .pushmode ul
  - and it has a sublist
  - with two items
  .popmode ul
- more items
- okay
.popmode ul


a 'pre' mode to output code
---------------------------
To use, add '&mmpextras_mode_pre' in your array of modes.

Example usage:

.pushmode pre
this is code

nothing special
.popmode pre

This whole pre block can be indented,
the mode will take care of indents and strip them so the outputted formatted code will not be indented.


a 'paragraphed' mode to add paragraphs
--------------------------------------
To use, add '&mmpextras_mode_paragraphed' in your array of modes.

Paragraphs inside this mode will be wrapped in <p></p> tags; but in order for this to work correctly,
ensure to leave at least one whitespace line before and after the text block that needs to be paragraphed.


a 'section' mode to create sections
-----------------------------------
Paragraphs inside sections will be wrapped in <p></p> tags; but in order for this to work correctly,
ensure to leave at least one whitespace line before and after the text block that needs to be paragraphed.
Each section can have one or more headers (by using mmpextras_dir_h),
and those will be used to generate a table of contents (with mmpextras_dir_index) and breadcrumbs.
Each header can be linked to by using mmpextras_dir_href.

To use:
- add '&mmpextras_mode_section' in your array of modes
- add '{ "h", mmpextras_dir_h }' in your array of directives (required)
- add '{ "href", mmpextras_dir_href }' in your array of directives (optional)
- add '{ "index", mmpextras_dir_index } ' in your array of directives (optional)

Example usage:

.pushmode section

{Intro} ||| h id=intro

This is a paragraph
and this text is also in the same paragraph.

A line break was here, so this line is in a new paragraph

.popmode section
.pushmode section

{Index} ||| h id=index

{} ||| index

.popmode section
.pushmode section

{level one} ||| h id=level1

This is level one

.pushmode section

{level two} ||| h id=two

This is level two

I can link to {level1} ||| href

.pushmode section

{level three} ||| h id=three

And three

.popmode section
.popmode section

and we're back in level one

.popmode section
*/
/*jeanine:p:i:1;p:0;a:b;y:1.88;*/
static
struct mmp_dir_arg *mmpextras_find_directive_argument(struct mmp_dir *dir, char *name)
{
	register int i;

	for (i = 0; i < dir->argc; i++) {
		if (!strcmp(dir->args[i].name, name)) {
			return dir->args + i;
		}
	}
	return NULL;
}
/*jeanine:p:i:2;p:1;a:b;y:1.88;*/
/**
Aborts if argument not found.
*/
static
struct mmp_dir_arg *mmpextras_require_directive_argument(struct mmparse *mm, struct mmp_dir *dir, char *name)
{
	struct mmp_dir_arg *arg;

	arg = mmpextras_find_directive_argument(dir, name);
	if (!arg) {
		mmparse_failmsgf(mm, "required argument '%s' not present on directive '%s'", name, dir->name);
		assert(0);
	}
	return arg;
}
/*jeanine:p:i:3;p:2;a:b;y:1.88;*/
struct mmpextras_index_entry {
	int level;
	char name_len;
	char name[50];
	char id_len;
	char id[30];
};
struct mmpextras_userdata {
	struct {
		int num_entries;
		struct mmpextras_index_entry entries[50];
	} index;
	struct {
		char has_p;
		char should_open_p;
	} paragraphed;
	struct {
		/**To prevent two or more 'index' links when multiple sections close after each other.*/
		char needs_index_link_at_bottom;
		unsigned char current_level;
		/**If doing a println and this is higher than 'current_level',
		we need to print continuation breadcrumbs*/
		char last_level_that_had_println;
#define MMPEXTRAS_MAX_NESTED_SECTIONS 10
		/**The index entry index that is applicable for this section.
		Used to print continuation breadcrumbs.*/
		int matching_index_entry_idx[MMPEXTRAS_MAX_NESTED_SECTIONS];
	} section;
	struct {
		int whitespacelen;
	} pre;
	struct {
#define MMPEXTRAS_MAX_NEXTED_UL 10
		char li_open[MMPEXTRAS_MAX_NEXTED_UL];
		unsigned char level;
	} ul;
};
static struct mmpextras_userdata *mmpextras_get_userdata(struct mmparse *mm);
/*jeanine:p:i:9;p:15;a:r;x:5.56;y:-21.00;*/
static
void mmpextras_cb_mode_pre_start(struct mmparse *mm)
{
	struct mmpextras_userdata *ud;
	register int wslen;

	wslen = 0;
	while (mm->pd.line[wslen] == ' ') {
		wslen++;
	}
	ud = mmpextras_get_userdata(mm);
	ud->pre.whitespacelen = wslen;
	mmparse_append_to_main_output(mm, "<pre>\n", 6);
}
/*jeanine:p:i:10;p:15;a:r;x:5.56;y:-4.00;*/
static
int mmpextras_cb_mode_pre_println(struct mmparse *mm)
{
	struct mmpextras_userdata *ud;
	register int wslen;

	ud = mmpextras_get_userdata(mm);
	wslen = ud->pre.whitespacelen;
	if (mm->pd.line_len > wslen) {
		mmparse_append_to_main_output(mm, mm->pd.line + wslen, mm->pd.line_len - wslen);
	}
	mmparse_append_to_main_output(mm, "\n", 1);
	return mm->pd.line_len - wslen;
}
/*jeanine:p:i:11;p:15;a:r;x:5.56;y:12.00;*/
static
void mmpextras_cb_mode_pre_end(struct mmparse *mm)
{
	mmparse_append_to_main_output(mm, "</pre>\n", 7);
}
/*jeanine:p:i:15;p:3;a:b;y:28.42;*/
struct mmp_mode mmpextras_mode_pre = {
	mmpextras_cb_mode_pre_start,/*jeanine:r:i:9;*/
	mmpextras_cb_mode_pre_println,/*jeanine:r:i:10;*/
	mmpextras_cb_mode_pre_end,/*jeanine:r:i:11;*/
	mmparse_cb_mode_normal_directive,
	"pre",
	MMPARSE_DO_PARSE_LINES
};
/*jeanine:p:i:12;p:16;a:r;x:5.56;y:-33.00;*/
static
void mmpextras_cb_mode_ul_start(struct mmparse *mm)
{
	struct mmpextras_userdata *ud;

	ud = mmpextras_get_userdata(mm);
	mmparse_append_to_main_output(mm, "<ul>", 4);
	ud->ul.level++;
	assert(ud->ul.level < MMPEXTRAS_MAX_NEXTED_UL);
	ud->ul.li_open[ud->ul.level] = 0;
}
/*jeanine:p:i:13;p:16;a:r;x:5.56;y:-19.00;*/
static
int mmpextras_cb_mode_ul_println(struct mmparse *mm)
{
	struct mmpextras_userdata *ud;
	char *line;
	int len;

	ud = mmpextras_get_userdata(mm);
	if (mm->pd.line_len) {
		line = mm->pd.line;
		len = mm->pd.line_len;
		line[len++] = '\n';
		while (*line == ' ' && len) {
			line++;
			len--;
		}
		if (line[0] == '-' && line[1] == ' ') {
			if (ud->ul.li_open[ud->ul.level]) {
				mmparse_append_to_main_output(mm, "</li>", 5);
			}
			ud->ul.li_open[ud->ul.level] = 1;
			mmparse_append_to_main_output(mm, "<li>", 4);
			len -= 2;
			line += 2;
		} else {
			line = mm->pd.line;
			len = mm->pd.line_len;
		}
		mmparse_append_to_main_output(mm, line, len);
		return -2;
	} else {
		return 0;
	}
}
/*jeanine:p:i:14;p:16;a:r;x:5.56;y:17.00;*/
static
void mmpextras_cb_mode_ul_end(struct mmparse *mm)
{
	struct mmpextras_userdata *ud;

	ud = mmpextras_get_userdata(mm);
	if (ud->ul.li_open[ud->ul.level]) {
		mmparse_append_to_main_output(mm, "</li>", 5);
	}
	ud->ul.level--;
	mmparse_append_to_main_output(mm, "</ul>", 5);
}
/*jeanine:p:i:16;p:15;a:b;y:50.81;*/
struct mmp_mode mmpextras_mode_ul = {
	mmpextras_cb_mode_ul_start,/*jeanine:r:i:12;*/
	mmpextras_cb_mode_ul_println,/*jeanine:r:i:13;*/
	mmpextras_cb_mode_ul_end,/*jeanine:r:i:14;*/
	mmparse_cb_mode_normal_directive,
	"ul",
	MMPARSE_DO_PARSE_LINES
};
/*jeanine:p:i:6;p:27;a:r;x:3.33;*/
static
int mmpextras_line_can_have_paragraph(struct mmparse *mm)
{
	register char *tag;

	if (mm->pd.line[0] != '<') {
		return 1;
	}
	tag = mm->pd.line + 1;
	if (*tag == '/') {
		tag++;
	}
	return *tag != 'h' && *tag != 'p' &&
		strncmp(tag, "details", 7) &&
		strncmp(tag, "table", 5) &&
		strncmp(tag, "ul", 2);
}
/*jeanine:p:i:26;p:28;a:r;x:11.11;y:-8.25;*/
static
void mmpextras_cb_mode_paragraphed_start(struct mmparse *mm)
{
	struct mmpextras_userdata *ud;

	ud = mmpextras_get_userdata(mm);
	ud->paragraphed.should_open_p = 1;
}
/*jeanine:p:i:27;p:28;a:r;x:205.32;*/
static
int mmpextras_cb_mode_paragraphed_println(struct mmparse *mm)
{
	struct mmpextras_userdata *ud;
	char is_empty_line;
	char extra_offset;

	ud = mmpextras_get_userdata(mm);
	is_empty_line = !mm->pd.line_len;
	if (is_empty_line) {
		if (ud->paragraphed.has_p) {
			ud->paragraphed.has_p = 0;
			extra_offset = 4;
			mmparse_append_to_main_output(mm, "</p>", 4);
		}
		ud->paragraphed.should_open_p = 1;
	} else {
		if (ud->paragraphed.should_open_p) {
			ud->paragraphed.should_open_p = 0;
			if (mmpextras_line_can_have_paragraph(mm)) {/*jeanine:r:i:6;*/
				ud->paragraphed.has_p = 1;
				extra_offset = 3;
				mmparse_append_to_main_output(mm, "<p>", 3);
			}
		}
	}
	return extra_offset + mmparse_cb_mode_normal_println(mm);
}
/*jeanine:p:i:28;p:16;a:b;y:40.25;*/
struct mmp_mode mmpextras_mode_paragraphed = {
	mmpextras_cb_mode_paragraphed_start,/*jeanine:r:i:26;*/
	mmpextras_cb_mode_paragraphed_println,/*jeanine:r:i:27;*/
	mmparse_cb_mode_nop_start_end,
	mmparse_cb_mode_normal_directive,
	"paragraphed",
	MMPARSE_DO_PARSE_LINES
};
/*jeanine:p:i:4;p:25;a:r;x:40.30;y:-38.56;*/
static
void mmpextras_append_breadcrumbs(struct mmparse *mm, void (*append_func)(void*,struct mmparse*,char*,int), void *append_func_data, int for_index_entry_idx, int is_continuation)
{
	struct mmpextras_index_entry *entry;
	struct mmpextras_userdata *ud;
	int level;

	ud = mmpextras_get_userdata(mm);
	entry = ud->index.entries + for_index_entry_idx;
	append_func(append_func_data, mm, "<p id='", 7);
	append_func(append_func_data, mm, entry->id, entry->id_len);
	append_func(append_func_data, mm, "'><small># ", 11);
	level = entry->level;
	while (level--) {
		while ((--entry)->level != level);
		append_func(append_func_data, mm, "<a href='#", 10);
		append_func(append_func_data, mm, entry->id, entry->id_len);
		append_func(append_func_data, mm, "'>", 2);
		append_func(append_func_data, mm, entry->name, entry->name_len);
		append_func(append_func_data, mm, "</a> > ", 7);
		entry = ud->index.entries + for_index_entry_idx;
	}
	if (is_continuation) {
		append_func(append_func_data, mm, "<a href='#", 10);
		append_func(append_func_data, mm, entry->id, entry->id_len);
		append_func(append_func_data, mm, "'>", 2);
		append_func(append_func_data, mm, entry->name, entry->name_len);
		append_func(append_func_data, mm, "</a> (continuation)", 19);
	} else {
		append_func(append_func_data, mm, entry->name, entry->name_len);
	}
	append_func(append_func_data, mm, "</small></p>", 12);
}
/*jeanine:p:i:5;p:17;a:r;x:5.56;y:-30.00;*/
static
void mmpextras_cb_mode_section_start(struct mmparse *mm)
{
	struct mmpextras_userdata *ud;

	ud = mmpextras_get_userdata(mm);
	ud->paragraphed.should_open_p = 1;
	ud->section.current_level++;
	assert(ud->section.current_level < MMPEXTRAS_MAX_NESTED_SECTIONS);
	ud->section.matching_index_entry_idx[ud->section.current_level] = -1;
	ud->section.last_level_that_had_println = ud->section.current_level;
	mmparse_append_to_main_output(mm, "<div>", 5);
}
/*jeanine:p:i:7;p:24;a:r;x:68.17;*/
static
void mmpextras_breadcrumb_append_func_from_println(void *data, struct mmparse *mm, char *buf, int len)
{
	*((int*) data) += len;
	mmparse_append_to_main_output(mm, buf, len);
}
/*jeanine:p:i:24;p:17;a:r;x:5.56;y:-14.00;*/
static
int mmpextras_cb_mode_section_println(struct mmparse *mm)
{
	struct mmpextras_userdata *ud;
	int index_entry_idx;
	char extra_offset;

	ud = mmpextras_get_userdata(mm);
	ud->section.needs_index_link_at_bottom = 1;

	extra_offset = 0;
	if (ud->section.last_level_that_had_println > ud->section.current_level) {
		ud->section.last_level_that_had_println = ud->section.current_level;
		index_entry_idx = ud->section.matching_index_entry_idx[ud->section.current_level];
		if (index_entry_idx == -1) {
			mmparse_failmsg(mm, "trying to print continuation breadcrumbs for section, but no h element seen!");
			assert(0);
		}
		mmpextras_append_breadcrumbs(mm, mmpextras_breadcrumb_append_func_from_println, &extra_offset, index_entry_idx, 1);/*jeanine:r:i:7;:s:a:r;i:4;*/
	}

	return extra_offset + mmpextras_cb_mode_paragraphed_println(mm);/*jeanine:s:a:r;i:27;*/
}
/*jeanine:p:i:8;p:17;a:r;x:5.56;y:12.00;*/
static
void mmpextras_cb_mode_section_end(struct mmparse *mm)
{
	struct mmpextras_userdata *ud;

	ud = mmpextras_get_userdata(mm);
	ud->section.current_level--;
	if (ud->section.needs_index_link_at_bottom) {
		ud->section.needs_index_link_at_bottom = 0;
		mmparse_append_to_main_output(mm, "<p><small><a href='#index'>Index</a></small></p></div>", 54);
	} else {
		mmparse_append_to_main_output(mm, "</div>", 6);
	}
}
/*jeanine:p:i:17;p:28;a:b;y:38.88;*/
struct mmp_mode mmpextras_mode_section = {
	mmpextras_cb_mode_section_start,/*jeanine:r:i:5;*/
	mmpextras_cb_mode_section_println,/*jeanine:r:i:24;*/
	mmpextras_cb_mode_section_end,/*jeanine:r:i:8;*/
	mmparse_cb_mode_normal_directive,
	"section",
	MMPARSE_DO_PARSE_LINES
};
/*jeanine:p:i:18;p:19;a:r;x:3.33;*/
static
void mmpextras_cb_placeholder_index(struct mmparse *mm, struct mmp_output_part *output, void *data, int data_size)
{
	struct mmpextras_index_entry *entry;
	struct mmpextras_userdata *ud;
	int i, level, last_level;

	last_level = 0;
	ud = mmpextras_get_userdata(mm);
	mmparse_append_to_placeholder_output(mm, output, "<ul>\n", 5);
	for (i = 0, entry = ud->index.entries;; i++, entry++) {
		level = i < ud->index.num_entries ? entry->level : 0;
		if (level > last_level) {
			while (level > last_level++) {
				mmparse_append_to_placeholder_output(mm, output, "\n<ul>\n", 6);
			}
		} else if (level < last_level) {
			while (level < last_level--) {
				mmparse_append_to_placeholder_output(mm, output, "</li>\n</ul>\n", 12);
			}
			mmparse_append_to_placeholder_output(mm, output, "</li>\n", 6);
		} else if (i) {
			mmparse_append_to_placeholder_output(mm, output, "</li>\n", 6);
		}
		if (i >= ud->index.num_entries) {
			break;
		}
		last_level = level;
		mmparse_append_to_placeholder_output(mm, output, "<li><a href='#", 14);
		mmparse_append_to_placeholder_output(mm, output, entry->id, entry->id_len);
		mmparse_append_to_placeholder_output(mm, output, "'>", 2);
		mmparse_append_to_placeholder_output(mm, output, entry->name, entry->name_len);
		mmparse_append_to_placeholder_output(mm, output, "</a>", 4);
	}
	mmparse_append_to_placeholder_output(mm, output, "</ul>\n", 6);
}
/*jeanine:p:i:19;p:21;a:b;y:19.24;*/
static
enum mmp_dir_content_action mmpextras_dir_index(struct mmparse *mm, struct mmp_dir_content_data *data)
{
	mmparse_allocate_placeholder(mm, mmpextras_cb_placeholder_index, 0);/*jeanine:r:i:18;*/
	return LEAVE_CONTENTS;
}
/*jeanine:p:i:20;p:21;a:r;x:3.33;*/
static
void mmpextras_cb_placeholder_href(struct mmparse *mm, struct mmp_output_part *output, void *data, int data_size)
{
	struct mmpextras_index_entry *entry;
	struct mmpextras_userdata *ud;
	char *ref_id;
	int i;

	ref_id = data;
	ud = mmpextras_get_userdata(mm);
	for (i = ud->index.num_entries, entry = ud->index.entries; i; i--, entry++) {
		if (!strcmp(entry->id, ref_id)) {
			mmparse_append_to_placeholder_output(mm, output, "<a href='#", 10);
			mmparse_append_to_placeholder_output(mm, output, entry->id, entry->id_len);
			mmparse_append_to_placeholder_output(mm, output, "'>", 2);
			mmparse_append_to_placeholder_output(mm, output, entry->name, entry->name_len);
			mmparse_append_to_placeholder_output(mm, output, "</a>", 4);
			return;
		}
	}
	mmparse_failmsgf(mm, "cannot find a header with id '%s'", ref_id);
	assert(0);
}
/*jeanine:p:i:21;p:29;a:b;y:1.88;*/
static
enum mmp_dir_content_action mmpextras_dir_href(struct mmparse *mm, struct mmp_dir_content_data *data)
{
	struct mmp_placeholder *ph;

	ph = mmparse_allocate_placeholder(mm, mmpextras_cb_placeholder_href, data->content_len + 1);/*jeanine:r:i:20;*/
	memcpy(ph->data, data->contents, data->content_len + 1);
	return DELETE_CONTENTS;
}
/*jeanine:p:i:22;p:25;a:r;x:40.21;*/
static
void mmpextras_breadcrumb_append_func_from_placeholder(void *data, struct mmparse *mm, char *buf, int len)
{
	mmparse_append_to_placeholder_output(mm, data, buf, len);
}
/*jeanine:p:i:25;p:23;a:r;x:3.33;*/
static
void mmpextras_cb_placeholder_section_breadcrumbs(struct mmparse *mm, struct mmp_output_part *output, void *data, int data_size)
{
	int entry_idx;

	entry_idx = *((int*) data);
	mmpextras_append_breadcrumbs(mm, mmpextras_breadcrumb_append_func_from_placeholder, output, entry_idx, 0);/*jeanine:r:i:4;:r:i:22;*/
}
/*jeanine:p:i:23;p:17;a:b;y:28.27;*/
static
enum mmp_dir_content_action mmpextras_dir_h(struct mmparse *mm, struct mmp_dir_content_data *data)
{
	struct mmpextras_index_entry *entry;
	struct mmpextras_userdata *ud;
	register char *directive_name;
	struct mmp_dir_arg *id_arg;
	struct mmp_placeholder *ph;
	int level, i, id_len;

	ud = mmpextras_get_userdata(mm);

	level = 2;
	for (i = 0; i < mm->md.num_pushed_modes; i++) {
		if (mm->md.pushed_modes[i] == &mmpextras_mode_section) {
			level++;
			if (level >= 6) {
				break;
			}
		}
	}

	directive_name = data->directive->name;
	directive_name[1] = '0' + level;
	directive_name[2] = 0;
	level -= 2;
	id_arg = mmpextras_require_directive_argument(mm, data->directive, "id");
	id_len = strlen(id_arg->value);
	if (level) {
		id_arg->name[0] = 0; /*make it so the 'id' argument doesn't get printed as attribute*/
		ph = mmparse_allocate_placeholder(mm, mmpextras_cb_placeholder_section_breadcrumbs, 4);/*jeanine:r:i:25;*/
		*((int*) ph->data) = ud->index.num_entries;
	}

	mmparse_print_tag_with_directives(mm, data->directive, ">");
	mmparse_append_to_closing_tag(mm, " <a href='#", 11);
	mmparse_append_to_closing_tag(mm, id_arg->value, id_len);
	mmparse_append_to_closing_tag(mm, "'>#</a></", 9);
	mmparse_append_to_closing_tag(mm, directive_name, 2);
	mmparse_append_to_closing_tag(mm, ">", 1);

	if (ud->index.num_entries >= sizeof(ud->index.entries)/sizeof(ud->index.entries[0])) {
		mmparse_failmsg(mm, "increase number of index entries");
		assert(0);
	}
	if (ud->section.matching_index_entry_idx[ud->section.current_level] == -1) {
		ud->section.matching_index_entry_idx[ud->section.current_level] = ud->index.num_entries;
	}
	entry = ud->index.entries + ud->index.num_entries++;
	entry->level = level;
	if (data->content_len >= sizeof(ud->index.entries[0].name)) {
		mmparse_failmsgf(mm, "increase max length of index entry name (need %d)", data->content_len);
		assert(0);
	}
	entry->name_len = data->content_len;
	memcpy(entry->name, data->contents, data->content_len + 1);
	if (id_len >= sizeof(ud->index.entries[0].id)) {
		mmparse_failmsgf(mm, "increase max length of index entry id (need %d)", id_len);
		assert(0);
	}
	entry->id_len = id_len;
	memcpy(entry->id, id_arg->value, id_len + 1);
	return LEAVE_CONTENTS;
}
/*jeanine:p:i:29;p:23;a:b;y:1.88;*/
static
enum mmp_dir_content_action mmpextras_dir_a(struct mmparse *mm, struct mmp_dir_content_data *data)
{
	struct mmp_dir_arg *href;
	struct mmp_dir *dir;

	dir = data->directive;
	href = mmpextras_find_directive_argument(dir, "href");
	if (!href) {
		assert(((void)"can't add 'href' arg, increase MMPARSE_DIRECTIVE_MAX_ARGS", dir->argc < MMPARSE_DIRECTIVE_MAX_ARGS));
		assert(((void)"can't add 'href' arg, increase MMPARSE_DIRECTIVE_ARGV_MAX_LEN", data->content_len < MMPARSE_DIRECTIVE_ARGV_MAX_LEN));
		memcpy(dir->args[dir->argc].name, "href", 5);
		memcpy(dir->args[dir->argc].value, data->contents, data->content_len + 1);
		dir->argc++;
	}
	mmparse_print_tag_with_directives(mm, dir, ">");
	mmparse_append_to_closing_tag(mm, "</a>", 4);
	return LEAVE_CONTENTS;
}
