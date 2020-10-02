int actualReadFile;
int actualWriteFile;
int actualCreateFileA;

static
void ReadFileHookPrint(
	int ignore,
	HANDLE hFile,
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToRead,
	LPDWORD lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped)
{
	char path[2000];

	GetFinalPathNameByHandleA(hFile, path, sizeof(path), 0);
	log(buf, sprintf(buf, "ReadFile(\"%s\")", path));
}

static
__declspec(naked) void ReadFileHook()
{
	_asm {
		call ReadFileHookPrint
		mov eax, actualReadFile
		jmp eax
	}
}

static
void WriteFileHookPrint(
	int ignore,
	HANDLE hFile,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped)
{
	char path[2000];

	GetFinalPathNameByHandleA(hFile, path, sizeof(path), 0);
	log(buf, sprintf(buf, "WriteFile(\"%s\")", path));
}

static
__declspec(naked) void WriteFileHook()
{
	_asm {
		call WriteFileHookPrint
		mov eax, actualWriteFile
		jmp eax
	}
}

static
void CreateFileAHookPrint(
	int ignore,
	LPCSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES
	lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	log(buf, sprintf(buf, "CreateFileA(\"%s\", access=%X)", lpFileName, dwDesiredAccess));
}

static
__declspec(naked) void CreateFileAHook()
{
	_asm {
		call CreateFileAHookPrint
		mov eax, actualCreateFileA
		jmp eax
	}
}

static
void initReadFileHook()
{
#define hookfileapi(addr,variable,funcaddr) variable=*(int*)addr;*(int*)addr=(int)funcaddr;
	//hookfileapi(0x783234, actualReadFile, &ReadFileHook);
	//hookfileapi(0x783224, actualWriteFile, &WriteFileHook);
	hookfileapi(0x7831D0, actualCreateFileA, &CreateFileAHook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initReadFileHook
}
