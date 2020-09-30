/*
Only seen calling stuff like this so far:

SomethingWithAFile E:\bin.dat
SomethingWithAFile E:\bin.dat
SomethingWithAFile speech/UG2_Speech.big
SomethingWithAFile CARS\MUSTANGGT\VINYLS.BIN
SomethingWithAFile CARS\MUSTANGGT\PARTS_ANIMATIONS.BIN
SomethingWithAFile CARS\PEUGOT\GEOMETRY.BIN
SomethingWithAFile speech/UG2_Speech.big
SomethingWithAFile Tracks\RoutesL4RA\Paths4001.bin
SomethingWithAFile Tracks\RoutesL4RA\Routes4001F.bin
SomethingWithAFile CARS\MUSTANGGT\VINYLS.BIN
SomethingWithAFile NIS\Scene_IntroNis08_EvtTrk.txt
*/

static
void SomethingWithABinFile(int returnaddr, char *filename)
{
	log(buf, sprintf(buf, "SomethingWithABinFile %s", filename));
}

static
__declspec(naked) void SomethingWithABinFileHook()
{
	_asm {
		call SomethingWithABinFile
		mov eax, [esp+0x4]
		push 0
		push 1
		push eax
		mov eax, 0x57CA10
		push 0x57CACE
		jmp eax
	}
}

static
void initSomethingWithABinFileHook()
{
	mkjmp(0x57CAC0, &SomethingWithABinFileHook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initSomethingWithABinFileHook
}
