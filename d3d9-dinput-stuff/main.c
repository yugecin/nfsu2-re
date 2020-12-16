#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <dinput.h>
#include <d3d9.h>

int main()
{
	printf("x %x\n", D3DRS_SHADEMODE);
	printf("x %x\n", D3DSHADE_FLAT);
	printf("x %x\n", D3DZB_FALSE);
	printf("FIONBIO %p\n", FIONBIO);
	printf("DIERR_INVALIDPARAM %p\n", DIERR_INVALIDPARAM);
	printf("DIERR_NOTINITIALIZED %p\n", DIERR_NOTINITIALIZED);
	printf("DIERR_OTHERAPPHASPRIO %p\n", DIERR_OTHERAPPHASPRIO);
	printf("S_FALSE  %p\n", S_FALSE);
	printf("DISCL_BACKGROUND %p\n", DISCL_BACKGROUND);
	printf("DISCL_EXCLUSIVE %p\n", DISCL_EXCLUSIVE);
	printf("DISCL_FOREGROUND %p\n", DISCL_FOREGROUND);
	printf("DISCL_NONEXCLUSIVE %p\n", DISCL_NONEXCLUSIVE);
	printf("DISCL_NOWINKEY %p\n", DISCL_NOWINKEY);
	printf("DI_OK %p\n", DI_OK);
	printf("c_dfDIKeyboard %p\n", c_dfDIKeyboard);
	printf("c_dfDIMouse %p\n", c_dfDIMouse);
	printf("c_dfDIMouse2 %p\n", c_dfDIMouse2);
	printf("c_dfDIJoystick %p\n", c_dfDIJoystick);
	printf("c_dfDIJoystick2 %p\n", c_dfDIJoystick2);
	return 1;
}