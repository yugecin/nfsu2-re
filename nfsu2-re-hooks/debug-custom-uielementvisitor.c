static
void debug_custom_uielementvisitor_visit_element(struct UIElement *element, char *prefix)
{
	struct UIElement *containerchild;
	struct UILabel *label;
	char locbuf[100];
	char locbuf2[100];

	sprintf(locbuf, "%s    ", prefix);
	while (element) {
		locbuf2[0] = 0;
		if (element->type == 2) {
			label = (void*) element;
			if (label->string.ptrString) {
				locbuf2[0] = ' ';
				wchar2char(locbuf2 + 1, label->string.ptrString);
			}
		}
		//element->someFlags |= 0x2; // set visible
		//element->someFlags &= ~1; // set visible
		log(buf, sprintf(buf, "%selement type %d hash %08X flags %8X:%s",
			prefix, element->type, element->hash, element->someFlags, locbuf2));
		if (element->type == 5) {
			containerchild = ((struct UIContainer*) element)->children;
			debug_custom_uielementvisitor_visit_element(containerchild, locbuf);
		}
		element = element->nextSibling;
	}
}

static
void debug_custom_uielementvisitor_visit_fng(struct FNGInfo *fng, char *prefix)
{
	char locbuf[100];
	struct UIElement *element;

	sprintf(locbuf, "%s    ", prefix);
	do {
		log(buf, sprintf(buf, "%sfng %s:", prefix, fng->fngName));
		element = fng->rootUIElement;
		debug_custom_uielementvisitor_visit_element(element, locbuf);
		fng = fng->child;
	} while (fng);
}

static
void debug_custom_uielementvisitor(int wparam)
{
	if (wparam == 121) { // y
		log(buf, sprintf(buf, "\n\n\nVISITING ELEMENT\n"));
		debug_custom_uielementvisitor_visit_fng(pUIData[0]->field_8->topPackage, "");
	}

	DEBUG_WMCHAR_FUNC(wparam);
}
#undef DEBUG_WMCHAR_FUNC
#define DEBUG_WMCHAR_FUNC debug_custom_uielementvisitor
