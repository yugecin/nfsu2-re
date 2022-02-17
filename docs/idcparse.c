/**
Code to _very_ naively parse an IDA .idc file, only targetted at parsing
files that are the output of a "dump database to IDC file" operation,
with the purpose to read common database stuff like enums, structs, functions etc.
*/

/*TODO: printf to stderr if they're for the next assert(0)*/

#define IDCP_MAX_TOKENS 6000000
#define IDCP_MAX_FUNCTIONS 500
/*Max args max be more than max parameters because functioncalls can
be to built-in functions which could have more parameters than any
function defined in the idc file.*/
#define IDCP_MAX_ARGUMENTS 10
#define IDCP_MAX_PARAMETERS 5
#define IDCP_MAX_LOCAL_VARIABLES 10
#define IDCP_MAX_ENUMS 50
#define IDCP_MAX_TOTAL_ENUM_MEMBERS 7000
#define IDCP_MAX_STRUCTS 300
#define IDCP_MAX_TOTAL_STRUCT_MEMBERS 14000
#define IDCP_MAX_STUFFS 325000

/*Some IDA definitions.*/
#define IDCP_IDA_SN_LOCAL 0x200

#define IDCP_VERBOSE_LEVEL 0
/*useful when debugging segfaults*/
#if 0
#define IDCP_FFLUSH fflush(stdout)
#else
#define IDCP_FFLUSH
#endif

/*verbose*/
#if IDCP_VERBOSE_LEVEL == 5
#define idcp_dprintf5(...) printf(__VA_ARGS__);IDCP_FFLUSH
#define idcp_dprintf4(...) printf(__VA_ARGS__);IDCP_FFLUSH
#define idcp_dprintf3(...) printf(__VA_ARGS__);IDCP_FFLUSH
#define idcp_dprintf2(...) printf(__VA_ARGS__);IDCP_FFLUSH
#define idcp_dprintf1(...) printf(__VA_ARGS__);IDCP_FFLUSH
#elif IDCP_VERBOSE_LEVEL == 4
#define idcp_dprintf5(...)
#define idcp_dprintf4(...) printf(__VA_ARGS__);IDCP_FFLUSH
#define idcp_dprintf3(...) printf(__VA_ARGS__);IDCP_FFLUSH
#define idcp_dprintf2(...) printf(__VA_ARGS__);IDCP_FFLUSH
#define idcp_dprintf1(...) printf(__VA_ARGS__);IDCP_FFLUSH
#elif IDCP_VERBOSE_LEVEL == 3
#define idcp_dprintf5(...)
#define idcp_dprintf4(...)
#define idcp_dprintf3(...) printf(__VA_ARGS__);IDCP_FFLUSH
#define idcp_dprintf2(...) printf(__VA_ARGS__);IDCP_FFLUSH
#define idcp_dprintf1(...) printf(__VA_ARGS__);IDCP_FFLUSH
#elif IDCP_VERBOSE_LEVEL == 2
#define idcp_dprintf5(...)
#define idcp_dprintf4(...)
#define idcp_dprintf3(...)
#define idcp_dprintf2(...) printf(__VA_ARGS__);IDCP_FFLUSH
#define idcp_dprintf1(...) printf(__VA_ARGS__);IDCP_FFLUSH
#elif IDCP_VERBOSE_LEVEL == 1
#define idcp_dprintf5(...)
#define idcp_dprintf4(...)
#define idcp_dprintf3(...)
#define idcp_dprintf2(...)
#define idcp_dprintf1(...) printf(__VA_ARGS__);IDCP_FFLUSH
#elif IDCP_VERBOSE_LEVEL == 0
#define idcp_dprintf5(...)
#define idcp_dprintf4(...)
#define idcp_dprintf3(...)
#define idcp_dprintf2(...)
#define idcp_dprintf1(...)
#else
#error invalid IDCP_VERBOSE_LEVEL value
#endif

#define IDCP_TOKENTYPE_IDENTIFIER 1
#define IDCP_TOKENTYPE_LPAREN 2
#define IDCP_TOKENTYPE_RPAREN 3
#define IDCP_TOKENTYPE_LBRACE 4
#define IDCP_TOKENTYPE_RBRACE 5
#define IDCP_TOKENTYPE_COMMA 6
#define IDCP_TOKENTYPE_SEMICOLON 7
#define IDCP_TOKENTYPE_PIPE 8
#define IDCP_TOKENTYPE_TILDE 9
#define IDCP_TOKENTYPE_AMP 10
#define IDCP_TOKENTYPE_STRING 11
#define IDCP_TOKENTYPE_NUMBER 12
#define IDCP_TOKENTYPE_HEXNUMBER 13
#define IDCP_TOKENTYPE_EQ 14
#define IDCP_TOKENTYPE_PLUS 15
static char *IDCP_TOKEN_NAME[] = {
	"<invalid>",
	"T_IDENT",
	"T_LPAREN",
	"T_RPAREN",
	"T_LBRACE",
	"T_RBRACE",
	"T_COMMA",
	"T_SEMICOLON",
	"T_PIPE",
	"T_TILDE",
	"T_AMP",
	"T_STRING",
	"T_NUMBER",
	"T_HEXNUMBER",
	"T_EQ",
	"T_PLUS"
};

struct idcp_token {
	int line;
	char type;
	union {
		struct {
			char *name;
			short name_len;
		} identifier;
		struct {
			char *value;
			short value_len;
		} string;
		struct {
			int value;
			char negative;
		} integer;
	} data;
};

struct idcp_function {
	char *name;
	int name_len;
	int num_parameters;
	char *parameters[IDCP_MAX_PARAMETERS];
	int parameters_len[IDCP_MAX_PARAMETERS];
	/**first token in idcparse.tokens that is part of the function*/
	int start_token_idx;
	/**last token in idcparse.tokens that is part of the function, exclusive*/
	int end_token_idx;
};

struct idcp_enum {
	char *name;
	int flags; /*denotes how numbers are represented, no details known about values.*/
	char is_bitfield;
	/**idx in idcp->enum_members where members of this enum start*/
	int start_idx;
	/**idx in idcp->enum_members where members of this enum end, exclusive*/
	int end_idx;
	char *comment, *rep_comment;
};

struct idcp_enum_member {
	char *name;
	int value;
	int bmask; /*do we need this?*/
	char *comment, *rep_comment;
};

struct idcp_struct {
	char *name;
	char is_union;
	/**Set by set_struc_align, which is undocumented.
	Is this alignment of members, or on which boundary this struct is aligned?*/
	int align;
	/**idx in idcp->struct_members where members of this struct start*/
	int start_idx;
	/**idx in idcp->struct_members where members of this struct end, exclusive*/
	int end_idx;
	char *comment, *rep_comment;
	/**idcp internal usage only*/
	char idcp_finished_adding_members;
};

struct idcp_struct_member {
	char *name;
	int offset;
	int flag;
	/**
	value depends on 'flag', see definitions in your IDA's idc.idc file:
	if is_enum0(flag) then enum id
	if is_struct0(flag) then struct id
	if is_stroff0(flag) then struct id
	if is_off0(flag) then offset base (?)
	if is_strlit(flag) then string type (?)
	*/
	int typeid;
	int nbytes;
	int target;
	int tdelta;
	int reftype;
	char *type;
	char *comment, *rep_comment;
};

#define IDCP_STUFF_TYPE_UNK 0
#define IDCP_STUFF_TYPE_DATA 1
#define IDCP_STUFF_TYPE_FUNC 2
/*instruction, can be a lot of things*/
#define IDCP_STUFF_TYPE_INSTR 3
/*instruction with set_name called afterwards, probably a function*/
#define IDCP_STUFF_TYPE_NAMED_INSTR 4

#define IDCP_DATA_STRLIT 1
#define IDCP_DATA_FLOAT 2
#define IDCP_DATA_DOUBLE 4

struct idcp_stuff {
	char type;
	int addr;
	union {
		struct {
			/*TODO: check if in rodata?*/
			/**If this is 0, it should be a struct var. That means
			'struct_type' should be set, resolve the struct and use
			its size instead.*/
			int size;
			/**0 means not an array*/
			short arraysize;
			/**see IDCP_DATA_xxx*/
			char flags;
			/**Generated idc files don't contain the type info of data!!
			Should this be resolved by putting type info in a comment?
			The only exception is when the type is a struct,
			then this field has the struct type of this piece of data.*/
			char *struct_type;
		} data;
		struct {
			char *name;
		} named_insn;
		struct {
			char *name;
			char *type;
		} func;
	} data;
	char *comment, *rep_comment;
};

struct idcparse {
	int num_tokens;
	struct idcp_token tokens[IDCP_MAX_TOKENS];
	int num_functions;
	struct idcp_function functions[IDCP_MAX_FUNCTIONS];
	int num_enum_members;
	struct idcp_enum_member enum_members[IDCP_MAX_TOTAL_ENUM_MEMBERS];
	int last_added_enum_member_enum_id;
	int num_enums;
	struct idcp_enum enums[IDCP_MAX_ENUMS];
	int num_struct_members;
	struct idcp_struct_member struct_members[IDCP_MAX_TOTAL_STRUCT_MEMBERS];
	int last_added_struct_member_struct_id;
	int num_structs;
	struct idcp_struct structs[IDCP_MAX_STRUCTS];
	int num_stuffs;
	struct idcp_stuff stuffs[IDCP_MAX_STUFFS];
	char *token_str_pool;
	char *token_str_pool_ptr;
	int num_lines;
};

/*Below definitions are only applicable during execution phase.*/

#define IDCP_VARIABLE_TYPE_VOID 0
#define IDCP_VARIABLE_TYPE_INT 1
#define IDCP_VARIABLE_TYPE_STRING 2
#define IDCP_VARIABLE_TYPE_STRUCT_MEMBER_ID 3

struct idcp_variable {
	char *name;
	int name_len;
	char type;
	union {
		int integer;
		char *string;
	} value;
};

/*shitty name*/
struct idcp_functioncallframe {
	char *function_name;
	int function_name_len;
	struct idcp_variable arguments[IDCP_MAX_ARGUMENTS];
	int num_arguments;
	struct idcp_variable variables[IDCP_MAX_LOCAL_VARIABLES];
	int num_variables;
	struct idcp_variable returnvalue;
};
/*jeanine:p:i:28;p:1;a:t;x:3.33;*/
static
struct idcp_token *idcp_get_next_token(struct idcparse *idcp)
{
	struct idcp_token *token;

	assert(((void)"hit IDCP_MAX_TOKENS limit", idcp->num_tokens < IDCP_MAX_TOKENS));
	token = idcp->tokens + idcp->num_tokens++;
	token->line = idcp->num_lines;
	return token;
}
/*jeanine:p:i:2;p:1;a:r;x:3.33;*/
static
struct idcp_function* idcp_create_function_from_lbrace_token(struct idcparse *idcp)
{
	struct idcp_function *f;
	struct idcp_token *tok;
	int i, type;

	tok = idcp->tokens + idcp->num_tokens - 1;
	assert(tok->type == IDCP_TOKENTYPE_LBRACE);
	tok--;
	assert(tok >= idcp->tokens);
	assert(tok->type == IDCP_TOKENTYPE_RPAREN);
	tok--;
	assert(tok >= idcp->tokens);
	/*go back until we find the name*/
	if (tok->type != IDCP_TOKENTYPE_LPAREN) {
		for (;;) {
			assert(tok->type == IDCP_TOKENTYPE_IDENTIFIER);
			tok--;
			assert(tok >= idcp->tokens);
			if (tok->type == IDCP_TOKENTYPE_LPAREN) {
				break;
			}
			assert(tok->type == IDCP_TOKENTYPE_COMMA);
		}
	}
	tok--;
	assert(tok >= idcp->tokens);
	assert(tok->type == IDCP_TOKENTYPE_IDENTIFIER);
	assert(((void)"hit IDCP_MAX_FUNCTIONS limit", idcp->num_functions < IDCP_MAX_FUNCTIONS));
	f = idcp->functions + idcp->num_functions++;
	f->name = tok->data.identifier.name;
	f->name_len = tok->data.identifier.name_len;
	f->start_token_idx = idcp->num_tokens;
	f->end_token_idx = idcp->num_tokens;
	f->num_parameters = 0;
	/*now go forwards again and note down the parameter names*/
	tok += 2;
	if (tok->type != IDCP_TOKENTYPE_RPAREN) {
		assert(tok->type == IDCP_TOKENTYPE_IDENTIFIER);
		if (!strcmp(tok->data.identifier.name, "void")) {
			assert((tok + 1)->type == IDCP_TOKENTYPE_RPAREN);
		} else {
			for (;;) {
				assert(tok->type == IDCP_TOKENTYPE_IDENTIFIER);
				assert(((void)"hit IDCP_MAX_PARAMETERS limit", f->num_parameters < IDCP_MAX_PARAMETERS));
				f->parameters[f->num_parameters] = tok->data.identifier.name;
				f->parameters_len[f->num_parameters] = tok->data.identifier.name_len;
				f->num_parameters++;
				tok++;
				if (tok->type == IDCP_TOKENTYPE_RPAREN) {
					break;
				}
				assert(tok->type == IDCP_TOKENTYPE_COMMA);
				tok++;
			}
		}
	}
	return f;
}
/*jeanine:p:i:3;p:4;a:r;x:50.15;y:-2.98;*/
static
void idcp_print_function_info(struct idcparse *idcp)
{
	struct idcp_function *f;
	int i;

	idcp_dprintf3("%d functions\n", idcp->num_functions);
	for (i = 0; i < idcp->num_functions; i++) {
		f = idcp->functions + i;
		idcp_dprintf4("  function %s start %d end %d\n", f->name, f->start_token_idx, f->end_token_idx);
	}
}
/*jeanine:p:i:1;p:4;a:r;x:208.82;y:-248.47;*/
#define IDCP_CHARTYPE_IDENTIFIER 1
#define IDCP_CHARTYPE_IDENTIFIER_NOTFIRST 2
#define IDCP_CHARTYPE_NUMBER 4
#define IDCP_CHARTYPE_HEXNUMBER 8
/**
Also populates idcp->functions/idcp->num_functions

Things that are not accounted for:
- assuming LF or CRLF line endings, CR alone won't work (only used for reporting errors though)
- lines ending with \
- many operators (|| && + - etc), because that stuff does not usually occur in database-dumped idc files
- ...a whole bunch more stuff probably
*/
static
void idcparse_tokenize(struct idcparse *idcp, char *chars, int length)
{
	unsigned char charmap[255];
	struct idcp_token *token;
	struct idcp_function *current_function;
	int i, charsleft, brace_depth;
	char c, c_type, *tmp_str_ptr;

	current_function = NULL;
	brace_depth = 0;
	charsleft = length;

	/*init charmap for parsing*/
	memset(charmap, 0, sizeof(charmap));
	for (i = 'a'; i <= 'z'; i++) {
		charmap[i] |= IDCP_CHARTYPE_IDENTIFIER | IDCP_CHARTYPE_IDENTIFIER_NOTFIRST;
	}
	for (i = 'A'; i <= 'Z'; i++) {
		charmap[i] |= IDCP_CHARTYPE_IDENTIFIER | IDCP_CHARTYPE_IDENTIFIER_NOTFIRST;
	}
	for (i = '0'; i <= '9'; i++) {
		charmap[i] |= IDCP_CHARTYPE_IDENTIFIER_NOTFIRST | IDCP_CHARTYPE_NUMBER;
	}
	for (i = 'a'; i <= 'f'; i++) {
		charmap[i] |= IDCP_CHARTYPE_HEXNUMBER;
	}
	for (i = 'A'; i <= 'F'; i++) {
		charmap[i] |= IDCP_CHARTYPE_HEXNUMBER;
	}
	charmap['_'] = IDCP_CHARTYPE_IDENTIFIER;
	charmap['@'] = IDCP_CHARTYPE_IDENTIFIER | IDCP_CHARTYPE_IDENTIFIER_NOTFIRST;

	idcp->num_lines = 1;
	/*char reading loop*/
	do {
		c = *chars;
		chars++; charsleft--;
havechar:
		c_type = charmap[c];

		if (c == ' ' || c == '\t' || c == '\r') {
			;
		} else if (c == '\n') {
			idcp->num_lines++;
		}
		/*macro*/
		else if (c == '#') {
			idcp_dprintf5("%d: found #...", idcp->num_lines);
			/*we ignore these for now, skip until next line*/
			while (charsleft && c != '\n') {
				c = *chars;
				chars++; charsleft--;
			}
			idcp_dprintf5(" skipped macro\n");
			idcp->num_lines++;
		}
		/*line comment*/
		else if (c == '/') {
			idcp_dprintf5("%d: found /...", idcp->num_lines);
			assert(((void)"single slash at EOF", charsleft));
			assert(((void)"single slash followed by non-slash", *chars == '/'));
			while (charsleft && c != '\n') {
				c = *chars;
				chars++; charsleft--;
			}
			idcp_dprintf5("skipped line comment\n");
			idcp->num_lines++;
		}
		/*identifier*/
		else if (c_type & IDCP_CHARTYPE_IDENTIFIER) {
			token = idcp_get_next_token(idcp);
			token->type = IDCP_TOKENTYPE_IDENTIFIER;
			token->data.identifier.name = tmp_str_ptr = idcp->token_str_pool_ptr;
			goto ident_first_char;
			while (charsleft) {
				c = *chars;
				chars++; charsleft--;
				if (!(charmap[c] & (IDCP_CHARTYPE_IDENTIFIER | IDCP_CHARTYPE_IDENTIFIER_NOTFIRST))) {
					break;
				}
ident_first_char:
				*tmp_str_ptr = c; tmp_str_ptr++;
			}
			*tmp_str_ptr = 0;
			idcp->token_str_pool_ptr = tmp_str_ptr + 1;
			token->data.identifier.name_len = tmp_str_ptr - token->data.identifier.name;
			idcp_dprintf5("%d: T_IDENTIFIER '%s'\n", idcp->num_lines, token->data.identifier.name);
			if (charsleft) {
				goto havechar;
			}
		}
		/*possibly negative number*/
		else if (c == '-' && charsleft && (charmap[*chars] & IDCP_CHARTYPE_NUMBER)) {
			token = idcp_get_next_token(idcp);
			token->data.integer.negative = 1;
			tmp_str_ptr = idcp->token_str_pool_ptr;
			*tmp_str_ptr = c; tmp_str_ptr++;
			c = *chars; c_type = charmap[c];
			chars++; charsleft--;
			goto parse_chartype_token;
		}
		/*number*/
		else if (c_type & IDCP_CHARTYPE_NUMBER) {
			token = idcp_get_next_token(idcp);
			token->data.integer.negative = 0;
			tmp_str_ptr = idcp->token_str_pool_ptr;
parse_chartype_token:
			*tmp_str_ptr = c; tmp_str_ptr++;
			if (charsleft && c == '0' && (*chars == 'x' || *chars == 'X')) {
				token->type = IDCP_TOKENTYPE_HEXNUMBER;
				i = IDCP_CHARTYPE_NUMBER | IDCP_CHARTYPE_HEXNUMBER;
				*tmp_str_ptr = *chars; tmp_str_ptr++;
				chars++; charsleft--;
			} else {
				token->type = IDCP_TOKENTYPE_NUMBER;
				i = IDCP_CHARTYPE_NUMBER;
			}
			while (charsleft) {
				c = *chars;
				chars++; charsleft--;
				if (!(charmap[c] & i)) {
					break;
				}
				*tmp_str_ptr = c; tmp_str_ptr++;
			}
			*tmp_str_ptr = 0;
			tmp_str_ptr = idcp->token_str_pool_ptr;
			if (*tmp_str_ptr == '-') {
				tmp_str_ptr++;
			}
			token->data.integer.value = 0;
			if (token->type == IDCP_TOKENTYPE_HEXNUMBER) {
				tmp_str_ptr += 2;
				while (*tmp_str_ptr) {
					token->data.integer.value <<= 4;
					if ('a' <= *tmp_str_ptr && *tmp_str_ptr <= 'f') {
						token->data.integer.value |= *tmp_str_ptr - 'a' + 10;
					} else if ('A' <= *tmp_str_ptr && *tmp_str_ptr <= 'F') {
						token->data.integer.value |= *tmp_str_ptr - 'A' + 10;
					} else {
						token->data.integer.value |= *tmp_str_ptr - '0';
					}
					tmp_str_ptr++;
				}
				if (token->data.integer.negative) {
					token->data.integer.value = -token->data.integer.value;
				}
				idcp_dprintf5("%d: T_HEXNUMBER '%s' %x\n",
					idcp->num_lines, idcp->token_str_pool_ptr, token->data.integer.value);
			} else {
				while (*tmp_str_ptr) {
					token->data.integer.value *= 10;
					token->data.integer.value += *tmp_str_ptr - '0';
					tmp_str_ptr++;
				}
				if (token->data.integer.negative) {
					token->data.integer.value = -token->data.integer.value;
				}
				idcp_dprintf5("%d: T_NUMBER '%s' %d\n",
					idcp->num_lines, idcp->token_str_pool_ptr, token->data.integer.value);
			}
			if (charsleft) {
				goto havechar;
			}
		}
		/*string*/
		else if (c == '"') {
			token = idcp_get_next_token(idcp);
			token->type = IDCP_TOKENTYPE_STRING;
			token->data.string.value = tmp_str_ptr = idcp->token_str_pool_ptr;
			for (;;) {
				assert(((void)"EOF while string is open", charsleft));
				c = *chars;
				chars++; charsleft--;
				if (c == '\\') {
					assert(((void)"EOF while escape char in string", charsleft));
					if (*chars == '"') {
						c = '"';
					} else if (*chars == 'n') {
						c = '\n';
					} else if (*chars != '\\') {
						printf("unexpected character '%c' after \\ in string\n", *chars);
						assert(0);
					}
					chars++; charsleft--;
				} else if (c == '"') {
					break;
				}
				*tmp_str_ptr = c; tmp_str_ptr++;
			}
			*tmp_str_ptr = 0;
			idcp->token_str_pool_ptr = tmp_str_ptr + 1;
			token->data.string.value_len = tmp_str_ptr - token->data.string.value;
			idcp_dprintf5("%d: T_STRING '%s'\n", idcp->num_lines, token->data.string.value);
		}
		else if (c == '(') {
			idcp_get_next_token(idcp)->type = IDCP_TOKENTYPE_LPAREN;
			idcp_dprintf5("%d: T_LPAREN\n", idcp->num_lines);
		} else if (c == ')') {
			idcp_get_next_token(idcp)->type = IDCP_TOKENTYPE_RPAREN;
			idcp_dprintf5("%d: T_RPAREN\n", idcp->num_lines);
		} else if (c == '{') {
			idcp_get_next_token(idcp)->type = IDCP_TOKENTYPE_LBRACE;
			idcp_dprintf5("%d: T_LBRACE\n", idcp->num_lines);
			if (!brace_depth) {
				current_function = idcp_create_function_from_lbrace_token(idcp);/*jeanine:r:i:2;*/
			}
			brace_depth++;
		} else if (c == '}') {
			idcp_get_next_token(idcp)->type = IDCP_TOKENTYPE_RBRACE;
			idcp_dprintf5("%d: T_RBRACE\n", idcp->num_lines);
			brace_depth--;
			if (!brace_depth && current_function) {
				current_function->end_token_idx = idcp->num_tokens - 1;
				current_function = NULL;
			}
		} else if (c == ',') {
			idcp_get_next_token(idcp)->type = IDCP_TOKENTYPE_COMMA;
			idcp_dprintf5("%d: T_COMMA\n", idcp->num_lines);
		} else if (c == ';') {
			idcp_get_next_token(idcp)->type = IDCP_TOKENTYPE_SEMICOLON;
			idcp_dprintf5("%d: T_SEMICOLON\n", idcp->num_lines);
		} else if (c == '=') {
			idcp_get_next_token(idcp)->type = IDCP_TOKENTYPE_EQ;
			idcp_dprintf5("%d: T_EQ\n", idcp->num_lines);
		} else if (c == '+') {
			idcp_get_next_token(idcp)->type = IDCP_TOKENTYPE_PLUS;
			idcp_dprintf5("%d: T_PLUS\n", idcp->num_lines);
		} else if (c == '|') {
			idcp_get_next_token(idcp)->type = IDCP_TOKENTYPE_PIPE;
			idcp_dprintf5("%d: T_PIPE\n", idcp->num_lines);
		} else if (c == '~') {
			idcp_get_next_token(idcp)->type = IDCP_TOKENTYPE_TILDE;
			idcp_dprintf5("%d: T_TILDE\n", idcp->num_lines);
		} else if (c == '&') {
			idcp_get_next_token(idcp)->type = IDCP_TOKENTYPE_AMP;
			idcp_dprintf5("%d: T_AMP\n", idcp->num_lines);
		} else {
			printf("%d: unexpected character '%c'\n", idcp->num_lines, c);
			assert(0);
		}
	} while (charsleft);
}
/*jeanine:p:i:30;p:35;a:r;x:53.56;*/
static
struct idcp_stuff* idcp_get_or_allocate_stuff(struct idcparse *idcp, int addr, int type)
{
	struct idcp_stuff *stuff;

	if (idcp->num_stuffs) {
		stuff = idcp->stuffs + idcp->num_stuffs - 1;
		if (stuff->addr == addr) {
			if (type != IDCP_STUFF_TYPE_UNK) {
				stuff->type = type;
			}
			goto ret;
		}
		/*Last entry in stuffs is not for the addr we want,
		so either delete and reuse the last entry if that entry is not of a
		type we care about, or assign a new entry if we do care about the
		last entry.*/
		switch (stuff->type) {
		case IDCP_STUFF_TYPE_UNK:
			goto reuse;
		case IDCP_STUFF_TYPE_INSTR:
		case IDCP_STUFF_TYPE_NAMED_INSTR:
		case IDCP_STUFF_TYPE_DATA:
			assert(addr > stuff->addr);
			break;
		default:
			assert(0);
		}
	}
	assert(idcp->num_stuffs < IDCP_MAX_STUFFS);
	stuff = idcp->stuffs + idcp->num_stuffs++;
reuse:
	memset(stuff, 0, sizeof(struct idcp_stuff));
	stuff->addr = addr;
	stuff->type = type;
ret:
	return stuff;
}
/*jeanine:p:i:40;p:39;a:r;x:81.22;*/
static
struct idcp_stuff* idcp_get_func(struct idcparse *idcp, int addr)
{
	static int last_idx;
	static int last_addr;
	static struct idcp_stuff *last_stuff;

	struct idcp_stuff *stuff;
	int idx, min, max, current;
	char *name;

	/*Binary search will work as long as the 'new_addr > addr'
	assertion in 'idcp_get_or_allocate_stuff' still stands.*/

	/*Since a few consecutive calls often happen on the same address,
	keeping this cache of the last result could speed up things.*/
	if (last_addr == addr) {
		return last_stuff;
	}
	/*Since everything should happen in order from low addr to hi addr,
	we can skip some things here and put 'min' to the value of the last idx
	instead of zero.*/
	min = last_idx;
	max = idcp->num_stuffs - 1;
	for (;;) {
		idx = min + (max - min) / 2;
		current = idcp->stuffs[idx].addr;
		if (current == addr) {
			stuff = idcp->stuffs + idx;
			if (stuff->type == IDCP_STUFF_TYPE_NAMED_INSTR ||
				stuff->type == IDCP_STUFF_TYPE_INSTR)
			{
				name = stuff->data.named_insn.name;
				stuff->type = IDCP_STUFF_TYPE_FUNC;
				stuff->data.func.name = name;
			} else {
				assert(stuff->type == IDCP_STUFF_TYPE_FUNC);
			}
			last_idx = idx;
			last_addr = addr;
			last_stuff = stuff;
			return stuff;
		} else {
			if (max == min) {
				assert(0);
			}
			if (current > addr) {
				max = idx - 1;
			} else if (current < addr) {
				min = idx + 1;
			}
		}
	}
}
/*jeanine:p:i:12;p:10;a:r;x:28.89;y:-171.00;*/
static
void idcp_func_add_enum(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	struct idcp_enum *e;
	int enum_id, flags;
	char *name;

	assert(frame->num_arguments == 3);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_STRING);
	assert(frame->arguments[2].type == IDCP_VARIABLE_TYPE_INT);
	enum_id = frame->arguments[0].value.integer;
	name    = frame->arguments[1].value.string;
	flags   = frame->arguments[2].value.integer;
	idcp_dprintf3("add_enum: allocating '%s' id %d flags 0x%x\n", name, idcp->num_enums, flags);

	assert(enum_id == -1); /*not gonna support 'insert in middle'*/
	assert(idcp->num_enums < IDCP_MAX_ENUMS);
	e = idcp->enums + idcp->num_enums++;
	e->name = name;
	e->flags = flags;
	e->start_idx = 0;
	e->end_idx = 0;
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_INT;
	frame->returnvalue.value.integer = idcp->num_enums - 1;
}
/*jeanine:p:i:14;p:10;a:r;x:28.89;y:107.00;*/
static
void idcp_func_add_enum_member(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	struct idcp_enum_member *m;
	int enum_id, value, bmask;
	char *name;

	assert(frame->num_arguments == 4);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_STRING);
	assert(frame->arguments[2].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[3].type == IDCP_VARIABLE_TYPE_INT);
	enum_id = frame->arguments[0].value.integer;
	name    = frame->arguments[1].value.string;
	value   = frame->arguments[2].value.integer;
	bmask   = frame->arguments[3].value.integer;

	assert(idcp->num_enum_members < IDCP_MAX_TOTAL_ENUM_MEMBERS);

	/*All enum members are added in one array and each enum holds indices where its members start and stop.
	So all members of a single enum must be consecutive entries in that array, and the easiest
	way to ensure that is to assert that members are only added to an enum with an equal or higher id
	as the enum id that the last member was added to.*/
	assert(enum_id >= idcp->last_added_enum_member_enum_id);
	if (enum_id > idcp->last_added_enum_member_enum_id) {
		idcp->enums[enum_id].start_idx = idcp->num_enum_members;
		idcp->enums[enum_id].end_idx = idcp->num_enum_members;
		idcp->last_added_enum_member_enum_id = enum_id;
	}

	idcp->enums[enum_id].end_idx++;
	m = idcp->enum_members + idcp->num_enum_members++;
	m->name = name;
	m->value = value;
	m->bmask = bmask;
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:15;p:10;a:r;x:28.89;y:-21.00;*/
static
void idcp_func_set_enum_cmt(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	int enum_id, repeatable;
	char *cmt;

	assert(frame->num_arguments == 3);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_STRING);
	assert(frame->arguments[2].type == IDCP_VARIABLE_TYPE_INT);
	enum_id    = frame->arguments[0].value.integer;
	cmt        = frame->arguments[1].value.string;
	repeatable = frame->arguments[2].value.integer;

	assert(0 <= enum_id && enum_id < idcp->num_enums);
	if (repeatable) {
		idcp->enums[enum_id].rep_comment = cmt;
	} else {
		idcp->enums[enum_id].comment = cmt;
	}
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:21;p:10;a:r;x:28.89;y:2.00;*/
static
void idcp_func_get_struc_id(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	struct idcp_struct *str, *end;
	char* name;

	assert(frame->num_arguments == 1);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_STRING);
	name = frame->arguments[0].value.string;

	str = idcp->structs;
	end = idcp->structs + idcp->num_structs;
	while (str < end) {
		if (!strcmp(str->name, name)) {
			frame->returnvalue.type = IDCP_VARIABLE_TYPE_INT;
			frame->returnvalue.value.integer = str - idcp->structs;
			return;
		}
		str++;
	}
	printf("get_struc_id: can't find struct for name '%s'\n", name);
	assert(0);
}
/*jeanine:p:i:16;p:10;a:r;x:28.89;y:144.00;*/
static
void idcp_func_get_enum_member(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	int enum_id, value, serial, idx, end;
	struct idcp_enum_member *mbr;

	assert(frame->num_arguments == 4);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[2].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[3].type == IDCP_VARIABLE_TYPE_INT);
	enum_id = frame->arguments[0].value.integer;
	value   = frame->arguments[1].value.integer;
	serial  = frame->arguments[2].value.integer;
	/*bmask  = frame->arguments[3].value.integer;*/

	assert(0 <= enum_id && enum_id < idcp->num_enums);
	idx = idcp->enums[enum_id].start_idx;
	end = idcp->enums[enum_id].end_idx;
	mbr = idcp->enum_members + idx;
	while (idx < end) {
		if (mbr->value == value) {
			if (!serial) {
				frame->returnvalue.type = IDCP_VARIABLE_TYPE_INT;
				frame->returnvalue.value.integer = idx;
				return;
			}
			serial--;
		}
		mbr++;
		idx++;
	}
	printf("get_enum_member: can't find member for value 0x%x\n", value);
	assert(0);
}
/*jeanine:p:i:17;p:10;a:r;x:28.89;y:245.00;*/
static
void idcp_func_set_enum_member_cmt(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	int member_id, repeatable;
	char *cmt;

	assert(frame->num_arguments == 3);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_STRING);
	assert(frame->arguments[2].type == IDCP_VARIABLE_TYPE_INT);
	member_id  = frame->arguments[0].value.integer;
	cmt        = frame->arguments[1].value.string;
	repeatable = frame->arguments[2].value.integer;

	assert(0 <= member_id && member_id < idcp->num_enum_members);
	if (repeatable) {
		idcp->enum_members[member_id].rep_comment = cmt;
	} else {
		idcp->enum_members[member_id].comment = cmt;
	}
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:18;p:10;a:r;x:28.89;y:-59.00;*/
static
void idcp_func_set_enum_bf(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	int enum_id, is_bitfield;

	assert(frame->num_arguments == 2);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_INT);
	enum_id     = frame->arguments[0].value.integer;
	is_bitfield = frame->arguments[1].value.integer;

	assert(0 <= enum_id && enum_id < idcp->num_enums);
	idcp->enums[enum_id].is_bitfield = is_bitfield;
}
/*jeanine:p:i:19;p:10;a:r;x:28.89;y:-128.00;*/
static
void idcp_func_add_struc(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	struct idcp_struct *s;
	int index, is_union;
	char *name;

	assert(frame->num_arguments == 3);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_STRING);
	assert(frame->arguments[2].type == IDCP_VARIABLE_TYPE_INT);
	index    = frame->arguments[0].value.integer;
	name     = frame->arguments[1].value.string;
	is_union = frame->arguments[2].value.integer;
	idcp_dprintf3("add_struc: allocating '%s' id %d is_union %d\n", name, idcp->num_structs, is_union);

	assert(idcp->num_structs < IDCP_MAX_STRUCTS);
	s = idcp->structs + idcp->num_structs++;
	s->name = name;
	s->is_union = is_union;
	s->start_idx = 0;
	s->end_idx = 0;
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_INT;
	frame->returnvalue.value.integer = idcp->num_structs - 1;
}
/*jeanine:p:i:20;p:10;a:r;x:28.89;y:44.00;*/
static
void idcp_func_set_struc_cmt(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	int struct_id, repeatable;
	char *cmt;

	assert(frame->num_arguments == 3);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_STRING);
	assert(frame->arguments[2].type == IDCP_VARIABLE_TYPE_INT);
	struct_id  = frame->arguments[0].value.integer;
	cmt        = frame->arguments[1].value.string;
	repeatable = frame->arguments[2].value.integer;

	assert(0 <= struct_id && struct_id < idcp->num_structs);
	if (repeatable) {
		idcp->structs[struct_id].rep_comment = cmt;
	} else {
		idcp->structs[struct_id].comment = cmt;
	}
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:22;p:10;a:r;x:28.89;y:187.00;*/
static
void idcp_func_add_struc_member(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	struct idcp_struct_member *m;
	struct idcp_struct *s;
	int struct_id, offset, flag, typeid, nbytes, target, tdelta, reftype;
	char *name;

	assert(frame->num_arguments == 6 || frame->num_arguments == 9);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_STRING);
	assert(frame->arguments[2].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[3].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[4].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[5].type == IDCP_VARIABLE_TYPE_INT);
	struct_id = frame->arguments[0].value.integer;
	name      = frame->arguments[1].value.string;
	offset    = frame->arguments[2].value.integer;
	flag      = frame->arguments[3].value.integer;
	typeid    = frame->arguments[4].value.integer;
	nbytes    = frame->arguments[5].value.integer;
	if (frame->num_arguments == 9) {
		assert(frame->arguments[6].type == IDCP_VARIABLE_TYPE_INT);
		assert(frame->arguments[7].type == IDCP_VARIABLE_TYPE_INT);
		assert(frame->arguments[8].type == IDCP_VARIABLE_TYPE_INT);
		target  = frame->arguments[6].value.integer;
		tdelta  = frame->arguments[7].value.integer;
		reftype = frame->arguments[8].value.integer;
	} else {
		target = -1;
		tdelta = 0;
		reftype = 0;
	}

	assert(idcp->num_struct_members < IDCP_MAX_TOTAL_STRUCT_MEMBERS);
	assert(0 <= struct_id && struct_id < idcp->num_structs);
	s = idcp->structs + struct_id;

	/*All struct members are added in one array and each struct holds indices where its members start and stop.
	So all members of a single struct must be consecutive entries in that array. It seems like structs are
	filled in an non-consecutive order, but a struct is filled completely before going to the next struct.*/
	if (struct_id != idcp->last_added_struct_member_struct_id) {
		/*If this fails, it means that struct A's members were added, then some other structs's
		member(s) were added, and now it's trying to add more members to struct A. That would suck,
		because of reasons in the comment above.*/
		assert(!s->idcp_finished_adding_members);
		/*Since we're adding members to a different struct, mark the last one as finished.*/
		if (idcp->last_added_struct_member_struct_id != -1) {
			idcp->structs[idcp->last_added_struct_member_struct_id].idcp_finished_adding_members = 1;
		}
		idcp->last_added_struct_member_struct_id = struct_id;
		s->start_idx = idcp->num_struct_members;
		s->end_idx = idcp->num_struct_members;
	}

	/*Ensure that members are added in order, because otherwise we'd need to sort and I don't want to*/
	if (s->start_idx != s->end_idx) {
		if (s->is_union) {
			assert(offset == idcp->struct_members[s->end_idx - 1].offset);
		} else {
			assert(offset > idcp->struct_members[s->end_idx - 1].offset);
		}
	}
	s->end_idx++;
	m = idcp->struct_members + idcp->num_struct_members++;
	m->name = name;
	m->offset = offset;
	m->flag = flag;
	m->typeid = typeid;
	m->nbytes = nbytes;
	m->target = target;
	m->tdelta = tdelta;
	m->reftype = reftype;
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_STRUCT_MEMBER_ID;
	frame->returnvalue.value.integer = 0; /*ok*/
}
/*jeanine:p:i:23;p:10;a:r;x:28.89;y:180.00;*/
static
void idcp_func_set_struc_align(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	int struct_id, align;

	assert(frame->num_arguments == 2);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_INT);
	struct_id = frame->arguments[0].value.integer;
	align     = frame->arguments[1].value.integer;

	assert(0 <= struct_id && struct_id < idcp->num_structs);
	idcp->structs[struct_id].align = align;
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:24;p:10;a:r;x:28.89;y:89.00;*/
static
void idcp_func_set_member_cmt(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	int struct_id, member_offset, repeatable;
	char *comment;

	assert(frame->num_arguments == 4);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[2].type == IDCP_VARIABLE_TYPE_STRING);
	assert(frame->arguments[3].type == IDCP_VARIABLE_TYPE_INT);
	struct_id     = frame->arguments[0].value.integer;
	member_offset = frame->arguments[1].value.integer;
	comment       = frame->arguments[2].value.string;
	repeatable    = frame->arguments[3].value.integer;

	assert(0 <= struct_id && struct_id < idcp->num_structs);
	if (repeatable) {
		idcp->structs[struct_id].rep_comment = comment;
	} else {
		idcp->structs[struct_id].comment = comment;
	}
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:25;p:10;a:r;x:28.89;y:-144.00;*/
static
void idcp_func_get_enum(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	struct idcp_enum *enu, *end;
	char* name;

	assert(frame->num_arguments == 1);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_STRING);
	name = frame->arguments[0].value.string;

	enu = idcp->enums;
	end = idcp->enums + idcp->num_enums;
	while (enu < end) {
		if (!strcmp(enu->name, name)) {
			frame->returnvalue.type = IDCP_VARIABLE_TYPE_INT;
			frame->returnvalue.value.integer = enu - idcp->enums;
			return;
		}
		enu++;
	}
	printf("get_enum: can't find enum for name '%s'\n", name);
	assert(0);
}
/*jeanine:p:i:26;p:10;a:r;x:28.89;y:67.00;*/
static
void idcp_func_get_member_id(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	struct idcp_struct_member *mem, *end;
	int struct_id, member_offset;

	assert(frame->num_arguments == 2);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_INT);
	struct_id =     frame->arguments[0].value.integer;
	member_offset = frame->arguments[1].value.integer;

	assert(0 <= struct_id && struct_id < idcp->num_structs);
	mem = idcp->struct_members + idcp->structs[struct_id].start_idx;
	end = idcp->struct_members + idcp->structs[struct_id].end_idx;
	while (mem < end) {
		/*member_offset can be any offset within the member's size*/
		if (mem->offset <= member_offset && member_offset < mem->offset + mem->nbytes) {
			frame->returnvalue.type = IDCP_VARIABLE_TYPE_STRUCT_MEMBER_ID;
			frame->returnvalue.value.integer = mem - idcp->struct_members;
			return;
		}
		mem++;
	}
	printf("get_member_id: can't find member in struct '%s' at offset 0x%x\n", idcp->structs[struct_id].name, member_offset);
	assert(0);
}
/*jeanine:p:i:27;p:10;a:r;x:28.89;y:-249.00;*/
static
void idcp_func_SetType(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	/*This could be either setting the struct member type or a function type.*/
	struct idcp_stuff *stuff;
	int struct_member_id, addr;
	char *type;

	assert(frame->num_arguments == 2);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_STRING);
	type = frame->arguments[1].value.string;

	if (frame->arguments[0].type == IDCP_VARIABLE_TYPE_STRUCT_MEMBER_ID) {
		struct_member_id = frame->arguments[0].value.integer;

		assert(0 <= struct_member_id && struct_member_id <= idcp->num_struct_members);
		idcp->struct_members[struct_member_id].type = type;
	} else if (frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT) {
		addr = frame->arguments[0].value.integer;

		stuff = idcp_get_func(idcp, addr);/*jeanine:s:a:r;i:40;*/
		stuff->data.func.type = type;
	} else {
		assert(0);
	}
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:33;p:10;a:r;x:28.89;y:-65.00;*/
static
void idcp_func_create_insn(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	int addr;

	assert(frame->num_arguments == 1);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	addr = frame->arguments[0].value.integer;

	/*A 'set_cmt' can always precede a 'create_xxx'.*/
	idcp_get_or_allocate_stuff(idcp, addr, IDCP_STUFF_TYPE_INSTR);/*jeanine:s:a:r;i:30;*/
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:32;p:10;a:r;x:28.89;y:-109.00;*/
static
void idcp_func_make_array(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	struct idcp_stuff *stuff;
	int addr, size;

	assert(frame->num_arguments == 2);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_INT);
	addr = frame->arguments[0].value.integer;
	size = frame->arguments[1].value.integer;

	/*A 'make_array' _should_ always be preceded by a 'create_xxx' instruction,
	but there weirdly sometimes isn't the case and a 'make_array' is executed
	on an address that is simply an 'align' pseudo instruction... */
	if (idcp->num_stuffs) {
		stuff = idcp->stuffs + idcp->num_stuffs - 1;
		if (stuff->addr == addr &&
			/*It could be a TYPE_UNK if it's an 'align' and also
			has a 'set_cmt' on this addr...*/
			stuff->type == IDCP_STUFF_TYPE_DATA)
		{
			stuff->data.data.arraysize = size;
		}
	}
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:34;p:10;a:r;x:28.89;y:-225.00;*/
static
void idcp_func_set_name(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	struct idcp_stuff *stuff;
	int addr, flags;
	char *name;

	assert(frame->num_arguments == 2 || frame->num_arguments == 3);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_STRING);
	addr = frame->arguments[0].value.integer;
	name = frame->arguments[1].value.string;
	if (frame->num_arguments == 3) {
		assert(frame->arguments[2].type == IDCP_VARIABLE_TYPE_INT);
		flags = frame->arguments[2].value.integer;

		if (flags & IDCP_IDA_SN_LOCAL) {
			/*Don't need local names.*/
			/*Note that not all local names have the SN_LOCAL flag.*/
			return;
		}
	}

	/*A 'set_name' _should_ be done after a 'create_insn'. If this is not
	the case, it is probably just a local name and can be ignored.*/
	if (idcp->num_stuffs) {
		stuff = idcp->stuffs + idcp->num_stuffs - 1;
		if (stuff->addr == addr &&
			/*It could be a TYPE_UNK if it's a local name and also
			has a 'set_cmt' on this addr...*/
			stuff->type == IDCP_STUFF_TYPE_INSTR)
		{
			stuff->type = IDCP_STUFF_TYPE_NAMED_INSTR;
			stuff->data.named_insn.name = name;
		}
	}
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:35;p:10;a:r;x:28.89;y:-50.00;*/
static
void idcp_func_create_dword_word_byte(struct idcparse *idcp, struct idcp_functioncallframe *frame, int size, int flags)
{
	struct idcp_stuff *stuff;
	int addr;

	assert(frame->num_arguments == 1);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	addr = frame->arguments[0].value.integer;

	/*A 'set_cmt' can always precede a 'create_xxx'.*/
	stuff = idcp_get_or_allocate_stuff(idcp, addr, IDCP_STUFF_TYPE_DATA);/*jeanine:r:i:30;*/
	stuff->data.data.size = size;
	stuff->data.data.flags = flags;
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:38;p:10;a:r;x:28.89;y:-81.00;*/
static
void idcp_func_MakeStruct(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	struct idcp_stuff *stuff;
	int addr;
	char *struct_name;

	assert(frame->num_arguments == 2);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_STRING);
	addr        = frame->arguments[0].value.integer;
	struct_name = frame->arguments[1].value.string;

	/*A 'set_cmt' can always precede a 'create_xxx'.*/
	stuff = idcp_get_or_allocate_stuff(idcp, addr, IDCP_STUFF_TYPE_DATA);/*jeanine:s:a:r;i:30;*/
	stuff->data.data.size = 0;
	stuff->data.data.struct_type = struct_name;
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:36;p:10;a:r;x:28.89;y:-272.00;*/
static
void idcp_func_set_cmt(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	struct idcp_stuff *stuff;
	int addr, rptble;
	char *comment;

	assert(frame->num_arguments == 3);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_STRING);
	assert(frame->arguments[2].type == IDCP_VARIABLE_TYPE_INT);
	addr    = frame->arguments[0].value.integer;
	comment = frame->arguments[1].value.string;
	rptble  = frame->arguments[2].value.integer;

	/*A 'set_cmt' can always precede a 'create_xxx'.*/
	stuff = idcp_get_or_allocate_stuff(idcp, addr, IDCP_STUFF_TYPE_UNK);/*jeanine:s:a:r;i:30;*/
	if (rptble) {
		stuff->rep_comment = comment;
	} else {
		stuff->comment = comment;
	}
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:37;p:10;a:r;x:28.89;y:19.00;*/
static
void idcp_func_create_strlit(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	struct idcp_stuff *stuff;
	int addr, endaddr;

	assert(frame->num_arguments == 2);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_INT);
	addr    = frame->arguments[0].value.integer;
	endaddr = frame->arguments[1].value.integer;

	if (idcp->num_stuffs) {
		stuff = idcp->stuffs + idcp->num_stuffs - 1;
		/*Sometimes there's a 'create_strlit' on irrelevant data...*/
		if (stuff->addr == addr) {
			if (stuff->type == IDCP_STUFF_TYPE_UNK) {
				stuff->type = IDCP_STUFF_TYPE_DATA;
				stuff->data.data.size = 1;
			} else {
				assert(stuff->type == IDCP_STUFF_TYPE_DATA);
				assert(stuff->data.data.size == 1);
			}
			stuff->data.data.flags |= IDCP_DATA_STRLIT;
		}
	}
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:39;p:10;a:r;x:28.89;y:-186.00;*/
static
void idcp_func_add_func(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	int addr, endaddr;

	assert(frame->num_arguments == 2);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_INT);
	addr    = frame->arguments[0].value.integer;
	endaddr = frame->arguments[1].value.integer;

	assert(idcp->num_stuffs);
	idcp_get_func(idcp, addr);/*jeanine:r:i:40;*/
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:41;p:10;a:r;x:28.89;y:-45.00;*/
static
void idcp_func_set_func_cmt(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	struct idcp_stuff *stuff;
	int addr, rptble;
	char *comment;

	assert(frame->num_arguments == 3);
	assert(frame->arguments[0].type == IDCP_VARIABLE_TYPE_INT);
	assert(frame->arguments[1].type == IDCP_VARIABLE_TYPE_STRING);
	assert(frame->arguments[2].type == IDCP_VARIABLE_TYPE_INT);
	addr    = frame->arguments[0].value.integer;
	comment = frame->arguments[1].value.string;
	rptble  = frame->arguments[2].value.integer;

	assert(idcp->num_stuffs);
	stuff = idcp_get_func(idcp, addr);/*jeanine:s:a:r;i:40;*/
	if (rptble) {
		stuff->rep_comment = comment;
	} else {
		stuff->comment = comment;
	}
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:10;p:11;a:r;x:347.66;*/
/*see https://hex-rays.com/products/ida/support/idadoc/162.shtml for a list of built-in idc functions,
or simply check idahelp.chm included in your IDA installation (press F1).*/
static
int idcp_execute_internal_function(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	register char *name;

	name = frame->function_name;
	switch (frame->function_name_len) {
	case 6:
		if (!strcmp("op_hex", name) || !strcmp("op_dec", name) ||
			!strcmp("op_chr", name) || /*rare*/!strcmp("op_bin", name) ||
			/*rare*/!strcmp("op_seg", name))
		{
			goto ignoredbuiltin; /*high prio nops*/
		}
		break;
	case 7:
		if (!strcmp("GetEnum", name)) {
			idcp_func_get_enum(idcp, frame);/*jeanine:s:a:r;i:25;*/
			return 1;
		} else if (!strcmp("set_cmt", name)) {
			idcp_func_set_cmt(idcp, frame);/*jeanine:r:i:36;*/
			return 1;
		} else if (!strcmp("op_enum", name)) {
			goto ignoredbuiltin; /*high prio nops*/
		} else if (!strcmp("SetType", name)) {
			idcp_func_SetType(idcp, frame);/*jeanine:r:i:27;*/
			return 1;
		}
		break;
	case 8:
		if (!strcmp("set_name", name)) {
			idcp_func_set_name(idcp, frame);/*jeanine:r:i:34;*/
			return 1;
		} else if (!strcmp("add_func", name)) {
			idcp_func_add_func(idcp, frame);/*jeanine:r:i:39;*/
			return 1;
		} else if (!strcmp("add_enum", name)) {
			idcp_func_add_enum(idcp, frame);/*jeanine:r:i:12;*/
			return 1;
		} else if (!strcmp("get_enum", name)) {
			idcp_func_get_enum(idcp, frame);/*jeanine:r:i:25;*/
			return 1;
		} else if (!strcmp("SegClass", name) || !strcmp("set_flag", name)) {
			goto ignoredbuiltin; /*low prio nops*/
		}
		break;
	case 9:
		if (!strcmp("op_stroff", name)) {
			goto ignoredbuiltin; /*high prio nops*/
		} else if (!strcmp("add_struc", name)) {
			idcp_func_add_struc(idcp, frame);/*jeanine:r:i:19;*/
			return 1;
		} else if (!strcmp("op_stkvar", name) || !strcmp("SegRename", name) ||
			!strcmp("SegDefReg", name))
		{
			goto ignoredbuiltin; /*low prio nops*/
		}
		break;
	case 10:
		if (!strcmp("make_array", name)) {
			idcp_func_make_array(idcp, frame);/*jeanine:r:i:32;*/
			return 1;
		} else if (!strcmp("MakeStruct", name)) {
			idcp_func_MakeStruct(idcp, frame);/*jeanine:r:i:38;*/
			return 1;
		}
		break;
	case 11:
		if (!strcmp("create_insn", name)) {
			idcp_func_create_insn(idcp, frame);/*jeanine:r:i:33;*/
			return 1;
		} else if (!strcmp("create_byte", name)) {
			idcp_func_create_dword_word_byte(idcp, frame, 1, 0);/*jeanine:s:a:r;i:35;*/
			return 1;
		} else if (!strcmp("create_word", name)) {
			idcp_func_create_dword_word_byte(idcp, frame, 2, 0);/*jeanine:s:a:r;i:35;*/
			return 1;
		} else if (!strcmp("toggle_sign", name) || !strcmp("toggle_bnot", name)) {
			goto ignoredbuiltin; /*high prio nops*/
		} else if (!strcmp("set_enum_bf", name)) {
			idcp_func_set_enum_bf(idcp, frame);/*jeanine:r:i:18;*/
			return 1;
		} else if (!strcmp("add_segm_ex", name)) {
			goto ignoredbuiltin; /*low prio nops*/
		}
		break;
	case 12:
		if (!strcmp("create_dword", name)) {
			idcp_func_create_dword_word_byte(idcp, frame, 4, 0);/*jeanine:r:i:35;*/
			return 1;
		} else if (!strcmp("create_float", name)) {
			idcp_func_create_dword_word_byte(idcp, frame, 4, IDCP_DATA_FLOAT);/*jeanine:s:a:r;i:35;*/
			return 1;
		} else if (!strcmp("create_qword", name)) {
			idcp_func_create_dword_word_byte(idcp, frame, 8, 0);/*jeanine:s:a:r;i:35;*/
			return 1;
		} else if (!strcmp("create_oword", name)) {
			idcp_func_create_dword_word_byte(idcp, frame, 16, 0);/*jeanine:s:a:r;i:35;*/
			return 1;
		} else if (!strcmp("create_tbyte", name)) {
			idcp_func_create_dword_word_byte(idcp, frame, 10, 0);/*jeanine:s:a:r;i:35;*/
			return 1;
		} else if (!strcmp("set_func_cmt", name)) {
			idcp_func_set_func_cmt(idcp, frame);/*jeanine:r:i:41;*/
			return 1;
		} else if (!strcmp("set_enum_cmt", name)) {
			idcp_func_set_enum_cmt(idcp, frame);/*jeanine:r:i:15;*/
			return 1;
		} else if (!strcmp("get_struc_id", name)) {
			idcp_func_get_struc_id(idcp, frame);/*jeanine:r:i:21;*/
			return 1;
		} else if (!strcmp("set_selector", name) || !strcmp("get_inf_attr", name) ||
			!strcmp("set_inf_attr", name))
		{
			goto ignoredbuiltin; /*low prio nops*/
		}
		break;
	case 13:
		if (!strcmp("create_strlit", name)) {
			idcp_func_create_strlit(idcp, frame);/*jeanine:r:i:37;*/
			return 1;
		} else if (!strcmp("create_double", name)) {
			idcp_func_create_dword_word_byte(idcp, frame, 8, IDCP_DATA_DOUBLE);/*jeanine:s:a:r;i:35;*/
			return 1;
		} else if (!strcmp("set_struc_cmt", name)) {
			idcp_func_set_struc_cmt(idcp, frame);/*jeanine:r:i:20;*/
			return 1;
		} else if (!strcmp("get_member_id", name)) {
			idcp_func_get_member_id(idcp, frame);/*jeanine:r:i:26;*/
			return 1;
		} else if (!strcmp("set_segm_type", name)) {
			goto ignoredbuiltin; /*low prio nops*/
		}
		break;
	case 14:
		if (!strcmp("set_member_cmt", name)) {
			idcp_func_set_member_cmt(idcp, frame);/*jeanine:r:i:24;*/
			return 1;
		} else if (!strcmp("set_func_flags", name) || !strcmp("set_frame_size", name)) {
			goto ignoredbuiltin; /*high prio nops*/
		}
		break;
	case 15:
		if (!strcmp("op_plain_offset", name)) {
			goto ignoredbuiltin; /*high prio nops*/
		} else if (!strcmp("add_enum_member", name)) {
			idcp_func_add_enum_member(idcp, frame);/*jeanine:r:i:14;*/
			return 1;
		} else if (!strcmp("get_enum_member", name)) {
			idcp_func_get_enum_member(idcp, frame);/*jeanine:r:i:16;*/
			return 1;
		} else if (!strcmp("set_struc_align", name)) {
			idcp_func_set_struc_align(idcp, frame);/*jeanine:r:i:23;*/
			return 1;
		} else if (!strcmp("add_default_til", name)) {
			goto ignoredbuiltin; /*low prio nops*/
		}
		break;
	case 16:
		if (!strcmp("GetStrucIdByName", name)) {
			idcp_func_get_struc_id(idcp, frame);/*jeanine:s:a:r;i:21;*/
			return 1;
		} else if (!strcmp("add_struc_member", name)) {
			idcp_func_add_struc_member(idcp, frame);/*jeanine:r:i:22;*/
			return 1;
		} else if (!strcmp("define_local_var", name)) {
			goto ignoredbuiltin; /*high prio nops*/
		} else if (!strcmp("split_sreg_range", name) ||
			/*anterior/posterior comment lines*/ !strcmp("update_extra_cmt", name))
		{
			goto ignoredbuiltin; /*low prio nops*/
		}
		break;
	case 17:
		if (!strcmp("end_type_updating", name)) {
			goto ignoredbuiltin;
		}
		break;
	case 18:
		if (!strcmp("set_processor_type", name)) {
			goto ignoredbuiltin;
		}
		break;
	case 19:
		if (!strcmp("set_enum_member_cmt", name)) {
			idcp_func_set_enum_member_cmt(idcp, frame);/*jeanine:r:i:17;*/
			return 1;
		} else if (!strcmp("delete_all_segments", name) || !strcmp("begin_type_updating", name)) {
			goto ignoredbuiltin; /*low prio nops*/
		}
		break;
	}
	return 0;
ignoredbuiltin:
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_INT;
	frame->returnvalue.value.integer = 0;
	return 1;
}
/*jeanine:p:i:9;p:8;a:r;x:4.29;y:-29.27;*/
/**
@param tok should be the identifier doing the variable access
*/
static
void idcp_get_variable_value(struct idcparse *idcp, struct idcp_variable *result, struct idcp_token *tok, struct idcp_functioncallframe *frame)
{
	char *name;
	int i, name_len;

	name = tok->data.identifier.name;
	name_len = tok->data.identifier.name_len;
	idcp_dprintf4("access variable %s\n", name);
	for (i = 0; i < frame->num_variables; i++) {
		if (!strcmp(name, frame->variables[i].name)) {
			*result = frame->variables[i];
			assert(result->type != IDCP_VARIABLE_TYPE_VOID);
			return;
		}
	}
	/*bunchof global definitions*/
	switch (name_len) {
	case 6:
		if (!strcmp(name, "E_PREV") || !strcmp(name, "E_NEXT")) {
			goto ignoredvariable;
		}
		break;
	case 8:
		if (!strcmp(name, "SN_LOCAL")) {
			result->type = IDCP_VARIABLE_TYPE_INT;
			result->value.integer = IDCP_IDA_SN_LOCAL;
			return;
		} else if (!strcmp(name, "UTP_ENUM")) {
			goto ignoredvariable;
		}
		break;
	case 9:
		if (!strcmp(name, "SW_ALLCMT")) {
			goto ignoredvariable;
		}
		break;
	case 10:
		if (!strcmp(name, "INF_MAXREF") || !strcmp(name, "INF_INDENT") ||
			!strcmp(name, "UTP_STRUCT"))
		{
			goto ignoredvariable;
		}
		break;
	case 11:
		if (!strcmp(name, "INF_LOW_OFF") || !strcmp(name, "INF_CMTFLAG") ||
			!strcmp(name, "INF_XREFNUM") || !strcmp(name, "INF_COMMENT"))
		{
			goto ignoredvariable;
		}
		break;
	case 12:
		if (!strcmp(name, "INF_HIGH_OFF") || !strcmp(name, "INF_GENFLAGS") ||
			!strcmp(name, "INF_COMPILER") || !strcmp(name, "INF_OUTFLAGS") ||
			!strcmp(name, "SETPROC_USER"))
		{
			goto ignoredvariable;
		}
		break;
	case 13:
		if (!strcmp(name, "ADDSEG_NOSREG") || !strcmp(name, "INFFL_LOADIDC")) {
			goto ignoredvariable;
		}
		break;
	case 14:
		if (!strcmp(name, "OFLG_SHOW_VOID") || !strcmp(name, "OFLG_SHOW_AUTO")) {
			goto ignoredvariable;
		}
		break;
	case 16:
		if (!strcmp(name, "INF_STRLIT_BREAK")) {
			goto ignoredvariable;
		}
		break;
	}
	printf("%d: failed to find variable '%s' for reading\n", tok->line, name);
	assert(0);
ignoredvariable:
	result->type = IDCP_VARIABLE_TYPE_INT;
	result->value.integer = 0;
}
/*jeanine:p:i:13;p:8;a:r;x:4.44;y:-48.00;*/
/**
Will set the local variable (found in param 'frame') with the same name as provided in param 'value' to the value in param 'value'.

@param tok only used to report line number in case of error
*/
static
void idcp_set_variable_value(struct idcparse *idcp, struct idcp_variable *value, struct idcp_token *tok, struct idcp_functioncallframe *frame, char *name)
{
	int i;

	idcp_dprintf4("write variable %s\n", name);
	for (i = 0; i < frame->num_variables; i++) {
		if (!strcmp(name, frame->variables[i].name)) {
			frame->variables[i].type = value->type;
			frame->variables[i].value = value->value;
			return;
		}
	}
	printf("%d: failed to find variable '%s' for writing\n", tok->line, name);
	assert(0);
}
/*jeanine:p:i:8;p:11;a:r;x:3.33;*/
static void idcp_execute_function(struct idcparse*, struct idcp_functioncallframe*);
/**
Evaluates whatever tokens it finds until it finds a semicolon or comma or rparen.
Order of operations is always left to right
*/
static
void idcp_eval_expression_value(struct idcparse *idcp, struct idcp_variable *result, struct idcp_functioncallframe *frame, struct idcp_token **_tok, int *_tokensleft)
#define IDCP_EVAL_OP_NONE 0
#define IDCP_EVAL_OP_PLUS 1
#define IDCP_EVAL_OP_OR 2
#define IDCP_EVAL_OP_AND 3
#define IDCP_EVAL_OP_NOT 4
{
	struct idcp_functioncallframe childframe;
	struct idcp_variable tmp_var, *arg;
	struct idcp_token *tok;
	int tokensleft, current_operation, have_value;
	char *tmp_name;

	idcp_dprintf4("start eval\n");
	have_value = 0;
	current_operation = IDCP_EVAL_OP_NONE;
	result->type = IDCP_VARIABLE_TYPE_VOID;
	tok = *_tok;
	tokensleft = *_tokensleft;
	while (tokensleft) {
		switch (tok->type) {
		case IDCP_TOKENTYPE_IDENTIFIER:
			idcp_dprintf4("  identifier %s\n", tok->data.identifier.name);
			assert(!have_value || current_operation != IDCP_EVAL_OP_NONE);
			if ((tok + 1)->type == IDCP_TOKENTYPE_LPAREN) {
				idcp_dprintf4("    has lparen, is function\n");
				childframe.function_name = tok->data.identifier.name;
				childframe.function_name_len = tok->data.identifier.name_len;
				childframe.num_variables = 0;
				childframe.num_arguments = 0;
				tok++; tokensleft--; /*now lparen*/
				assert(tokensleft);
				tok++; tokensleft--; /*now arg or rparen*/
				assert(tokensleft);
				if (tok->type != IDCP_TOKENTYPE_RPAREN) {
					for (;;) {
						idcp_dprintf4("  getting argument #%d\n", childframe.num_arguments);
						assert(childframe.num_arguments < IDCP_MAX_ARGUMENTS);
						arg = childframe.arguments + childframe.num_arguments;
						idcp_eval_expression_value(idcp, arg, frame, &tok, &tokensleft);/*jeanine:s:a:r;i:8;*/
						assert(arg->type != IDCP_VARIABLE_TYPE_VOID);
						idcp_dprintf4("  got argument #%d\n", childframe.num_arguments);
						childframe.num_arguments++;
						if (tok->type == IDCP_TOKENTYPE_RPAREN) {
							break;
						}
						assert(tok->type == IDCP_TOKENTYPE_COMMA);
						tok++; tokensleft--;
						assert(tokensleft);
					}
				}
				idcp_execute_function(idcp, &childframe);
				if (childframe.returnvalue.type == IDCP_VARIABLE_TYPE_VOID) {
					idcp_dprintf4("  returned void\n");
					assert(current_operation == IDCP_EVAL_OP_NONE);
					assert(tokensleft);
					tok++; tokensleft--;
					assert(tok->type == IDCP_TOKENTYPE_COMMA || tok->type == IDCP_TOKENTYPE_SEMICOLON);
					goto ret;
				}
				tmp_var = childframe.returnvalue;
			} else if ((tok + 1)->type == IDCP_TOKENTYPE_EQ) {
				idcp_dprintf4("    has eq, is assignment\n");
				tmp_var.name = tok->data.identifier.name;
				tok++; tokensleft--; /*now eq*/
				assert(tokensleft);
				tok++; tokensleft--; /*now start of next expr*/
				assert(tokensleft);
				idcp_eval_expression_value(idcp, &tmp_var, frame, &tok, &tokensleft);/*jeanine:s:a:r;i:8;*/
				assert(tmp_var.type != IDCP_VARIABLE_TYPE_VOID);
				idcp_set_variable_value(idcp, &tmp_var, tok, frame, tmp_var.name);/*jeanine:r:i:13;*/
				/*since eval expr will put tok ptr at the end, we'll need to go back one,
				otherwise it might stop at a semicolon, and we'll advance (see below) past it = no good*/
				tok--; tokensleft++;
			} else {
				idcp_dprintf4("    no lparen nor eq, is variable\n");
				idcp_get_variable_value(idcp, &tmp_var, tok, frame);/*jeanine:r:i:9;*/
				assert(tmp_var.type != IDCP_VARIABLE_TYPE_VOID);
			}
			break;
		case IDCP_TOKENTYPE_NUMBER:
		case IDCP_TOKENTYPE_HEXNUMBER:
			idcp_dprintf4("  number|hexnumber 0x%x\n", tok->data.integer.value);
			assert(!have_value || current_operation != IDCP_EVAL_OP_NONE);
			tmp_var.type = IDCP_VARIABLE_TYPE_INT;
			tmp_var.value.integer = tok->data.integer.value;
			break;
		case IDCP_TOKENTYPE_STRING:
			idcp_dprintf4("  string '%s'\n", tok->data.string.value);
			assert(!have_value || current_operation != IDCP_EVAL_OP_NONE);
			tmp_var.type = IDCP_VARIABLE_TYPE_STRING;
			tmp_var.value.string = tok->data.string.value;
			break;
		case IDCP_TOKENTYPE_PLUS:
			idcp_dprintf4("  plus\n");
			assert(have_value && current_operation == IDCP_EVAL_OP_NONE);
			current_operation = IDCP_EVAL_OP_PLUS;
			goto got_op;
		case IDCP_TOKENTYPE_PIPE:
			idcp_dprintf4("  pipe\n");
			assert(have_value && current_operation == IDCP_EVAL_OP_NONE);
			current_operation = IDCP_EVAL_OP_OR;
			goto got_op;
		case IDCP_TOKENTYPE_AMP:
			idcp_dprintf4("  amp\n");
			assert(have_value && current_operation == IDCP_EVAL_OP_NONE);
			current_operation = IDCP_EVAL_OP_AND;
			goto got_op;
		case IDCP_TOKENTYPE_TILDE:
			idcp_dprintf4("  tilde\n");
			assert(!have_value && current_operation == IDCP_EVAL_OP_NONE);
			/*this currently will only work if the NOT is first in an expression chain,
			ie: ~A | B will work,
			while B | ~A will not.*/
			have_value = 1; /*hack for this unary operator*/
			current_operation = IDCP_EVAL_OP_NOT;
			goto got_op;
		case IDCP_TOKENTYPE_COMMA:
		case IDCP_TOKENTYPE_SEMICOLON:
		case IDCP_TOKENTYPE_RPAREN:
			idcp_dprintf4("  comma|semicolon|rparen\n");
			assert(have_value && current_operation == IDCP_EVAL_OP_NONE);
			goto ret;
		default:
			printf("%d: unexpected token %s while evaluating expression\n", tok->line, IDCP_TOKEN_NAME[tok->type]);
			assert(0);
		}
		switch (current_operation) {
		case IDCP_EVAL_OP_NONE:
			idcp_dprintf4("  (setting initial value)\n");
			assert(!have_value);
			assert(result->type == IDCP_VARIABLE_TYPE_VOID);
			tmp_name = result->name;
			*result = tmp_var;
			result->name = tmp_name;
			have_value = 1;
			break;
		case IDCP_EVAL_OP_PLUS:
			idcp_dprintf4("  (executing plus)\n");
			assert(tmp_var.type == IDCP_VARIABLE_TYPE_INT);
			assert(result->type == IDCP_VARIABLE_TYPE_INT);
			result->value.integer += tmp_var.value.integer;
			current_operation = IDCP_EVAL_OP_NONE;
			break;
		case IDCP_EVAL_OP_OR:
			idcp_dprintf4("  (executing or)\n");
			assert(tmp_var.type == IDCP_VARIABLE_TYPE_INT);
			assert(result->type == IDCP_VARIABLE_TYPE_INT);
			result->value.integer |= tmp_var.value.integer;
			current_operation = IDCP_EVAL_OP_NONE;
			break;
		case IDCP_EVAL_OP_AND:
			idcp_dprintf4("  (executing and)\n");
			assert(tmp_var.type == IDCP_VARIABLE_TYPE_INT);
			assert(result->type == IDCP_VARIABLE_TYPE_INT);
			result->value.integer &= tmp_var.value.integer;
			current_operation = IDCP_EVAL_OP_NONE;
			break;
		case IDCP_EVAL_OP_NOT:
			idcp_dprintf4("  (executing not)\n");
			assert(tmp_var.type == IDCP_VARIABLE_TYPE_INT);
			result->value.integer = ~tmp_var.value.integer;
			result->type = IDCP_VARIABLE_TYPE_INT;
			current_operation = IDCP_EVAL_OP_NONE;
			break;
		default:
			assert(0);
		}
got_op:
		tok++; tokensleft--;
		assert(tokensleft);
	}
ret:
#if IDCP_VERBOSE_LEVEL >= 4
	switch (result->type) {
	case IDCP_VARIABLE_TYPE_VOID:
		idcp_dprintf4("end eval, result void\n");
		break;
	case IDCP_VARIABLE_TYPE_INT:
	case IDCP_VARIABLE_TYPE_STRUCT_MEMBER_ID:
		idcp_dprintf4("end eval, result int 0x%x\n", result->value.integer);
		break;
	case IDCP_VARIABLE_TYPE_STRING:
		idcp_dprintf4("end eval, result string '%s'\n", result->value.string);
		break;
	default:
		assert(0);
	}
#endif
	*_tok = tok;
	*_tokensleft = tokensleft;
}
/*jeanine:p:i:11;p:4;a:r;x:42.94;y:12.42;*/
static
void idcp_execute_function(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	struct idcp_variable tmp_variable;
	struct idcp_function *func;
	struct idcp_token *tok;
	int tokensleft, i;

#if IDCP_VERBOSE_LEVEL >= 3
	idcp_dprintf3("start function '%s' num args %d:", frame->function_name, frame->num_arguments);
	for (i = 0; i < frame->num_arguments; i++) {
		switch (frame->arguments[i].type) {
		case IDCP_VARIABLE_TYPE_VOID:
			assert(0);
		case IDCP_VARIABLE_TYPE_INT:
		case IDCP_VARIABLE_TYPE_STRUCT_MEMBER_ID:
			idcp_dprintf3(" 0x%x", frame->arguments[i].value.integer);
			break;
		case IDCP_VARIABLE_TYPE_STRING:
			idcp_dprintf3(" \"%s\"", frame->arguments[i].value.string);
			break;
		default:
			assert(0);
		}
	}
	idcp_dprintf3("\n");
#endif
	if (idcp_execute_internal_function(idcp, frame)) {/*jeanine:r:i:10;*/
		goto ret;
	}
	/*find the function*/
	for (i = 0, func = idcp->functions;; i++, func++) {
		if (i >= idcp->num_functions) {
			printf("can't find function '%s'\n", frame->function_name);
			assert(0);
		}
		if (!strcmp(func->name, frame->function_name)) {
			break;
		}
	}
	idcp_dprintf2("function '%s'\n", frame->function_name);
	/*put arguments into local variables as determined by parameters*/
	assert(func->num_parameters == frame->num_arguments);
	for (i = 0; i < frame->num_arguments; i++) {
		assert(frame->num_variables < IDCP_MAX_LOCAL_VARIABLES);
		frame->variables[frame->num_variables] = frame->arguments[i];
		frame->variables[frame->num_variables].name = func->parameters[i];
		frame->variables[frame->num_variables].name_len = func->parameters_len[i];
		frame->num_variables++;
	}
	/*walk through the tokens and execute*/
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
	tok = idcp->tokens + func->start_token_idx;
	tokensleft = func->end_token_idx - func->start_token_idx;
	while (tokensleft) {
		if (tok->type == IDCP_TOKENTYPE_SEMICOLON) {
			/*empty statement, seen once in the Segments() function*/
			/*DO I REALLY NEED THIS BRANCH FOR ONE OF 750K LINES?*/
			tok++; tokensleft--;
			continue;
		}
		assert(tok->type == IDCP_TOKENTYPE_IDENTIFIER);
		if (tok->data.identifier.name_len == 6 && !strcmp("return", tok->data.identifier.name)) {
			tok++; tokensleft--;
			assert(tokensleft);
			if (tok->type != IDCP_TOKENTYPE_SEMICOLON) {
				idcp_eval_expression_value(idcp, &frame->returnvalue, frame, &tok, &tokensleft);/*jeanine:r:i:8;*/
				assert(frame->returnvalue.type != IDCP_VARIABLE_TYPE_VOID);
			}
			goto ret;
		} else if (tok->data.identifier.name_len == 4 && !strcmp("auto", tok->data.identifier.name)) {
			/*declaring a variable*/
			tok++; tokensleft--;
			assert(tokensleft);
			assert(tok->type == IDCP_TOKENTYPE_IDENTIFIER);
			assert(frame->num_variables < IDCP_MAX_LOCAL_VARIABLES);
			frame->variables[frame->num_variables].name = tok->data.identifier.name;
			frame->variables[frame->num_variables].name_len = tok->data.identifier.name_len;
			frame->variables[frame->num_variables].type = IDCP_VARIABLE_TYPE_INT;
			frame->variables[frame->num_variables].value.integer = 0;
			frame->num_variables++;
			tok++; tokensleft--;
			assert(tokensleft);
		} else {
			idcp_eval_expression_value(idcp, &tmp_variable, frame, &tok, &tokensleft);
		}
		assert(tok->type == IDCP_TOKENTYPE_SEMICOLON);
		tok++; tokensleft--;
	}
ret:
	;
#if IDCP_VERBOSE_LEVEL >= 4
	switch (frame->returnvalue.type) {
	case IDCP_VARIABLE_TYPE_VOID:
		idcp_dprintf3("done function '%s', result void\n", frame->function_name);
		break;
	case IDCP_VARIABLE_TYPE_INT:
	case IDCP_VARIABLE_TYPE_STRUCT_MEMBER_ID:
		idcp_dprintf3("done function '%s', result int 0x%x\n", frame->function_name, frame->returnvalue.value.integer);
		break;
	case IDCP_VARIABLE_TYPE_STRING:
		idcp_dprintf3("done function '%s', result string '%s'\n", frame->function_name, frame->returnvalue.value.string);
		break;
	default:
		assert(0);
	}
#endif
}
/*jeanine:p:i:4;p:0;a:t;x:3.33;*/
/**
May exit the program.

@param idcp an uninitialized instance of struct idcparse
*/
void idcparse(struct idcparse *idcp, char *chars, int length)
{
	struct idcp_functioncallframe frame;

	/*init idcp*/
	memset(idcp, 0, sizeof(struct idcparse));
	idcp->last_added_struct_member_struct_id = -1;
	/*Mallocing a token str pool size of 'length' should mean we'll never overrun that buffer.*/
	idcp->token_str_pool = idcp->token_str_pool_ptr = malloc(length);
	assert(((void)"failed str pool malloc", idcp->token_str_pool));

	idcparse_tokenize(idcp, chars, length);/*jeanine:r:i:1;*/
	idcp_dprintf1("%d tokens %d lines\n", idcp->num_tokens, idcp->num_lines);
	idcp_print_function_info(idcp);/*jeanine:r:i:3;*/

	frame.function_name = "main";
	frame.function_name_len = 4;
	frame.num_arguments = 0;
	frame.num_variables = 0;
	idcp_execute_function(idcp, &frame);/*jeanine:r:i:11;*/
}