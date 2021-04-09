static
__declspec(naked)
void modifyJoyIdsInCareer()
{
	_asm {
		mov eax, [esp+4]
		cmp eax, JOY_EVENT_REQUEST_WORLD_MAP //JOY_EVENT_CAMERA_LOOKBACK
		jnz check1
		mov [esp+4], JOY_EVENT_AUTO_PILOT // works
		mov [esp+4], JOY_EVENT_HONK_HORN // nop?
		mov [esp+4], JOY_EVENT_ZONE_PREVIEW // crash on first test
		mov [esp+4], JOY_EVENT_ZONE_SLOMO // hide hud, slow mo for a few seconds, restore
		mov [esp+4], JOY_EVENT_ZONE_WARP // nop on first test
		mov [esp+4], JOY_EVENT_AIM_ROCKET_UD // nop on first test
		mov [esp+4], JOY_EVENT_NITRO_BOOST_ALTERNATE // holds nos until depleted
		mov [esp+4], JOY_EVENT_CAMERA_POV_CHANGE // c
		mov [esp+4], JOY_EVENT_CYCLE_HUD // hides hud elements
		mov [esp+4], JOY_EVENT_REQUEST_MESSAGE_SYSTEM
		jmp ok
check1:
ok:
		push 0x605C68 // ret addr
		mov eax, 0x602BD0
		jmp eax
	}
}

static
void initSwitchJoyIdsInCareer()
{
	mkjmp(0x605C63, modifyJoyIdsInCareer);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initSwitchJoyIdsInCareer
}
