// -----------------------------
// Allows selecting preset cars in customization menu

static
__declspec(naked)
int
__stdcall
CarSelectFNGObject__CountAvailableCars(struct CarSelectFNGObject *this, enum INVENTORY_CAR_FLAGS flags)
{
	_asm { pop eax }
	_asm { push 0x497EE0 }
	_asm { push eax }
	_asm { jmp ThisCallOneArg }
}

#define CarSelectFNGObject__ResetBrowableCars 0x4EEC90 /*()*/
#define CarSelectFNGObject__UpdateUI 0x4B2310 /*()*/
#define CarCollection__CreateNewTunedCarFromFromDataAtSlot 0x52A710 /*(unsigned int slotNamehash)*/
#define SponsorCar__ApplyTuningToInstance 0x5039D0 /*(int playerIndex, struct MenuCarInstance*, int)*/
#define TunedCar18__CopyTuningFromMenuCarInstance 0x503950 /*(struct MenuCarInstance*)*/

#define CUSTOM_IS_PRESET_CAR 0x20

// copied from replace-4EED10-CarSelectFNGObject__ChangeCategory.c and stripped to just do what we need
static
int fun_car_customize_preset_CarSelectFNGObject__ChangeCategory(struct CarSelectFNGObject *this, void *_, unsigned int message)
{
#define MSG_PREV 0x5073EF13
#define MSG_NEXT 0xD9FEEC59

	struct UIElement *olGroup;
	enum INVENTORY_CAR_FLAGS newCarSelectCategory = *carSelectCategory;

	if (profileData->menuState == MENU_STATE_CAR_CUSTOMIZE) {
		if (message == MSG_PREV) {
			// stock|tuned -> preset -> (tuned ->) stock -> (loop)
			switch (newCarSelectCategory) {
			case IS_STOCK_CAR | IS_TUNED_CAR:
				newCarSelectCategory = CUSTOM_IS_PRESET_CAR;
				break;
			case CUSTOM_IS_PRESET_CAR:
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
			// stock|tuned -> stock -> (tuned ->) preset -> (loop)
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

		olGroup = FindUIElementByHash(this->__parent.fngName, cihash("OL_CarMode_Group"));
		HideNullableUIElementAndChildren(olGroup);

		// checking the actual value instead of 'newCarSelectCategory', because if our hooks aren't
		// good enough, something might've changed the actual value back to something else (because
		// no cars are found with our custom filter)
		if (*carSelectCategory == CUSTOM_IS_PRESET_CAR) {
			ShowNullableUIElementAndChildren(olGroup);
			SetUIElementAnimationByName(olGroup, "SHOW", 0);
		}

		return 1;
	}
	return 0;
}

static
__declspec(naked)
void fun_car_customize_preset_CarSelectFNGObject__ChangeCategory_hook(unsigned int message)
{
	_asm {
		push ecx
		call fun_car_customize_preset_CarSelectFNGObject__ChangeCategory
		test eax, eax
		jnz ok
		// we were not in customize menu, call the normal function
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
void fun_car_customize_preset_CarCollection__CountAvailableCars_hook(enum INVENTORY_CAR_FLAGS flagsToCheck, int typeToCheck)
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

// my game has 32, assuming all versions will have the same ones
#define MAX_PRESET_CARS 32
struct SponsorCar presetCarAsInventoryCar[MAX_PRESET_CARS];
int numPresetCars;

static
struct InventoryCar *
__stdcall
FindPresetCarAfterGivenCar(enum INVENTORY_CAR_FLAGS flagsToCheck, struct InventoryCar *givenCar)
{
	struct SponsorCar *s;
	struct CarPreset *p;
	int i;
	char buf[32];

	if (!numPresetCars) {
		p = (void*) carPresets;
		for (i = 0;
			(p = (void*) p->link.next) != (void*) carPresets &&
			i < MAX_PRESET_CARS;
			i++)
		{
			sprintf(buf, "STOCK_%s", p->modelName);
			s = presetCarAsInventoryCar + numPresetCars++;
			s->__parent.vtable = (void*) 0x79AD18;
			s->__parent.field_4 = 0;
			s->__parent.slotHash = i; //cihash(buf); // HELP
			s->__parent.floatField_C = 0.0f;
			s->__parent.field_10 = 0;
			s->__parent.flags14 = CUSTOM_IS_PRESET_CAR;
			s->carPresetHash = cihash(p->name);
		}
	}
	if (!givenCar) {
		return &presetCarAsInventoryCar->__parent;
	}
	p = (void*) carPresets; // (CarPreset's link is at offset 0)
	for (i = 0; i < numPresetCars - 1; i++) {
		if (presetCarAsInventoryCar + i == (void*) givenCar) {
			return (void*) (presetCarAsInventoryCar + i + 1);
		}
	}
	return NULL;
}

static
__declspec(naked)
struct InventoryCar*
fun_car_customize_preset_CarCollection__FindCarWithFlagAfterGivenCar_hook(enum INVENTORY_CAR_FLAGS flagsToCheck, struct InventoryCar *givenCar)
{
	_asm {
		test [esp+4], CUSTOM_IS_PRESET_CAR
		jnz its_preset
		// not preset, return back where we hooked from (but do the things we overwrote first)
		push ebx
		push esi
		push edi
		mov edi, [esp+0x14]
		mov eax, 0x5162D7
		jmp eax
its_preset:
		jmp FindPresetCarAfterGivenCar
	}
}

#define hashof_carselect_category_label 0x3E81DE59
#define hashof_OL_CarMode_Group 0x2043ABA0
#define FindUIElementByHash 0x5379C0 /*(char *fngName, unsigned int hash)*/
#define HideNullableUIElementAndChildren 0x50CA00 /*(struct UIElement *uielement)*/
#define ShowNullableUIElementAndChildren 0x50CA50 /*(struct UIElement *uielement)*/
#define SetUIElementAnimationByName 0x51CF70 /*(struct UIElement *uielement, char *animationStr, char resetAnimationWhenThisAnimationAlreadyActive)*/
static char *sUIQR_CarSelect = "UI_QRCarSelect.fng", *sPresetCars = "Preset cars", *sSHOW = "SHOW";

static
__declspec(naked)
void
fun_car_customize_preset_CarSelectFNGObject__UpdateUI_hook()
{
	_asm {
		mov eax, 0x7F444C // carSelectCategory
		test [eax], CUSTOM_IS_PRESET_CAR
		jz not_preset
		pushad
		// change the text of the category label to "Preset cars"
		push sPresetCars
		push hashof_carselect_category_label
		push sUIQR_CarSelect
		mov eax, 0x537B80
		call eax
		// show ol_carmode_group
		push hashof_OL_CarMode_Group
		push sUIQR_CarSelect
		mov eax, FindUIElementByHash
		call eax
		push eax
		push eax
		mov eax, ShowNullableUIElementAndChildren
		call eax
		// change the animation of ol_carmode_group to SHOW because usually it will still be HIDEµ
		pop eax // olGroup
		push 0 // resetAnimationWhenThisAnimationAlreadyActive
		push sSHOW
		push eax
		mov eax, SetUIElementAnimationByName
		call eax
		// change label of ol_carmode_group to name of preset car
		add esp, 0x24
		popad
not_preset:
		// ensure the ol_carmode_group is hidden
		pushad
		push hashof_OL_CarMode_Group
		push sUIQR_CarSelect
		mov eax, FindUIElementByHash
		call eax
		push eax
		mov eax, HideNullableUIElementAndChildren
		call eax
		add esp, 0xC
		popad
		// we overwrote a jmp so just execute that to get back
		mov eax, 0x497CD0
		jmp eax
	}
}

static
__declspec(naked)
struct SponsorCar *
fun_car_customize_preset_CarCollection__GetCarForSlot_hook(unsigned int slotHash)
{
	_asm {
		mov eax, [numPresetCars]
		cmp [esp+4], eax
		jb its_preset
		push esi
		push edi
		mov edi, [esp+0xC]
		mov eax, 0x503516
		jmp eax
its_preset:
		mov eax, [esp+4]
		imul eax, 0x1C // sizeof(SponsorCar)
		add eax, offset presetCarAsInventoryCar
		retn 4
	}
}

static
__declspec(naked)
struct CarPreset*
FindCarPreset(unsigned int nameHash)
{
	_asm { mov eax, 0x61C460 }
	_asm { jmp eax }
}

static
struct TunedCar*
__stdcall
fun_car_customize_preset_CustomizeCar_set_car_instance_if_missing(struct CarCollection *this, unsigned int slotNameHash)
{
	struct MenuCarInstance *menuCarInstance;
	struct SponsorCar *presetCar;
	struct TunedCar* tunedCar;
	char buf[32];
	int i;

	// if car instance is missing, just assume it's a preset car. Not checking if slotNameHash actually is
	// our custom slotNameHash. Game will crash anyways if we can't create an instance here.
	presetCar = presetCarAsInventoryCar + slotNameHash;

	// skipping null check on the result of FindCarPreset
	sprintf(buf, "STOCK_%s", FindCarPreset(presetCar->carPresetHash)->modelName);
	tunedCar = ThisCallOneArg(CarCollection__CreateNewTunedCarFromFromDataAtSlot, this, (void*) cshash(buf));
	menuCarInstance = (void*) 0x8389D0;
	// (3rd param is MenuCarInstance, using an instance in the data section that
	//  gets overridden later in the Customize process anyways)
	// (4th param is unknown, but unused in SponsorCar's ApplyTuning
	ThisCallThreeArgs(SponsorCar__ApplyTuningToInstance, presetCar, (void*) profileData->currentPlayerIndex, menuCarInstance, 0);
	// copy tuning back from MenuCarInstance to tuned car instance
	ThisCallOneArg(TunedCar18__CopyTuningFromMenuCarInstance, &tunedCar->field_18, menuCarInstance);
	return tunedCar;
}

static
__declspec(naked)
void
fun_car_customize_preset_CustomizeCar_set_car_instance_if_missing_hook()
{
	_asm {
		test esi, esi
		jnz not_preset
		push ebx // slotNameHash
		push ecx // this
		call fun_car_customize_preset_CustomizeCar_set_car_instance_if_missing
		mov esi, eax
		mov byte ptr [ebp+8+3], 1 // this happens when a new tuned car is created from stock car, so lets do that
not_preset:
		mov cl, byte ptr [ebp+8+3] // overwrote this
		push 0 // overwrote this
		mov eax, 0x552DC0
		jmp eax
	}
}

static
void fun_car_customize_preset()
{
	// partially replace ChangeCategory function to add a new category
	// of preset cars in the car select menu when customizing.
	mkjmp(0x4EED10, fun_car_customize_preset_CarSelectFNGObject__ChangeCategory_hook);

	// hook at the end of UpdateUI so we can set the correct category string, since we added a custom
	// category which the proc doesn't know and will default back to showing "All cars"
	mkjmp(0x4B2855, fun_car_customize_preset_CarSelectFNGObject__UpdateUI_hook);

	// hook CarCollection__CountAvailableCars to return the amount of present cars we have
	// if it was called with the correct flags. Because otherwise it will return 0 (because it
	// doesn't know how to count cars of our custom flag) and default back to "all cars" category.
	mkjmp(0x534850, fun_car_customize_preset_CarCollection__CountAvailableCars_hook);

	// CarCollection__FindCarWithFlagAfterGivenCar
	mkjmp(0x5162D0, fun_car_customize_preset_CarCollection__FindCarWithFlagAfterGivenCar_hook);

	// when the car entries for the menu are actually created, it searches the InventoryCar instance
	// again based on carslothash, so we need to return the preset cars for our custom carslothash
	mkjmp(0x503510, fun_car_customize_preset_CarCollection__GetCarForSlot_hook);

	// when a preset car is selected, create a tuned car instance and apply preset tuning to it
	mkjmp(0x552DBB, fun_car_customize_preset_CustomizeCar_set_car_instance_if_missing_hook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC fun_car_customize_preset
}
