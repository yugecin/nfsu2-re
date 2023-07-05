static
void debug_custom_iterate_carpresets(int wparam)
{
	struct CarPreset *entry;
	struct ObjectLink *sentinel, *next;

	if (wparam == 121) { // y
		sentinel = (struct ObjectLink*) 0x8A31E4;
		next = sentinel->next;
		while (next != sentinel) {
			entry = (void*) next;
			printf("preset car entry\n");
			printf("  modelName %s\n", entry->modelName);
			printf("  preset name %s (@%X)\n", entry->name, &entry->name);
			next = next->next;
		}
	}

	DEBUG_WMCHAR_FUNC();
}
#undef DEBUG_WMCHAR_FUNC
#define DEBUG_WMCHAR_FUNC debug_custom_iterate_carpresets
