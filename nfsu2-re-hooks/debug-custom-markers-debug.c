static char do_markers_debug;

static
float get_distance(float *pos, struct Marker *marker)
{
	float dx, dy;

	dx = pos[0] - marker->pos_x;
	dy = pos[1] - marker->pos_y;
	return (float) sqrt(dx * dx + dy * dy);
}

static
struct Marker *find_nearest(float *pos)
{
	struct Marker *last;
	struct Marker *closest;
	float closest_distance;
	float dist;

	closest_distance = 100000.0f;
	closest = 0;
	last = 0;
	for (;;) {
		// 0x13 = money pickups
		last = Markers__FindAtPosWithTypeAfterIndex(0, 0x13, last);
		if (last) {
			dist = get_distance(pos, last);
			if (dist < closest_distance) {
				closest_distance = dist;
				closest = last;
			}
		} else {
			break;
		}
	}
	return closest;
}

static
float *getCarPos()
{
	int* ptr;

	ptr = *(int**) 0x8900B4;
	if (ptr) {
		ptr = (int*) (*(ptr + 1) + 0x60);
	}
	return (void*) ptr;
}

static
void debug_custom_markers_debug_doui(int wparam)
{
	float *pos;
	struct Marker *nearest;
	float dist;
	int len;
	char mbuf[300];

	if (do_markers_debug) {
		pos = getCarPos();
		if (!pos) {
			SetUILabelByHashFormattedString("ScreenPrintf.fng", 0xB9D45799, "%s", "no car pos?");
		} else {
			nearest = find_nearest(pos);
			len = sprintf(mbuf, "my pos %.1f %.1f %.1f^nearest %p",
				*pos, *(pos + 1), *(pos + 2), nearest);
			if (nearest) {
				dist = get_distance(pos, nearest);
				sprintf(mbuf + len, "^pos %.1f %.1f^dist %.1f",
					nearest->pos_x, nearest->pos_y, dist);
			}
			SetUILabelByHashFormattedString("ScreenPrintf.fng", 0xB9D45799, "%s", mbuf);
		}
	}

	DEBUG_DOUI_FUNC();
}

static
void debug_custom_markers_debug_wmchar(int wparam)
{
	if (wparam == 121) { // y
		AddFngToUIObject_1("ScreenPrintf.fng", 0);
		do_markers_debug = !do_markers_debug;
	}

	DEBUG_WMCHAR_FUNC(wparam);
}
#undef DEBUG_WMCHAR_FUNC
#define DEBUG_WMCHAR_FUNC debug_custom_markers_debug_wmchar
#undef DEBUG_DOUI_FUNC
#define DEBUG_DOUI_FUNC debug_custom_markers_debug_doui
