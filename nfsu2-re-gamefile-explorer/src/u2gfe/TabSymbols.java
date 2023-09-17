package u2gfe;

import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.util.ArrayList;

import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JScrollPane;

class TabSymbols extends JScrollPane implements MouseListener
{
JList<Symbol> list;

TabSymbols(boolean ordered)
{
	super();

	if (Symbol.symbols.isEmpty()) {
		this.setViewportView(new JLabel("no hashes available. did files parse correctly?"));
		return;
	}

	ArrayList<Symbol> symbols = Symbol.symbolsInNaturalOrder;
	if (ordered) {
		symbols = new ArrayList<>(Symbol.symbols.values());
		symbols.sort(null);
	};

	this.list = new JList<>(symbols.toArray(new Symbol[0]));
	this.list.setFont(WindowMain.monospace);
	this.list.addMouseListener(this);
	this.setViewportView(this.list);
}

/*MouseListener*/
@Override
public void mouseClicked(MouseEvent e)
{
	Symbol symbol = this.list.getSelectedValue();
	if (e.getClickCount() == 2 && symbol != null) {
		BinFile file = symbol.file;
		int offset = symbol.definitionOffset;
		String title = String.format("hash definition %X", symbol.id);
		Messages.SHOW_BINFILE_AT_OFFSET.send(new Messages.ShowBinFileAtOffset(file, offset, title));
	}
}

/*MouseListener*/
@Override
public void mousePressed(MouseEvent e)
{
}

/*MouseListener*/
@Override
public void mouseReleased(MouseEvent e)
{
}

/*MouseListener*/
@Override
public void mouseEntered(MouseEvent e)
{
}

/*MouseListener*/
@Override
public void mouseExited(MouseEvent e)
{
}
}
