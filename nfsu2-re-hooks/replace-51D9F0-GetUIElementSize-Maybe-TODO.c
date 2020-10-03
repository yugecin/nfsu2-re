struct in51D9F0 {
	int d0;
	int d4;
	int d8;
	int dC;
	int d10;
	int d14;
	int type;
	int d1C;
	int d20;
	int d24;
	int d28;
	int d2C;
	int d30;
	int d34;
	int d38;
	int d3C;
	int d40;
	int d44;
	int d48;
	int d4C;
	int d50;
	int d54;
	int d58;
	int d5C;
	int d60;
	int d64;
};

struct out51D9F0 {
	float maybeX;
	float maybeY;
	float maybeWidth;
	float maybeHeight;
};

static
int Sub51D9F0GetUIElementSizeReplace(struct in51D9F0 *in, struct out51D9F0 *out)
{
	if (!in) {
		return 0;
	}

	switch (in->type) {
	case 0:
	case 6:
	case 8:
	case 9:
	case 10:
		/*TODO 51DA23 this is big*/
		return 1;
	case 4:
		if (!in->d64) {
			return 0;
		}
		/*TODO 51DA5A*/
		return 1;
	case 1:
		/*TODO 51DB21*/
		return 1;
	case 2:
	case 3:
	case 5:
		/*TODO 51DB87*/
		return 1;
	}
	return 0;
}

static
void initSub51D9F0GetUIElementSizeReplace()
{
	mkjmp(0x51D9F0, &Sub51D9F0GetUIElementSizeReplace);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initSub51D9F0GetUIElementSizeReplace
}
