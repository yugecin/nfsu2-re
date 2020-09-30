/*
526C40 GETFNGFORDIALOG

Decides the FNG to display for passed dialog.
*/

static
char* do526C40getFNGforDialog(struct DialogInfo *dialog)
{
	int num_lines;
	int i;
	int tmp;
	float text_width;

	if (dialog->pTypeName && dialog->pTypeName[0]) {
		if (strcmp(dialog->pTypeName, "animating") || strcmp(dialog->pTypeName, "3button")) {
			return dialog->pTypeName;
		}
	}

	/*526C80*/
	if (dialog->isHelpDialog) {
		if (dialog->text[0]) {
			num_lines = 1;
			i = 0;
nextchar:		{
				switch (dialog->text[i]) {
				case '\n':
				case '^': num_lines++;
				default:
					i++;
					goto nextchar;
				case 0: break;
				}
			}
			if (num_lines > 4) {
				return "HelpDialog_MED.fng";
			}
		}
		return "HelpDialog_SMALL.fng";
	}

	/*526CBF*/
	tmp = ((int(__cdecl *)(char*))0x43DB50)("CONDUITMDITC_TT21I"); /*SomeHashCS43DB50*/
	((void(__cdecl*)(int))0x51BD70)(tmp);
	_asm { mov eax, [eax+0x24] }
	_asm { mov text_width, eax }

	text_width *= strlen(dialog->text);

	if (strcmp(dialog->pTypeName, "3button") == 0) {
		return "GenericDialog_ThreeButton.fng";
	}

	/*526D07*/
	if (text_width < 2561.0f) {
		if (strcmp(dialog->pTypeName, "animating")) {
			return "GenericDialog_SMALL.fng";
		} else {
			return "GenericDialog_Animate_SMALL.fng";
		}
	}

	if (text_width < 5122.0f) {
		if (strcmp(dialog->pTypeName, "animating")) {
			return "GenericDialog_MED.fng";
		} else {
			return "GenericDialog_Animate_MED.fng";
		}
	}

	if (strcmp(dialog->pTypeName, "animating")) {
		return "GenericDialog_LARGE.fng";
	} else {
		return "GenericDialog_Animate_LARGE.fng";
	}
}

static
void initHook526C40getFNGforDialog()
{
	mkjmp(0x526C40, &do526C40getFNGforDialog);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHook526C40getFNGforDialog
}
