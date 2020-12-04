static
void replaceloadingscreen()
{
	strcpy((char*) 0x790734, "loading_boot.fng");

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC replaceloadingscreen
}
