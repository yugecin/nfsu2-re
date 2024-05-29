package u2gfe;

import java.awt.Component;
import java.awt.Dimension;
import java.awt.FileDialog;
import java.awt.Frame;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;

import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.SwingUtilities;
import javax.swing.border.EmptyBorder;
import javax.swing.border.TitledBorder;

import static u2gfe.Util.bordered;

class TabStartPage extends JScrollPane
{
TabStartPage()
{
	super(new JPanel(new GridBagLayout()));

	JPanel root = (JPanel) this.getViewport().getView();

	GridBagConstraints c = new GridBagConstraints();
	c.insets.top = c.insets.bottom = 5;
	c.gridx = 1;
	c.gridy = 1;
	root.add(new PanelGameDir(), c);
	c.gridy++;
	root.add(new PanelListGameFiles(), c);
	c.gridy++;
	root.add(new PanelSymbols(), c);
	c.gridy++;
	root.add(new PanelListSaveGames(), c);
}
} /*TabStartPage*/

// ---

class PanelGameDir extends JPanel implements ActionListener
{
PanelGameDir()
{
	JButton btn = new JButton("browse");
	btn.addActionListener(this);

	this.setBorder(new TitledBorder("Game dir"));
	this.add(btn);
}

/*ActionListener*/
@Override
public void actionPerformed(ActionEvent e)
{
	Frame root = (Frame) SwingUtilities.getRoot(this);
	FileDialog fd = new FileDialog(root, "Select SPEED2.EXE (or any file in the game directory)");
	fd.setVisible(true);
	File file, files[] = fd.getFiles();
	if (files.length == 0) {
		return;
	}
	if (files[0].isFile() &&
		(file = files[0].getParentFile()) != null &&
		(new File(file, "speed2.exe").isFile() || new File(file, "speed2demo.exe").isFile()))
	{
		this.setVisible(false);
		Messages.GAME_DIR_SELECTED.send(file);
	} else {
		JOptionPane.showMessageDialog(this, "speed2.exe not found", root.getTitle(), JOptionPane.ERROR_MESSAGE);
	}
}
} /*PanelGameDir*/

// ---

class PanelSymbols extends Box
{
PanelSymbols()
{
	super(BoxLayout.Y_AXIS);

	this.setVisible(false);
	this.setBorder(new TitledBorder("Hash list"));
	this.setMinimumSize(new Dimension(200, 200));

	Messages.DONE_PARSING.subscribe(a -> this.setVisible(true));

	JButton btnSymbols = new JButton("show hash list");
	btnSymbols.setMaximumSize(new Dimension(10000, 100)); // ensure it will be streched
	btnSymbols.addActionListener(e -> Messages.SHOW_SYMBOLS.send(Boolean.FALSE));
	this.add(btnSymbols);

	btnSymbols = new JButton("show hash list (sorted)");
	btnSymbols.setMaximumSize(new Dimension(10000, 100)); // ensure it will be streched
	btnSymbols.addActionListener(e -> Messages.SHOW_SYMBOLS.send(Boolean.TRUE));
	this.add(btnSymbols);
}
} /*PanelSymbols*/

// ---

class PanelListGameFiles extends Box
{
PanelListGameFiles()
{
	super(BoxLayout.Y_AXIS);

	this.setVisible(false);
	this.setBorder(new TitledBorder("Game files"));
	this.setMinimumSize(new Dimension(200, 200));
	this.add(bordered(new JLabel("Loading"), new EmptyBorder(10, 30, 10, 30)));

	Messages.START_PARSING.subscribe(a -> this.setVisible(true));

	Messages.GAME_FILE_PARSED.subscribe(file -> {
		JButton btn = new JButton(file.name + (file.errors.isEmpty() ? "" : " (errors)"));
		btn.setMaximumSize(new Dimension(10000, 100)); // ensure it will be streched
		btn.setVisible(false);
		btn.addActionListener(e -> Messages.SHOW_BINFILE.send(file));
		this.add(btn);
	});

	Messages.DONE_PARSING.subscribe(a -> {
		for (Component c : this.getComponents()) {
			c.setVisible(c instanceof JButton);
		}
		if (this.getComponentCount() == 1) {
			this.add(bordered(new JLabel("none?!"), new EmptyBorder(10, 30, 10, 30)));
		}
	});
}
} /*PanelListGameFiles*/

// ---

class PanelListSaveGames extends JPanel
{
PanelListSaveGames()
{
	this.setBorder(new TitledBorder("Savegames"));
	this.setVisible(false);
}
} /*PanelListSaveGames*/
