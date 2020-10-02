static
void RCFS_ReadFileWrapperPrint(HANDLE hFile)
{
	char path[2000];

	GetFinalPathNameByHandleA(hFile, path, sizeof(path), 0);
	log(buf, sprintf(buf, "realcoreFS: ReadFileWrapper(\"%s\")", path));
}

static
__declspec(naked) void RCFS_ReadFileWrapper()
{
	_asm {
		push ecx
		push [esp+0x8]
		call RCFS_ReadFileWrapperPrint
		pop ecx
		pop ecx
		push ebp
		mov ebp, esp
		push 0
		mov eax, 0x6FAE25
		jmp eax
	}
}

static
void RCFS_WriteFileWrapperPrint(HANDLE hFile)
{
	char path[2000];

	GetFinalPathNameByHandleA(hFile, path, sizeof(path), 0);
	log(buf, sprintf(buf, "realcoreFS: WriteFileWrapper(\"%s\")", path));
}

static
__declspec(naked) void RCFS_WriteFileWrapper()
{
	_asm {
		push [esp+0x4]
		call RCFS_WriteFileWrapperPrint
		pop ecx
		push ebp
		mov ebp, esp
		push 0
		mov eax, 0x6FAE44
		jmp eax
	}
}

static
void RCFS_SetFilePointerWrapperPrint(HANDLE hFile, int distance, int movemethod)
{
	char path[2000];

	GetFinalPathNameByHandleA(hFile, path, sizeof(path), 0);
	log(buf, sprintf(buf, "realcoreFS: SetFilePointerWrapper(\"%s\", distance=%d from %s(method %d))",
		path,
		distance,
		movemethod == 1 ? "current" : "SOF",
		movemethod));
}

static
__declspec(naked) void RCFS_SetFilePointerWrapper()
{
	_asm {
		push ebp
		mov ebp, esp
		push [ebp+0x14]
		push [ebp+0xC]
		push [ebp+0x8]
		call RCFS_SetFilePointerWrapperPrint
		add esp, 0xC
		mov eax, [ebp+0x14]
		mov eax, 0x6FAE64
		jmp eax
	}
}

static
void RCFS_GetFileSizeWrapperPrint(HANDLE hFile)
{
	char path[2000];

	GetFinalPathNameByHandleA(hFile, path, sizeof(path), 0);
	log(buf, sprintf(buf, "realcoreFS: GetFileSizeWrapper(\"%s\")", path));
}

static
__declspec(naked) void RCFS_GetFileSizeWrapper()
{
	_asm {
		push [esp+0x4]
		call RCFS_GetFileSizeWrapperPrint
		add esp, 0x4
		push ebp
		mov ebp, esp
		push 0
		mov eax, 0x6FAE8E
		jmp eax
	}
}

static
void RCFS_DeleteFileAWrapperPrint(HANDLE hFile)
{
	char path[2000];

	GetFinalPathNameByHandleA(hFile, path, sizeof(path), 0);
	log(buf, sprintf(buf, "realcoreFS: DeleteFileAWrapper(\"%s\")", path));
}

static
__declspec(naked) void RCFS_DeleteFileAWrapper()
{
	_asm {
		push [esp+0x4]
		call RCFS_DeleteFileAWrapperPrint
		add esp, 0x4
		push ebp
		mov ebp, esp
		push [ebp+0x8]
		mov eax, 0x6FAEA3
		jmp eax
	}
}

static
void RCFS_CloseHandleWrapperPrint(HANDLE hFile)
{
	char path[2000];

	GetFinalPathNameByHandleA(hFile, path, sizeof(path), 0);
	log(buf, sprintf(buf, "realcoreFS: CloseHandleWrapper(\"%s\")", path));
}

static
__declspec(naked) void RCFS_CloseHandleWrapper()
{
	_asm {
		push [esp+0x4]
		call RCFS_CloseHandleWrapperPrint
		add esp, 0x4
		push ebp
		mov ebp, esp
		pop ebp
		mov eax, 0x783144
		mov eax, [eax]
		jmp eax
	}
}

static
void RCFS_FindFirstFileAWrapperPrint(char *lpFileName)
{
	log(buf, sprintf(buf, "realcoreFS: FindFirstFileA(\"%s\")", lpFileName));
}

static
__declspec(naked) void RCFS_FindFirstFileAWrapper()
{
	_asm {
		push [esp+0x4]
		call RCFS_FindFirstFileAWrapperPrint
		add esp, 0x4
		push ebp
		sub esp, 0x140
		mov eax, 0x6FAF27
		jmp eax
	}
}

static
void initRealCoreFileSystem()
{
	mkjmp(0x6FAE20, &RCFS_ReadFileWrapper);
	mkjmp(0x6FAE3F, &RCFS_WriteFileWrapper);
	mkjmp(0x6FAE5E, &RCFS_SetFilePointerWrapper);
	mkjmp(0x6FAE89, &RCFS_GetFileSizeWrapper);
	mkjmp(0x6FAE9D, &RCFS_DeleteFileAWrapper);
	mkjmp(0x6FAE16, &RCFS_CloseHandleWrapper);
	mkjmp(0x6FAF1E, &RCFS_FindFirstFileAWrapper);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initRealCoreFileSystem
}
