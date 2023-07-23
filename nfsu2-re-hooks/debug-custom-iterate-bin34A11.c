static
void debug_custom_iterate_bin34A11(int wparam)
{
	int ptr, num;
	short offset;
	char *careerStringPool;

	if (wparam == 121) { // y
		careerStringPool = *(char**) 0x838428;
		num = *(int*) 0x8383E4;
		ptr = *(int*) 0x8383E0;
		while (num-- > 0) {
			offset = *(short*)(ptr + 4);
			printf("%08X: %s\n", *(unsigned int*)(ptr + 0x8), careerStringPool + offset);
			ptr += 0x88;
		}
	}

	DEBUG_WMCHAR_FUNC();
}
#undef DEBUG_WMCHAR_FUNC
#define DEBUG_WMCHAR_FUNC debug_custom_iterate_bin34A11
