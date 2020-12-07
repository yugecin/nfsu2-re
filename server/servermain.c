#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <windows.h>
#pragma pack(push,1)

#define STATIC_ASSERT(E) typedef char __static_assert_[(E)?1:-1]
#define EXPECT_SIZE(S,SIZE) STATIC_ASSERT(sizeof(S)==(SIZE))
#define MEMBER_OFFSET(S,M) (int)&((S*)NULL)->M
#define ASSERT_OFFSET(STRUCT,MEMBER,OFFSET) STATIC_ASSERT(MEMBER_OFFSET(STRUCT,MEMBER)==OFFSET)

static
void *procStartServer, *procStopServer, *procIsServerRunning;

char data[100000];

static
int IsServerRunning()
{
	return ((int(__cdecl *)(void))procIsServerRunning)();
}

static
int StartServer(void *data, int gameRegion, int arg8, int argC)
{
	return ((int(__cdecl *)(void*,int,int,int))procStartServer)(data, gameRegion, arg8, argC);
}

static
int StopServer()
{
	return ((int(__cdecl *)(void))procStopServer)();
}

int main(int argc, char **argv)
{
	HMODULE serverlib;

	serverlib = LoadLibraryA("server.dll");
	if (!serverlib) {
		printf("Failed to load server.dll\n");
		return 1;
	}

	procStartServer = GetProcAddress(serverlib, "StartServer");
	procStopServer = GetProcAddress(serverlib, "StopServer");
	procIsServerRunning = GetProcAddress(serverlib, "IsServerRunning");
	if (!procStartServer || !procStopServer || !procIsServerRunning) {
		printf("missings procs\n");
		printf("StartServer %p StopServer %p IsServerRunning %p\n",
			procStartServer, procStopServer, procIsServerRunning);
		FreeLibrary(serverlib);
		return 1;
	}

	printf("Server running: %d\n", IsServerRunning());
	printf("StartServer result: %d\n", StartServer(&data, 0, 0, 0));

	fgetc(stdin);
	printf("hey\n");

	FreeLibrary(serverlib);
	return 0;
}
