
struct SMSMailboxFNGObject* smsm;

static
void debug_custom_printmailboxthings_wmchar(int wparam)
{
	struct ObjectLink *mailboxrelated1base, *mailboxrelated1;
	struct MailBoxRelated1 *mbr1;
	struct ObjectLink *mailboxrelated2base, *mailboxrelated2;
	struct MailBoxRelated1 *mbr2;

	if (wparam == 121) { // y
		printf("========= SMSMailboxFNGObject is %p\n", smsm);

		mailboxrelated1base = &smsm->subScrollingThing1.link1;
		mailboxrelated1 = mailboxrelated1base;
		for (;;) {
			mailboxrelated1 = mailboxrelated1->next;
			if (mailboxrelated1 == mailboxrelated1base) {
				break;
			}
			mbr1 = (void*) ((int) mailboxrelated1 - MEMBER_OFFSET(struct MailBoxRelated1, link1));
			printf("mbr1 top left bottom right %f %f %f %f\n", mbr1->elementRect.top, mbr1->elementRect.left,
				mbr1->elementRect.bottom, mbr1->elementRect.right);
			print_ui_element_and_children(mbr1->uielement, "uielement:", 0);
		}

		mailboxrelated2base = &smsm->subScrollingThing2.link1;
		mailboxrelated2 = mailboxrelated2base;
		for (;;) {
			mailboxrelated2 = mailboxrelated2->next;
			if (mailboxrelated2 == mailboxrelated2base) {
				break;
			}
			mbr2 = (void*) ((int) mailboxrelated2 - MEMBER_OFFSET(struct MailBoxRelated1, link1));
			printf("mbr2 top left bottom right %f %f %f %f\n", mbr2->elementRect.top, mbr2->elementRect.left,
				mbr2->elementRect.bottom, mbr2->elementRect.right);
			print_ui_element_and_children(mbr2->uielement, "uielement:", 0);
		}
	}

	DEBUG_WMCHAR_FUNC(wparam);
}
#undef DEBUG_WMCHAR_FUNC
#define DEBUG_WMCHAR_FUNC debug_custom_printmailboxthings_wmchar

static
__declspec(naked)
void SMSMailboxFNGObject__ctor_hook()
{
	_asm {
		mov [smsm], ecx
		mov eax, 0x4E1BF0
		jmp eax
	}
}

static
void debug_custom_printmailboxthings_init()
{
	redirectjmp(0x4E208A, SMSMailboxFNGObject__ctor_hook);

	INIT_FUNC();
}
#undef INIT_FUNC
#define INIT_FUNC debug_custom_printmailboxthings_init
