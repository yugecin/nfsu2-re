static
void cb_handler_symbols_start()
{
	append("<p><strong>Symbols:</strong></p><ul>", 36);
}

struct HANDLER handler_symbols = {
	cb_handler_symbols_start,
	cb_handler_ul_text,
	cb_handler_ul_end,
	cb_handler_normal_directive_start,
	cb_handler_normal_directive_end,
	"symbols"
};

static
void mmparse_ext_init_handler_symbols()
{
	MMPARSE_EXT_INIT();

	mmparse_register_handler(&handler_symbols);
}

#undef MMPARSE_EXT_INIT
#define MMPARSE_EXT_INIT mmparse_ext_init_handler_symbols
