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

#define CarSelectFNGObject__UpdateRankedModeLabel 0x497F20 /*(struct OnlineCar *onlineCar)*/
#define CarSelectFNGObject__ResetBrowableCars 0x4EEC90 /*()*/
#define CarSelectFNGObject__UpdateUI 0x4B2310 /*()*/
#define CarCollection__GetCarForSlot 0x503510 /*(unsigned int slotHash)*/

static
void Replace_CarSelectFNGObject__ChangeCategory(struct CarSelectFNGObject *this, void *_, unsigned int message)
{
#define MSG_PREV 0x5073EF13
#define MSG_NEXT 0xD9FEEC59

	enum INVENTORY_CAR_FLAGS newCarSelectCategory = *carSelectCategory;
	struct UIElement *uiElement;
	struct OnlineCar *onlineCar;

	// figure out new category based on current menu and button pressed and previous category
	switch (profileData->menuState) {
	case MENU_STATE_MAIN_MENU: // car select from quick race menu
	case MENU_STATE_4:
		if (message == MSG_PREV) {
			// stock|tuned -> (sponsor ->) (career ->) (tuned ->) stock -> (loop)
			switch (newCarSelectCategory) {
			case IS_STOCK_CAR | IS_TUNED_CAR:
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
			// stock|tuned -> stock -> (tuned ->) (career ->) (sponsor ->) (loop)
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
				newCarSelectCategory = IS_STOCK_CAR | IS_TUNED_CAR;
				break;
			}
		}
		break;
	case MENU_STATE_CAR_CUSTOMIZE:
		if (message == MSG_PREV) {
			// stock|tuned -> (tuned ->) stock -> (loop)
			switch (newCarSelectCategory) {
			case IS_STOCK_CAR | IS_TUNED_CAR:
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
			// stock|tuned -> stock -> (tuned ->) (loop)
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
				newCarSelectCategory = IS_STOCK_CAR | IS_TUNED_CAR;
				break;
			}
		}
		break;
	case MENU_STATE_ONLINE_MAIN_MENU:
	case MENU_STATE_LAN_MAIN_MENU:
		if (message == MSG_PREV) {
			// stock|tuned -> (career ->) online -> (tuned ->) stock -> (loop)
			switch (newCarSelectCategory) {
			case IS_STOCK_CAR | IS_TUNED_CAR:
				if (CarSelectFNGObject__CountAvailableCars(this, IS_CAREER_CAR)) {
					newCarSelectCategory = IS_CAREER_CAR;
					break;
				}
			case IS_CAREER_CAR:
				newCarSelectCategory = IS_ONLINE_CAR;
				break;
			case IS_ONLINE_CAR:
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
			// stock|tuned -> stock -> (tuned ->) online -> (career ->) (loop)
			switch (newCarSelectCategory) {
			case IS_STOCK_CAR | IS_TUNED_CAR:
				newCarSelectCategory = IS_STOCK_CAR;
			case IS_STOCK_CAR:
				if (CarSelectFNGObject__CountAvailableCars(this, IS_TUNED_CAR)) {
					newCarSelectCategory = IS_TUNED_CAR;
					break;
				}
			case IS_TUNED_CAR:
				newCarSelectCategory = IS_ONLINE_CAR;
				break;
			case IS_ONLINE_CAR:
				if (CarSelectFNGObject__CountAvailableCars(this, IS_CAREER_CAR)) {
					newCarSelectCategory = IS_CAREER_CAR;
					break;
				}
			case IS_CAREER_CAR:
				newCarSelectCategory = IS_STOCK_CAR | IS_TUNED_CAR;
				break;
			}
		}
		break;
	}

	// update UI if new category is different from current
	if (newCarSelectCategory != *carSelectCategory) {
		*carSelectCategory = newCarSelectCategory;
		profileData->players[profileData->currentPlayerIndex].d4.currentCarSelectionCategory = newCarSelectCategory;
		ThisCallNoArgs(CarSelectFNGObject__ResetBrowableCars, this);
		ThisCallNoArgs(CarSelectFNGObject__UpdateUI, this);

		uiElement = FindUIElementByHash(this->__parent.fngName, cihash("OL_CarMode_Group"));
		if (newCarSelectCategory == IS_ONLINE_CAR || !strcmp(this->__parent.fngName, "UI_OLCarLot.fng")) {
			ShowNullableUIElementAndChildren(uiElement);
			// In customize & quick race car select, the OL_CarMode_Group still won't show because
			// its animation will still be "HIDE". Does the UI_OLCarLot.fng screen set it to "FadeIn"
			// or "SHOW" on load perhaps? So adding this next line to counteract this:
			SetUIElementAnimationByName(uiElement, "SHOW", 0);
			/*this takes player1's car collection even though some lines before it uses currentPlayerIndex...*/
			/*But maybe it makes sense in that only player1 can play online?*/
			onlineCar = ThisCallOneArg(CarCollection__GetCarForSlot, &profileData->players[0].carCollection, (void*) this->currentSelectedCar->slotHash);
			ThisCallOneArg(CarSelectFNGObject__UpdateRankedModeLabel, this, onlineCar);
		} else {
			HideNullableUIElementAndChildren(uiElement);
		}

		if (profileData->menuState == MENU_STATE_ONLINE_MAIN_MENU ||
			profileData->menuState == MENU_STATE_CUSTOMIZE_FROM_ONLINE_MAIN_MENU)
		{
			// These elements are in a bar that, I assume, is the console equivalent of the pc help bar.
			// That bar is rendered off-screen in my game, and it has elements for "player name" (which
			// is literally that, it's not set to my player name), "customize" and "help".
			// The following hide/show statements ensure that the "customize" button is hidden when a
			// career car is selected from the online car select menu.
			// It makes sense, as customizing a career car shouldn't be allowed except in career, because
			// otherwise you can buy items without paying. But why is the condition for hiding this only
			// checking online menu states, because one can also select career cars from the quick race menu?
			// Either way this seems redundant, as some other place ensures that the customize button (both
			// on pc help bar and this console bar) is hidden when selecting a career or sponsor car.
			// 0xA936C3A2 is label "$JOY_EVENT_FENG_BUTTON1$"
			// 0x89782734 is label "Edit Customized Car" (or "Customize in QR) (but is also used for "Delete Tuned Car")
			if (newCarSelectCategory == IS_CAREER_CAR) {
				HideNullableUIElementAndChildren(FindUIElementByHash(this->__parent.fngName, 0xA936C3A2));
				HideNullableUIElementAndChildren(FindUIElementByHash(this->__parent.fngName, 0x89782734));
			} else {
				ShowNullableUIElementAndChildren(FindUIElementByHash(this->__parent.fngName, 0xA936C3A2));
				ShowNullableUIElementAndChildren(FindUIElementByHash(this->__parent.fngName, 0x89782734));
			}
		}
	}
}

static
__declspec(naked)
void Replace_CarSelectFNGObject__ChangeCategory_hook(unsigned int message)
{
	_asm {
		push ecx
		call Replace_CarSelectFNGObject__ChangeCategory
		add esp, 4
		retn 0x4
	}
}

static
void initReplaceCarSelectFNGObject__ChangeCategory()
{
	mkjmp(0x4EED10, &Replace_CarSelectFNGObject__ChangeCategory_hook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initReplaceCarSelectFNGObject__ChangeCategory
}
