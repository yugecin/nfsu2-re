/**
Code to _very_ naively parse an IDA .idc file, only targetted at parsing
files that are the output of a "dump database to IDC file" operation,
with the purpose to read common database stuff like enums, structs, functions etc.
*/

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

#define IDCP_VERBOSE_LEVEL 3
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

struct idcparse {
	int num_tokens;
	struct idcp_token tokens[IDCP_MAX_TOKENS];
	int num_functions;
	struct idcp_function functions[IDCP_MAX_FUNCTIONS];
	int num_enum_members;
	struct idcp_enum_member enum_members[IDCP_MAX_TOTAL_ENUM_MEMBERS];
	int num_enums;
	struct idcp_enum enums[IDCP_MAX_ENUMS];
	char *token_str_pool;
	char *token_str_pool_ptr;
	int num_lines;
};

static
struct idcp_token *idcp_get_next_token(struct idcparse *idcp)
{
	struct idcp_token *token;

	assert(((void)"hit IDCP_MAX_TOKENS limit", idcp->num_tokens < IDCP_MAX_TOKENS));
	token = idcp->tokens + idcp->num_tokens++;
	token->line = idcp->num_lines;
	return token;
}

static
struct idcp_function *idcp_get_function(struct idcparse *idcp, char *name)
{
	struct idcp_function *f;
	int i;

	f = idcp->functions;
	for (i = idcp->num_functions; i > 0; i--, f++) {
		if (!strcmp(name, f->name)) {
			return f;
		}
	}
	return NULL;
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
/*jeanine:p:i:3;p:4;a:r;x:33.00;*/
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
/*jeanine:p:i:1;p:4;a:r;x:103.88;y:-276.25;*/
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
/*jeanine:p:i:5;p:4;a:r;x:29.78;y:36.56;*/
#define IDCP_VARIABLE_TYPE_VOID 0
#define IDCP_VARIABLE_TYPE_INT 1
#define IDCP_VARIABLE_TYPE_STRING 2

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
/*jeanine:p:i:12;p:10;a:r;x:15.10;y:-41.13;*/
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
	e->start_idx = idcp->num_enum_members;
	e->end_idx = idcp->num_enum_members;
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_INT;
	frame->returnvalue.value.integer = idcp->num_enums - 1;
}
/*jeanine:p:i:14;p:10;a:r;x:14.78;y:-1.62;*/
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

	/*All enum members are added in one array and each enum holds indices where its members start and stop.
	So all members of a single enum must be consecutive entries in that array, and the easiest
	way to ensure that is to assert that members are only added to the most recently created enum.*/
	assert(enum_id == idcp->num_enums - 1);
	assert(idcp->num_enum_members < IDCP_MAX_TOTAL_ENUM_MEMBERS);
	idcp->enums[enum_id].end_idx++;
	m = idcp->enum_members + idcp->num_enum_members++;
	m->name = name;
	m->value = value;
	m->bmask = bmask;
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_VOID;
}
/*jeanine:p:i:15;p:10;a:r;x:14.89;y:-14.57;*/
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
/*jeanine:p:i:16;p:10;a:r;x:14.67;y:28.25;*/
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
	assert(0);
}
/*jeanine:p:i:17;p:10;a:r;x:14.56;y:47.56;*/
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
/*jeanine:p:i:18;p:10;a:r;x:15.00;y:-24.25;*/
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
/*jeanine:p:i:10;p:11;a:r;x:330.22;*/
/*see https://hex-rays.com/products/ida/support/idadoc/162.shtml for a list of built-in idc functions,
or simply check the compiled help files included in your IDA installation (press F1).*/
static
int idcp_execute_internal_function(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	register char *name;

	name = frame->function_name;
	switch (frame->function_name_len) {
	case 8:
		if (!strcmp("add_enum", name)) {
			idcp_func_add_enum(idcp, frame);/*jeanine:r:i:12;*/
			return 1;
		} else if (!strcmp("SegClass", name) || !strcmp("set_flag", name)) {
			goto ignoredbuiltin;
		}
		break;
	case 9:
		if (!strcmp("SegRename", name) || !strcmp("SegDefReg", name)) {
			goto ignoredbuiltin;
		}
		break;
	case 11:
		if (!strcmp("set_enum_bf", name)) {
			idcp_func_set_enum_bf(idcp, frame);/*jeanine:r:i:18;*/
			return 1;
		} else if (!strcmp("add_segm_ex", name)) {
			goto ignoredbuiltin;
		}
		break;
	case 12:
		if (!strcmp("set_enum_cmt", name)) {
			idcp_func_set_enum_cmt(idcp, frame);/*jeanine:r:i:15;*/
			return 1;
		} else if (!strcmp("set_selector", name) || !strcmp("get_inf_attr", name) || !strcmp("set_inf_attr", name)) {
			goto ignoredbuiltin;
		}
		break;
	case 13:
		if (!strcmp("set_segm_type", name)) {
			goto ignoredbuiltin;
		}
		break;
	case 15:
		if (!strcmp("add_enum_member", name)) {
			idcp_func_add_enum_member(idcp, frame);/*jeanine:r:i:14;*/
			return 1;
		} else if (!strcmp("get_enum_member", name)) {
			idcp_func_get_enum_member(idcp, frame);/*jeanine:r:i:16;*/
			return 1;
		} else if (!strcmp("add_default_til", name)) {
			goto ignoredbuiltin;
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
			goto ignoredbuiltin;
		}
		break;
	}
	return 0;
ignoredbuiltin:
	frame->returnvalue.type = IDCP_VARIABLE_TYPE_INT;
	frame->returnvalue.value.integer = 0;
	return 1;
}
/*jeanine:p:i:9;p:8;a:r;x:8.89;y:8.00;*/
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
	case 8:
		if (!strcmp(name, "UTP_ENUM")) {
			goto ignoredvariable;
		}
		break;
	case 9:
		if (!strcmp(name, "SW_ALLCMT")) {
			goto ignoredvariable;
		}
		break;
	case 10:
		if (!strcmp(name, "INF_MAXREF") || !strcmp(name, "INF_INDENT")) {
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
/*jeanine:p:i:13;p:8;a:r;x:8.78;y:-10.63;*/
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
{
	struct idcp_functioncallframe childframe;
	struct idcp_variable tmp_var, *arg;
	struct idcp_token *tok;
	int tokensleft, current_operation, have_value;

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
			if (tmp_var.type == IDCP_VARIABLE_TYPE_INT) {
				result->type = IDCP_VARIABLE_TYPE_INT;
				result->value.integer = tmp_var.value.integer;
			} else if (tmp_var.type == IDCP_VARIABLE_TYPE_STRING) {
				result->type = IDCP_VARIABLE_TYPE_STRING;
				result->value.string = tmp_var.value.string;
			} else {
				assert(((void)"dunno what to set variable type to, help", 0));
			}
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
/*jeanine:p:i:6;p:11;a:r;x:8.78;y:-24.06;*/
static
void idcp_debug_print_function_invocation(struct idcp_functioncallframe *frame)
{
	int i;

	idcp_dprintf3("start function '%s' num args %d:", frame->function_name, frame->num_arguments);
	for (i = 0; i < frame->num_arguments; i++) {
		switch (frame->arguments[i].type) {
		case IDCP_VARIABLE_TYPE_VOID:
			assert(0);
		case IDCP_VARIABLE_TYPE_INT:
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
}
/*jeanine:p:i:11;p:7;a:r;x:3.33;*/
static
void idcp_execute_function(struct idcparse *idcp, struct idcp_functioncallframe *frame)
{
	struct idcp_variable tmp_variable;
	struct idcp_function *func;
	struct idcp_token *tok;
	int tokensleft, i;

#if IDCP_VERBOSE_LEVEL >= 3
	idcp_debug_print_function_invocation(frame);/*jeanine:r:i:6;*/
#endif
	if (idcp_execute_internal_function(idcp, frame)) {/*jeanine:r:i:10;*/
		goto ret;
	}
	assert(func = idcp_get_function(idcp, frame->function_name));
	assert(func->num_parameters == frame->num_arguments);
	for (i = 0; i < frame->num_arguments; i++) {
		assert(frame->num_variables < IDCP_MAX_LOCAL_VARIABLES);
		frame->variables[frame->num_variables] = frame->arguments[i];
		frame->variables[frame->num_variables].name = func->parameters[i];
		frame->variables[frame->num_variables].name_len = func->parameters_len[i];
		frame->num_variables++;
	}
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
#if IDCP_VERBOSE_LEVEL >= 3
	switch (frame->returnvalue.type) {
	case IDCP_VARIABLE_TYPE_VOID:
		idcp_dprintf3("done function '%s', result void\n", frame->function_name);
		break;
	case IDCP_VARIABLE_TYPE_INT:
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
/*jeanine:p:i:7;p:5;a:b;y:1.88;*/
static
void idcp_execute(struct idcparse *idcp)
{
	struct idcp_functioncallframe frame;

	frame.function_name = "main";
	frame.function_name_len = 4;
	frame.num_arguments = 0;
	frame.num_variables = 0;
	idcp_execute_function(idcp, &frame);/*jeanine:r:i:11;*/
}
/*jeanine:p:i:4;p:0;a:t;x:3.33;*/
/**
May exit the program.

@param idcp an uninitialized instance of struct idcparse
*/
void idcparse(struct idcparse *idcp, char *chars, int length)
{
	/*init idcp*/
	memset(idcp, 0, sizeof(struct idcparse));
	/*Mallocing a token str pool size of 'length' should mean we'll never overrun that buffer.*/
	idcp->token_str_pool = idcp->token_str_pool_ptr = malloc(length);
	assert(((void)"failed str pool malloc", idcp->token_str_pool));

	idcparse_tokenize(idcp, chars, length);/*jeanine:r:i:1;*/
	idcp_dprintf1("%d tokens %d lines\n", idcp->num_tokens, idcp->num_lines);
	idcp_print_function_info(idcp);/*jeanine:r:i:3;*/
	idcp_execute(idcp);/*jeanine:r:i:5;*/
}
