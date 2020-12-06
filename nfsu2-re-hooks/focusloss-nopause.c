static
void focusloss_nopause()
{
	nop(0x5CCF40, 6);
	// should the call after that also be blocked?

	//nop(0x5CCE3A, 5);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC focusloss_nopause
}
