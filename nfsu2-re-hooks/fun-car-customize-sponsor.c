// -----------------------------
// Allows selecting sponsor cars in customization menu

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

// copied from replace-4EED10-CarSelectFNGObject__ChangeCategory.c and stripped to just do what we need
static
int fun_car_customize_sponsor_CarSelectFNGObject__ChangeCategory(struct CarSelectFNGObject *this, void *_, unsigned int message)
{
#define MSG_PREV 0x5073EF13
#define MSG_NEXT 0xD9FEEC59

	enum INVENTORY_CAR_FLAGS newCarSelectCategory = *carSelectCategory;

	if (profileData->menuState == MENU_STATE_CAR_CUSTOMIZE) {
		if (message == MSG_PREV) {
			// stock|tuned -> (sponsor ->) (tuned ->) stock -> (loop)
			switch (newCarSelectCategory) {
			case IS_STOCK_CAR | IS_TUNED_CAR:
				if (CarSelectFNGObject__CountAvailableCars(this, IS_SPONSOR_CAR)) {
					newCarSelectCategory = IS_SPONSOR_CAR;
					break;
				}
			case IS_SPONSOR_CAR:
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
			// stock|tuned -> stock -> (tuned ->) (sponsor ->) (loop)
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
				if (CarSelectFNGObject__CountAvailableCars(this, IS_SPONSOR_CAR)) {
					newCarSelectCategory = IS_SPONSOR_CAR;
					break;
				}
			case IS_SPONSOR_CAR:
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
void fun_car_customize_sponsor_CarSelectFNGObject__ChangeCategory_hook(unsigned int message)
{
	_asm {
		push ecx
		call fun_car_customize_sponsor_CarSelectFNGObject__ChangeCategory
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
struct CarPreset*
FindCarPreset(unsigned int nameHash)
{
	_asm { mov eax, 0x61C460 }
	_asm { jmp eax }
}

struct SponsorCar *fun_car_customize_sponsor_stored_sponsorcar;

static
struct TunedCar*
__stdcall
fun_car_customize_sponsor_GetSponsorCar(struct CarCollection *this, unsigned int slotNameHash)
{
	char buf[32];
	int i;

	i = this->numSponsorCars;
	while (i-- > 0) {
		if (this->sponsorCars[i].__parent.slotHash == slotNameHash) {
			fun_car_customize_sponsor_stored_sponsorcar = this->sponsorCars + i;
			// skipping null check on the result of FindCarPreset
			sprintf(buf, "STOCK_%s", FindCarPreset(this->sponsorCars[i].carPresetHash)->modelName);
			return ThisCallOneArg(CarCollection__CreateNewTunedCarFromFromDataAtSlot, this, (void*) cshash(buf));
		}
	}
	return NULL;
}

static
__declspec(naked)
void
fun_car_customize_sponsor_set_car_instance_if_missing()
{
	_asm {
		test esi, esi
		jnz allgood
		// oh yey, no car instance, we're probably trying to customize a sponsor car. LEHDOTHIS
		// only need to retain esi here (value in ebx is not used from the hooked point)
		push ebx // slotNameHash
		push ecx // this
		call fun_car_customize_sponsor_GetSponsorCar
		mov esi, eax
		mov byte ptr [ebp+8+3], 1 // this happens when a new tuned car is created from stock car, so lets do that
allgood:
		mov cl, byte ptr [ebp+8+3] // overwrote this
		push 0 // overwrote this
		mov eax, 0x552DC0
		jmp eax
	}
}

static
__declspec(naked)
void
fun_car_customize_sponsor_apply_sponsor_tuning()
{
	_asm {
		mov eax, [fun_car_customize_sponsor_stored_sponsorcar]
		test eax, eax
		jz wasnotsponsorcar
		//push 0 // already pushed
		//push 0x8392C0 // already pushed
		push edi // playerIndex
		mov ecx, [fun_car_customize_sponsor_stored_sponsorcar]
		mov eax, 0x5039D0 // vtable func C for sponsor cars
		call eax
		// 2nd car instance
		push 0
		push 0x8389D0
		push edi
		mov ecx, [fun_car_customize_sponsor_stored_sponsorcar]
		mov eax, 0x5039D0 // vtable func C for sponsor cars
		call eax

		// copy tuning back from the car instance to the tuned car entry
		push 0x8389D0 // car instance
		lea ecx, [esi+0x18]
		mov eax, 0x503950
		call eax

		// reset state (so we don't do this again when another car is selected) and rt
		mov dword ptr [fun_car_customize_sponsor_stored_sponsorcar], 0
		mov eax, 0x552DF2
		jmp eax
wasnotsponsorcar:
		// do what we overwrote and jmp back
		mov edx, [esi]
		push edi
		mov ecx, esi
		mov eax, 0x552DDC
		jmp eax
	}
}

static
int
__stdcall
fun_car_customize_sponsor_replace_CheatScreenData__IsSponsorCarCheatTriggered(unsigned int *sponsorCarHash)
{
	// easy way to make all sponsor cars available :D
	return 1;
}

static
void fun_car_customize_sponsor()
{
	// partially replace ChangeCategory function so the "SPONSOR CARS" category can be selected in customize menu
	mkjmp(0x4EED10, fun_car_customize_sponsor_CarSelectFNGObject__ChangeCategory_hook);

	// make all sponsor cars available by pretending to have triggered all of the sponsor cheats
	mkjmp(0x579D70, fun_car_customize_sponsor_replace_CheatScreenData__IsSponsorCarCheatTriggered);
	// codez so the game doesn't crash when continuing with a sponsor car, because it's not supported
	mkjmp(0x552DBB, fun_car_customize_sponsor_set_car_instance_if_missing);
	// codez so the sponsor tuning is applied to the tuned car that is created from the stock car
	mkjmp(0x552DD7, fun_car_customize_sponsor_apply_sponsor_tuning);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC fun_car_customize_sponsor
}
