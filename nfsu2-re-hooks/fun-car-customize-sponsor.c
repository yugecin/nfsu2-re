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
#define SponsorCar__ApplyTuningToInstance 0x5039D0 /*(int playerIndex, struct MenuCarInstance*, int)*/
#define TunedCar18__CopyTuningFromMenuCarInstance 0x503950 /*(struct MenuCarInstance*)*/

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

static
struct TunedCar*
__stdcall
fun_car_customize_sponsor_CreateTunedCarFromSponsorCar(struct CarCollection *this, unsigned int slotNameHash)
{
	struct MenuCarInstance *menuCarInstance;
	struct SponsorCar *sponsorCar;
	struct TunedCar* tunedCar;
	char buf[32];
	int i;

	i = this->numSponsorCars;
	while (i-- > 0) {
		if (this->sponsorCars[i].__parent.slotHash == slotNameHash) {
			sponsorCar = this->sponsorCars + i;
			// skipping null check on the result of FindCarPreset
			sprintf(buf, "STOCK_%s", FindCarPreset(sponsorCar->carPresetHash)->modelName);
			tunedCar = ThisCallOneArg(CarCollection__CreateNewTunedCarFromFromDataAtSlot, this, (void*) cshash(buf));
			menuCarInstance = (void*) 0x8389D0;
			// (3rd param is MenuCarInstance, using an instance in the data section that
			//  gets overridden later in the Customize process anyways)
			// (4th param is unknown, but unused in SponsorCar's ApplyTuning
			ThisCallThreeArgs(SponsorCar__ApplyTuningToInstance, sponsorCar, (void*) profileData->currentPlayerIndex, menuCarInstance, 0);
			// copy tuning back from MenuCarInstance to tuned car instance
			ThisCallOneArg(TunedCar18__CopyTuningFromMenuCarInstance, &tunedCar->field_18, menuCarInstance);
			return tunedCar;
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
		call fun_car_customize_sponsor_CreateTunedCarFromSponsorCar
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
	// when a sponsor car is selected, create a tuned car instance and apply sponsor tuning to it
	mkjmp(0x552DBB, fun_car_customize_sponsor_set_car_instance_if_missing);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC fun_car_customize_sponsor
}
