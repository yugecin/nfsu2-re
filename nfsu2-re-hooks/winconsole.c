static
void winconsole()
{
	AllocConsole();
	freopen("CONOUT$", "w+", stdout);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC winconsole
}
