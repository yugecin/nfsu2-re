// -----------------------------
// Allows selecting preset cars in quickrace menu

#ifndef CUSTOM_IS_PRESET_CAR
#error please include "fun-car-customize-preset.c" first
#endif

// copied from replace-4EED10-CarSelectFNGObject__ChangeCategory.c and stripped to just do what we need
static
int fun_use_preset_cars_in_quickrace_CarSelectFNGObject__ChangeCategory(struct CarSelectFNGObject *this, void *_, unsigned int message)
{
#define MSG_PREV 0x5073EF13
#define MSG_NEXT 0xD9FEEC59

	struct UIElement *olGroup;
	enum INVENTORY_CAR_FLAGS newCarSelectCategory = *carSelectCategory;

	if (profileData->menuState == MENU_STATE_MAIN_MENU || profileData->menuState == 4) {
		if (message == MSG_PREV) {
			// stock|tuned -> preset -> (sponsor ->) (career ->) (tuned ->) stock -> (loop)
			switch (newCarSelectCategory) {
			case IS_STOCK_CAR | IS_TUNED_CAR:
				newCarSelectCategory = CUSTOM_IS_PRESET_CAR; break;
			case CUSTOM_IS_PRESET_CAR:
				if (CarSelectFNGObject__CountAvailableCars(this, IS_SPONSOR_CAR)) {
					newCarSelectCategory = IS_SPONSOR_CAR;
					break;
				}
			case IS_SPONSOR_CAR:
				if (CarSelectFNGObject__CountAvailableCars(this, IS_CAREER_CAR)) {
					newCarSelectCategory = IS_CAREER_CAR;
					break;
				}
			case IS_CAREER_CAR:
				if (CarSelectFNGObject__CountAvailableCars(this, IS_TUNED_CAR)) {
					newCarSelectCategory = IS_TUNED_CAR;
					break;
				}
			case IS_TUNED_CAR:
				newCarSelectCategory = IS_STOCK_CAR;
				break;
			case IS_STOCK_CAR:
				newCarSelectCategory = IS_STOCK_CAR | IS_TUNED_CAR;
				break;
			}
		} else if (message == MSG_NEXT) {
			// stock|tuned -> stock -> (tuned ->) (career ->) (sponsor ->) preset -> (loop)
			switch (newCarSelectCategory) {
			case IS_STOCK_CAR | IS_TUNED_CAR:
				newCarSelectCategory = IS_STOCK_CAR;
				break;
			case IS_STOCK_CAR:
				if (CarSelectFNGObject__CountAvailableCars(this, IS_TUNED_CAR)) {
					newCarSelectCategory = IS_TUNED_CAR;
					break;
				}
			case IS_TUNED_CAR:
				if (CarSelectFNGObject__CountAvailableCars(this, IS_CAREER_CAR)) {
					newCarSelectCategory = IS_CAREER_CAR;
					break;
				}
			case IS_CAREER_CAR:
				if (CarSelectFNGObject__CountAvailableCars(this, IS_SPONSOR_CAR)) {
					newCarSelectCategory = IS_SPONSOR_CAR;
					break;
				}
			case IS_SPONSOR_CAR:
				newCarSelectCategory = CUSTOM_IS_PRESET_CAR;
				break;
			case CUSTOM_IS_PRESET_CAR:
				newCarSelectCategory = IS_STOCK_CAR | IS_TUNED_CAR;
				break;
			}
		}
		*carSelectCategory = newCarSelectCategory;
		profileData->players[profileData->currentPlayerIndex].d4.currentCarSelectionCategory = newCarSelectCategory;
		ThisCallNoArgs(CarSelectFNGObject__ResetBrowableCars, this);
		ThisCallNoArgs(CarSelectFNGObject__UpdateUI, this);
		return 1;
	}
	return 0;
}

static
__declspec(naked)
void fun_use_preset_cars_in_quickrace_CarSelectFNGObject__ChangeCategory_hook(unsigned int message)
{
	_asm {
		push ecx
		call fun_use_preset_cars_in_quickrace_CarSelectFNGObject__ChangeCategory
		test eax, eax
		jnz ok
		// since we overwrote the hook for customize menu, call that now too
		call fun_car_customize_preset_CarSelectFNGObject__ChangeCategory
		test eax, eax
		jnz ok
		// we were not in quick race nor customize menu, call the normal function
		// but first do the thing that we overwrote in order to jmp to this proc
		pop ecx
		mov eax, 0x7F444C // carSelectCategory
		mov eax, [eax]
		mov edx, 0x4EED15 // edx can be used because it's overridden later in that proc anyways
		jmp edx
ok:
		add esp, 4
		retn 0x4
	}
}

static
__declspec(naked)
void fun_use_preset_cars_in_quickrace_CarCollection__CountAvailableCars_hook(enum INVENTORY_CAR_FLAGS flagsToCheck, int typeToCheck)
{
	_asm {
		test [esp+4], CUSTOM_IS_PRESET_CAR
		jnz its_preset
		// not preset, return back where we hooked from (but do the things we overwrote first)
		sub esp, 8
		push ebx
		mov ebx, [esp+0xC+8]
		mov eax, 0x534858
		jmp eax
its_preset:

		// not sure if we really need to count or just return nonzero to make things happy, but lets do a real count
		or eax, 0xFFFFFFFF
		mov ecx, 0x8A31E4
next:
		inc eax
		mov ecx, [ecx] // next
		cmp ecx, 0x8A31E4
		jnz next
		retn 8
	}
}

static
__declspec(naked)
void
fun_use_preset_cars_in_quickrace_FindPresetCarWhenTuningForIngameCar()
{
	_asm {
		cmp edi, [numPresetCars]
		jb its_preset
		// this is what we overwrote
		add edx, 0x9BD8 // sizeof(struct CarCollection)
		mov ecx, 0x525FC1
		jmp ecx

its_preset:
		imul edi, 0x1C // sizeof(SponsorCar)
		add edi, offset presetCarAsInventoryCar
		mov eax, 0x525FE6
		jmp eax
	}
}

static
void fun_use_preset_cars_in_quickrace()
{
	// call other init funcs first, because we need to overwrite a jmp that fun-car-customize-preset.c made
	INIT_FUNC();

	// partially replace ChangeCategory function to add a new category
	// of preset cars in the car select menu when in quickrace.
	mkjmp(0x4EED10, fun_use_preset_cars_in_quickrace_CarSelectFNGObject__ChangeCategory_hook);

	// override the one from fun-car-customize-preset to not return 0 in quickrace menu
	mkjmp(0x534850, fun_use_preset_cars_in_quickrace_CarCollection__CountAvailableCars_hook);

	// when going ingame, it searches the player's carcollection again based on carslothash, so we
	// need to return the preset car for our custom carslothash
	mkjmp(0x525FBB, fun_use_preset_cars_in_quickrace_FindPresetCarWhenTuningForIngameCar);

#undef INIT_FUNC
#define INIT_FUNC fun_use_preset_cars_in_quickrace
}
