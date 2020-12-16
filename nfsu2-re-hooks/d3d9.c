static
void *d3device9_vtablefunc(int vtable_entry)
{
	int *device;
	int *vtable;

	device = *d3device9;
	vtable = *(int**) device;
	return *(int**) (vtable + vtable_entry);
}

static
void d3device9_SetRenderState(int state, int value)
{
	void *func;

	func = d3device9_vtablefunc(57);
	((void (__stdcall *)(void*,int,int))func)(*d3device9, state, value);
}
