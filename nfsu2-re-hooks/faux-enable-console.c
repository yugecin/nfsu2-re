/*
This doesn't do anything except for saving the string you're typing into the console_real_text
variable. Caret position can be changed with home, end, and arrow keys. Return and tab inserts
linefeeds. Escape inserts a zero byte at caret pos.
*/

char **console_text_string;
int *console_text_string_length;
int *console_text_string_caret_position;
int *console_enabled;
int *console_text_string_max_length;

char console_real_text[100];

static
void initConsolePOC()
{
	console_text_string = (char**) 0x8709B0;
	console_text_string_length = (int*) 0x8709B4;
	console_text_string_caret_position = (int*) 0x8709b8;
	console_enabled = (int*) 0x8709BC;
	console_text_string_max_length = (int*) 0x8709C0;

	*console_text_string = console_real_text;
	*console_text_string_max_length = sizeof(console_real_text);
	*console_enabled = 1;

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initConsolePOC
}
