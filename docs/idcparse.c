/**
Code to naively parse an IDA .idc file
*/

#define IDCP_MAX_TOKENS 6000000
#define IDCP_MAX_FUNCTIONS 500

#define IDCP_VERBOSE_LEVEL 3

/*verbose*/
#if IDCP_VERBOSE_LEVEL == 5
#define idcp_dprintf5(...) printf(__VA_ARGS__)
#define idcp_dprintf4(...) printf(__VA_ARGS__)
#define idcp_dprintf3(...) printf(__VA_ARGS__)
#define idcp_dprintf2(...) printf(__VA_ARGS__)
#define idcp_dprintf1(...) printf(__VA_ARGS__)
#elif IDCP_VERBOSE_LEVEL == 4
#define idcp_dprintf5(...)
#define idcp_dprintf4(...) printf(__VA_ARGS__)
#define idcp_dprintf3(...) printf(__VA_ARGS__)
#define idcp_dprintf2(...) printf(__VA_ARGS__)
#define idcp_dprintf1(...) printf(__VA_ARGS__)
#elif IDCP_VERBOSE_LEVEL == 3
#define idcp_dprintf5(...)
#define idcp_dprintf4(...)
#define idcp_dprintf3(...) printf(__VA_ARGS__)
#define idcp_dprintf2(...) printf(__VA_ARGS__)
#define idcp_dprintf1(...) printf(__VA_ARGS__)
#elif IDCP_VERBOSE_LEVEL == 2
#define idcp_dprintf5(...)
#define idcp_dprintf4(...)
#define idcp_dprintf3(...)
#define idcp_dprintf2(...) printf(__VA_ARGS__)
#define idcp_dprintf1(...) printf(__VA_ARGS__)
#elif IDCP_VERBOSE_LEVEL == 1
#define idcp_dprintf5(...)
#define idcp_dprintf4(...)
#define idcp_dprintf3(...)
#define idcp_dprintf2(...)
#define idcp_dprintf1(...) printf(__VA_ARGS__)
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
	/**first token in idcparse.tokens that is part of the function*/
	int start_token_idx;
	/**last token in idcparse.tokens that is part of the function, exclusive*/
	int end_token_idx;
};

struct idcparse {
	int num_tokens;
	struct idcp_token tokens[IDCP_MAX_TOKENS];
	int num_functions;
	struct idcp_function functions[IDCP_MAX_FUNCTIONS];
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
/*jeanine:p:i:2;p:1;a:r;x:3.33;*/
static
char *idcp_find_function_name_from_lbrace_token(struct idcparse *idcp)
{
	int i, type;

	/*current token is LBRACE+1*/
	i = idcp->num_tokens - 2;
	if (idcp->tokens[i].type == IDCP_TOKENTYPE_RPAREN) {
		for (;;) {
			i--;
			assert(i);
			type = idcp->tokens[i].type;
			if (type == IDCP_TOKENTYPE_LPAREN) {
				assert(i);
				assert(idcp->tokens[i - 1].type == IDCP_TOKENTYPE_IDENTIFIER);
				return idcp->tokens[i - 1].data.identifier.name;
			}
			assert(type == IDCP_TOKENTYPE_IDENTIFIER || type == IDCP_TOKENTYPE_COMMA);
		}
	}
	return NULL;
}
/*jeanine:p:i:3;p:4;a:r;x:6.11;y:2.94;*/
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
/*jeanine:p:i:1;p:4;a:r;x:125.44;*/
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
		else if (c == '-' && charsleft && charmap[*chars] & IDCP_CHARTYPE_NUMBER) {
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
					} else {
						assert(((void)"unexpected character after \\ in string", *chars));
					}
					c = '"';
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
				tmp_str_ptr = idcp_find_function_name_from_lbrace_token(idcp);/*jeanine:r:i:2;*/
				if (tmp_str_ptr) {
					assert(((void)"hit IDCP_MAX_FUNCTIONS limit", idcp->num_functions < IDCP_MAX_FUNCTIONS));
					current_function = idcp->functions + idcp->num_functions++;
					current_function->name = tmp_str_ptr;
					current_function->start_token_idx = idcp->num_tokens;
					current_function->end_token_idx = idcp->num_tokens;
				}
			}
			brace_depth++;
		} else if (c == '}') {
			idcp_get_next_token(idcp)->type = IDCP_TOKENTYPE_RBRACE;
			idcp_dprintf5("%d: T_RBRACE\n", idcp->num_lines);
			brace_depth--;
			if (!brace_depth && current_function) {
				current_function->end_token_idx = idcp->num_tokens;
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
}
