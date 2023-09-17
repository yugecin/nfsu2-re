package u2gfe;

import java.awt.EventQueue;
import java.io.File;
import java.util.ArrayList;

import javax.swing.SwingUtilities;

class Messages<T>
{
interface Subscriber<T>
{
	void receiveMessage(T arg);
}

static final Messages<File> GAME_DIR_SELECTED = new Messages<>();
static final Messages<Void> START_PARSING = new Messages<>();
static final Messages<Void> DONE_PARSING = new Messages<>();
static final Messages<BinFile> GAME_FILE_PARSED = new Messages<>();
static final Messages<Throwable> SHOW_ERROR = new Messages<>();
static final Messages<BinFile> SHOW_BINFILE = new Messages<>();
static final Messages<Boolean> SHOW_SYMBOLS = new Messages<>();
static final Messages<ShowBinFileAtOffset> SHOW_BINFILE_AT_OFFSET = new Messages<>();

private final ArrayList<Subscriber<T>> subs = new ArrayList<>();

void subscribe(Subscriber<T> sub)
{
	subs.add(sub);
}

void send(T arg)
{
	Runnable doSend = () -> {
		for (Subscriber<T> s : subs) {
			s.receiveMessage(arg);
		}
	};

	if (EventQueue.isDispatchThread()) {
		doSend.run();
	} else {
		SwingUtilities.invokeLater(doSend);
	}
}

static class ShowBinFileAtOffset
{
BinFile file;
int offset;
String titleName;

ShowBinFileAtOffset(Symbol symbol)
{
	this.file = symbol.file;
	this.offset = symbol.definitionOffset;
	this.titleName = String.format("hash definition %X", symbol.id);
}

ShowBinFileAtOffset(BinFile file, int offset, String titleName)
{
	this.file = file;
	this.offset = offset;
	this.titleName = titleName;
}
}
}
