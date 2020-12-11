static
void skipinsertdisc2()
{
	*(char*) 0x79DC60 = 1;

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC skipinsertdisc2
}
