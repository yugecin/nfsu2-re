package u2gfe;

import java.awt.Adjustable;
import java.awt.BorderLayout;

import javax.swing.JPanel;
import javax.swing.JScrollBar;

class TabBinFile extends JPanel
{
TabBinFile(BinFile file, int scrollToOffset)
{
	super(new BorderLayout());

	if (!file.errors.isEmpty()) {
		this.add(new TabBinFileHeaderErrorList(file.errors), BorderLayout.NORTH);
	}

	JScrollBar sbv = new JScrollBar(), sbh = new JScrollBar(Adjustable.HORIZONTAL);

	this.add(sbv, BorderLayout.EAST);
	this.add(sbh, BorderLayout.SOUTH);
	this.add(new PanelBinFile(file, sbh, sbv, scrollToOffset));
}
}
