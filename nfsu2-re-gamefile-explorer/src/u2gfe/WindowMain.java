package u2gfe;

import java.awt.Dimension;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.Graphics2D;

import javax.swing.JFrame;
import javax.swing.JTabbedPane;
import javax.swing.WindowConstants;

class WindowMain extends JFrame
{
static Font monospace;
static int fx, fy, fmaxascend;
static boolean hasFontMetrics;

static void updateFontMetrics(Graphics g)
{
	if (!hasFontMetrics) {
		hasFontMetrics = true;
		FontMetrics metrics = ((Graphics2D) g).getFontMetrics(monospace);
		fx = metrics.charWidth('m');
		fy = metrics.getHeight();
		fmaxascend = metrics.getMaxAscent();
	}
}

final JTabbedPane tabbedPane;

WindowMain()
{
	super("nfsu2-re-gamefile-explorer");

	monospace = new Font("Courier New", Font.PLAIN, 12);

	this.setLocationByPlatform(true);
	this.tabbedPane = new JTabbedPane();
	this.tabbedPane.addTab("Start Page", new TabStartPage());

	Messages.SHOW_ERROR.subscribe(error -> {
		this.tabbedPane.addTab(error.getMessage(), new TabError(error));
		this.tabbedPane.setSelectedIndex(this.tabbedPane.getTabCount() - 1);
		Util.makeTabLabelMMBClosable(this.tabbedPane, this.tabbedPane.getTabCount() - 1);
	});

	Messages.SHOW_SYMBOLS.subscribe(ordered -> {
		this.tabbedPane.addTab("Hash list", new TabSymbols(ordered.booleanValue()));
		this.tabbedPane.setSelectedIndex(this.tabbedPane.getTabCount() - 1);
		Util.makeTabLabelMMBClosable(this.tabbedPane, this.tabbedPane.getTabCount() - 1);
	});

	Messages.SHOW_BINFILE.subscribe(binfile -> {
		this.tabbedPane.addTab(binfile.name, new TabBinFile(binfile, 0));
		this.tabbedPane.setSelectedIndex(this.tabbedPane.getTabCount() - 1);
		Util.makeTabLabelMMBClosable(this.tabbedPane, this.tabbedPane.getTabCount() - 1);
	});

	Messages.SHOW_BINFILE_AT_OFFSET.subscribe(data -> {
		String title = String.format("%s (%s)", data.file.name, data.titleName);
		this.tabbedPane.addTab(title, new TabBinFile(data.file, data.offset));
		this.tabbedPane.setSelectedIndex(this.tabbedPane.getTabCount() - 1);
		Util.makeTabLabelMMBClosable(this.tabbedPane, this.tabbedPane.getTabCount() - 1);
	});

	this.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
	this.setPreferredSize(new Dimension(1100, 880));
	this.getContentPane().add(this.tabbedPane);
	this.pack();
	this.setVisible(true);
}
}
