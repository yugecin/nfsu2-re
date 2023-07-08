static
void destroy_garage_backdrop()
{
	// replace calls to SomeHashCS43DB50 to return hash of "DEBUG_CUBE"

#define DEBUG_CUBE 0x1786E724
#define DEBUG_LOD_CUBE 0x4847E6A2
#define DISC01 0x56FC31A3
#define DISC02 0x56FC31A4
#define DISC03 0x56FC31A5
#define PLATFORMBASE 0xF3CE89BF
#define BACKDROP 0xA1C35F05
#define BACKDROP_FLOOR 0xE8299D26
#define LOCK 0x0018ED48

#define REPLACEALL DEBUG_CUBE

	// PLATFORMBASE
	*(char*) (0x4EAF46) = 0xB8; // mov eax, xxxxxxxx
	*(int*)  (0x4EAF47) = REPLACEALL;
	// BACKDROP
	*(char*) (0x4EAF5C) = 0xB8; // mov eax, xxxxxxxx
	*(int*)  (0x4EAF5D) = REPLACEALL;
	// DISC01
	*(char*) (0x4EAF75) = 0xB8; // mov eax, xxxxxxxx
	*(int*)  (0x4EAF76) = REPLACEALL;
	// DISC02
	*(char*) (0x4EAF8E) = 0xB8; // mov eax, xxxxxxxx
	*(int*)  (0x4EAF8F) = REPLACEALL;
	// DISC03
	*(char*) (0x4EAFA7) = 0xB8; // mov eax, xxxxxxxx
	*(int*)  (0x4EAFA8) = REPLACEALL;
	// LOCK
	*(char*) (0x4EAFC0) = 0xB8; // mov eax, xxxxxxxx
	*(int*)  (0x4EAFC1) = REPLACEALL;
	// BACKDROP_FLOOR
	*(char*) (0x4EAFD9) = 0xB8; // mov eax, xxxxxxxx
	*(int*)  (0x4EAFDA) = REPLACEALL;

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC destroy_garage_backdrop
}
