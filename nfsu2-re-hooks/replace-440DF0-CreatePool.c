static
struct Pool *CreatePool(int elementSize, int elementAmount, char *name, int unknown)
{
	struct Pool *pool;
	struct PoolLink *linkNext;

	elementSize += 3;
	elementSize -= elementSize % 4;
	log(buf, sprintf(buf, "POOL:NEW '%s' size %d amt %d unk %08X",
		name, elementSize, elementAmount, unknown));
	pool = ((struct Pool*(__cdecl *)(int,int,char*,int))0x440B40)
			(elementSize, elementAmount, name, unknown);

	if (pool && pools->inited == 1) {
		linkNext = pools->link.next;
		linkNext->prev = &pool->__parent;
		pools->link.next = &pool->__parent;
		pool->__parent.next = linkNext;
		pool->__parent.prev = &pools->link;
	}

	return pool;
}

static
void initCreatePoolReplace()
{
	mkjmp(0x440DF0, &CreatePool);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initCreatePoolReplace
}
