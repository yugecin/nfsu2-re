package u2gfe;

import java.awt.Adjustable;
import java.awt.BorderLayout;

import javax.swing.JPanel;
import javax.swing.JScrollBar;
import javax.swing.JScrollPane;

class TabBinFile extends JScrollPane
{
TabBinFile(BinFile file, int scrollToOffset)
{
	super(new JPanel(new BorderLayout()));

	JPanel root = (JPanel) this.getViewport().getView();
	if (!file.errors.isEmpty()) {
		root.add(new TabBinFileHeaderErrorList(file.errors), BorderLayout.NORTH);
	}

	JScrollBar sbv = new JScrollBar(), sbh = new JScrollBar(Adjustable.HORIZONTAL);

	root.add(sbv, BorderLayout.EAST);
	root.add(sbh, BorderLayout.SOUTH);
	root.add(new PanelBinFile(file, sbh, sbv, scrollToOffset));
}
}
