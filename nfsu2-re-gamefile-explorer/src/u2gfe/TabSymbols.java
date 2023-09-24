package u2gfe;

import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.util.ArrayList;

import javax.swing.JLabel;
import javax.swing.JScrollPane;
import javax.swing.JTree;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.TreePath;

class TabSymbols extends JScrollPane implements MouseListener
{
JTree tree;

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

	DefaultMutableTreeNode root = new DefaultMutableTreeNode();
	for (Symbol sym : symbols) {
		DefaultMutableTreeNode node = new DefaultMutableTreeNode(sym);
		root.add(node);
		for (Symbol.Reference ref : sym.references) {
			node.add(new DefaultMutableTreeNode(ref));
		}
	}
	this.tree = new JTree(root);
	this.tree.setToggleClickCount(1);
	this.tree.setRootVisible(false);
	this.tree.setFont(WindowMain.monospace);
	this.tree.addMouseListener(this);
	this.setViewportView(this.tree);
}

/*MouseListener*/
@Override
public void mouseClicked(MouseEvent e)
{
	TreePath path = this.tree.getSelectionPath();
	if (e.getClickCount() == 2 && path != null) {
		Object o = ((DefaultMutableTreeNode) path.getLastPathComponent()).getUserObject();
		if (o instanceof Symbol) {
			Symbol symbol = (Symbol) o;
			BinFile file = symbol.file;
			int offset = symbol.definitionOffset;
			String title = String.format("hash definition %X", symbol.id);
			Messages.SHOW_BINFILE_AT_OFFSET.send(new Messages.ShowBinFileAtOffset(file, offset, title));
		} else if (o instanceof Symbol.Reference) {
			Symbol.Reference ref = (Symbol.Reference) o;
			BinFile file = ref.file;
			int offset = ref.offset;
			String title = String.format("hash ref @%X", ref.offset);
			Messages.SHOW_BINFILE_AT_OFFSET.send(new Messages.ShowBinFileAtOffset(file, offset, title));
		}
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
