struct Vert {
	float x, y, z;
	int dwReserved;
	unsigned int col; // color;
	unsigned int spec; // specular
	float u, v;
};

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

static
void d3device9_SetTextureStageState(int stage, int type, int value)
{
	void *func;

	func = d3device9_vtablefunc(67);
	((void (__stdcall *)(void*,int,int,int))func)(*d3device9,stage,type,value);
}

static
void d3device9_DrawPrimitiveUP(int type, int count, void *data)
{
	int size;
	void *func;

	func = d3device9_vtablefunc(83);
	size = sizeof(struct Vert);
	((void (__stdcall *)(void*,int,int,void*,int))func)(*d3device9,type,count,data,size);
}

static
void d3device9_SetFVF(int fvf)
{
	void *func;

	func = d3device9_vtablefunc(89);
	((void (__stdcall *)(void*,int))func)(*d3device9,fvf);
}

static
void d3device9_GetFVF(int *store)
{
	void *func;

	func = d3device9_vtablefunc(90);
	((void (__stdcall *)(void*,int*))func)(*d3device9,store);
}

static
void d3d9_draw_line_strip(struct Vert *verts, int num_lines)
{
	int fvf;

	d3device9_GetFVF(&fvf);
	d3device9_SetFVF(482 /*D3DFVF_LVERTEX*/);
	d3device9_SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	d3device9_SetRenderState(D3DRS_CLIPPING, 0);
	d3device9_SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
	d3device9_SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
	d3device9_DrawPrimitiveUP(D3DPT_LINESTRIP, num_lines, verts);
	d3device9_SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	d3device9_SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	d3device9_SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	d3device9_SetRenderState(D3DRS_CLIPPING, 1);
	d3device9_SetFVF(fvf);
}
