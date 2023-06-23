static
void debug_custom_iterate_somethingUI(int wparam)
{
	struct ObjectLink *link, *next;
	struct SomethingUIImpl *impl;

	if (wparam == 121) { // y
		link = (struct ObjectLink*) 0x8637B4;
		next = link->next;
		while (next && next != link) {
			impl = (struct SomethingUIImpl*)((int) next - 4);
			if (impl->fngData) {
				printf("%08X: fngdata %s\n", cshash(impl->fngData->name), impl->fngData->name);
			} else {
				if (impl->binsection->header.magic == 0x30203) {
					printf("%08X: FNGNAME %s\n", cshash(impl->data30203->fngName), impl->data30203->fngName);
				} else {
					printf("%08X: HASH\n", *((int*)&impl->binsection->data));
				}
			}
			next = next->next;
		}
	}

	DEBUG_WMCHAR_FUNC();
}
#undef DEBUG_WMCHAR_FUNC
#define DEBUG_WMCHAR_FUNC debug_custom_iterate_somethingUI
