static
void debug_custom_uielementvisitor_visit_fng(struct FNGInfo *fng, char *prefix)
{
	char locbuf[100];
	struct UIElement *element;

	sprintf(locbuf, "%s    ", prefix);
	do {
		log(buf, sprintf(buf, "%sfng %s:", prefix, fng->fngName));
		element = fng->rootUIElement;
		print_ui_element_and_children(element, locbuf, 1);
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
