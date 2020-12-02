static
void __stdcall PoolExtend(struct Pool *this, int elementAmount)
{
	struct Pool *ext;
	struct Pool *lastPool;
	char *ptr;

	log(buf, sprintf(buf, "POOL:EXT '%s' size %d amt %d + %d unk %08X",
		this->name, this->elementSize, this->elementAmount,
		elementAmount, this->field_20));
	ext = ((struct Pool*(__cdecl *)(int,int,char*,int))0x440B40)
			(this->elementSize, elementAmount, this->name, this->field_20);
	ext->flagsMaybe = this->flagsMaybe;

	lastPool = this;
	while (lastPool->nextLinkedPool) {
		lastPool = lastPool->nextLinkedPool;
	}
	lastPool->nextLinkedPool = ext;

	ptr = (char*) ext + sizeof(struct Pool); /*offset of first element*/
	ptr += this->elementSize * (elementAmount - 1); /*now offset of last element*/
	((struct PoolEntry*) ptr)->nextEntry = this->firstElement;

	this->firstElement = ext->firstElement;
	ext->firstElement = NULL;

	this->elementAmountOverAllLinkedPools += elementAmount;
}

static
__declspec(naked) void PoolExtendReplace(int elementAmount)
{
	_asm {
		push [esp+4]
		push ecx
		call PoolExtend
		pop eax
		pop ecx /*arg0, thiscall*/
		jmp eax
	}
}

static
void initPoolExtendReplace()
{
	mkjmp(0x440BB0, &PoolExtendReplace);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initPoolExtendReplace
}
