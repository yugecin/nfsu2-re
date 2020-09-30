/*
511E60 GetLogoForCarModel

Reimplementing because why not
*/

static
void GetLogoForCarModel(int car_model_index)
{
	struct CarModelInfo *car_model_info;

	car_model_info = (*_car_model_infos) + car_model_index;
	log(buf, sprintf(buf,
		"car idx %d at %p brand %s manufacturer %s",
		car_model_index,
		car_model_info,
		car_model_info->brand,
		car_model_info->manufacturer));
}

static
__declspec(naked) int GetLogoForCarModelHook(int car_model_index, char get_manufacturer)
{
	_asm {
		push [esp+0x4]
		call GetLogoForCarModel
		add esp, 0x4
		mov edx, 0x8A1CCC
		mov edx, [edx]
		push 0x511E66
		ret
	}
}

static
void initReplaceGetLogoForCarModel()
{
	mkjmp(0x511E60, &GetLogoForCarModelHook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initReplaceGetLogoForCarModel
}
