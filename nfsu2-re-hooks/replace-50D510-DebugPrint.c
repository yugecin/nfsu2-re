/*
Some proc that seems to get debug string passed, sometimes with placeholders. The proc is now
a nullsub (xor eax, eax; retn), so sometimes non-strings get passed too.
*/

static
void do50D510Print(int *esp)
{
	int from;
	char *format;
	int b, c, d, e, f, g, h, i;
	int len;

	from = *esp;
	format = (char*) *(esp+1);
	b = *(esp+2);
	c = *(esp+3);
	d = *(esp+4);
	e = *(esp+5);
	f = *(esp+6);
	g = *(esp+7);
	h = *(esp+8);
	i = *(esp+9);
	if (isprocstaticmem((int) format)) {
		sprintf(buf2, format, b, c, d, e, f, g, h, i);
		len = strlen(buf2);
		if (len && buf2[len - 1] == '\n') {
			buf2[len - 1] = 0;
		}
		log(buf, sprintf(buf, "debugstr\t50D510\t%p\t%s", from, buf2));
	}
}

static
__declspec(naked) void hook50D510Print()
{
	_asm {
		mov eax, esp
		pushad
		push eax
		call do50D510Print
		add esp, 0x4
		popad
		xor eax, eax
		ret
	}
}

static
void initHook50D510Print()
{
	mkjmp(0x50D510, &hook50D510Print);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHook50D510Print
}
