static
void speedyboot()
{
	static int zero = 0;

	*(int**) (0x7F65E8) = &zero; /*discerrorpc*/
	*(int**) (0x7F65EC) = &zero; /*??nothing*/
	//*(int**) (0x7F65F0) = &zero; /*mc_bootup*/
	*(int**) (0x7F65F4) = &zero; /*??nothing*/
	*(int**) (0x7F65F8) = &zero; /*??nothing*/
	*(int**) (0x7F65FC) = &zero; /*blankmovie*/
	*(int**) (0x7F6600) = &zero; /*ealogo*/
	*(int**) (0x7F6604) = &zero; /*??nothing*/
	*(int**) (0x7F6608) = &zero; /*thxmovie*/
	*(int**) (0x7F660C) = &zero; /*psamovie*/
	*(int**) (0x7F6610) = &zero; /*introfmv*/
	*(int**) (0x7F6614) = &zero; /*splash*/

	//*(int**) (0x7F6618) = &zero; /*mc_background*/
	//*(int**) (0x7F661C) = &zero; /*ui_main*/

	*canUseQToExit = 1;

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC speedyboot
}
