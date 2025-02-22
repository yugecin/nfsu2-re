package u2gfe;

import java.awt.*;
import java.io.*;
import java.util.*;
import javax.swing.*;
import u2gfe.cellui.*;

import static u2gfe.cellui.CURoot.*;

class WindowMain extends JFrame
{
/** {@link GamefileParser#parseFileSync} should not be used by any other class; they should use {@link #parseFileAsync} instead */
GamefileParser gamefileParser;
TabNewstartpage tabStartpage;
TabFilelist tabFilelist;
CURoot root;

WindowMain()
{
	super("nfsu2-re-gamefile-explorer");

	this.setLocationByPlatform(true);

	/*
	this.tabbedPane = new JTabbedPane();
	this.tabbedPane.addTab("Start Page", new TabStartPage());

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
	*/

	this.tabStartpage = new TabNewstartpage(this);
	this.root = new CURoot();
	this.root.addTab(new CUText("Start Page"), this.tabStartpage, NOT_CLOSABLE);

	var dir = System.getProperty("U2GFE_INIT_WITH_DIRECTORY");
	if (dir != null) {
		EventQueue.invokeLater(() -> this.loadGamedir(new File(dir)));
	}

	this.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
	this.setPreferredSize(new Dimension(1100, 880));
	this.getContentPane().add(this.root);
	this.pack();
	this.setVisible(true);
}

void showError(Throwable error)
{
	this.root.addTab(new CUText(error.toString()), new TabError(error), CLOSABLE);
}

void showGamedirSelectorFiledialog()
{
	var fd = new FileDialog(this, "Select SPEED2.EXE (or any file in the game directory)");
	fd.setVisible(true);
	var files = fd.getFiles();
	if (files.length == 0) {
		return;
	}
	File dir;
	try {
		dir = files[0].getParentFile().getCanonicalFile();
	} catch (IOException e) {
		this.showError(new Exception("failed to get game directory canonical file", e));
		return;
	}
	if (new File(dir, "speed2.exe").isFile() || new File(dir, "speed2demo.exe").isFile()) {
		this.loadGamedir(dir);
	} else {
		this.tabStartpage.badGameDirSelected = true;
		this.root.repaintIfActiveTab(this.tabStartpage);
	}
}

void loadGamedir(File dir)
{
	this.root.removeTab(this.tabStartpage);
	this.tabStartpage = null;
	Symbol.clearSymbols();
	this.gamefileParser = new GamefileParser(dir);
	Tasks.scheduleTask(() -> {
		//this.gamefileParser.collectFiles();
		//EventQueue.invokeLater(this.tabFilelist::onFilesCollected);
	});
	var filelistEssentialFiles = new ArrayList<String>(GamefileParser.ESSENTIAL_FILES_RELATIVEPATHS.length);
	for (String relativePath : GamefileParser.ESSENTIAL_FILES_RELATIVEPATHS) {
		var file = this.gamefileParser.gamedir.toPath().resolve(relativePath).toFile();
		var binfile = this.gamefileParser.getOrCreateBinFileFor(file);
		this.parseFileAsync(binfile);
		filelistEssentialFiles.add(binfile.path);
	}
	this.tabFilelist = new TabFilelist(this, filelistEssentialFiles);
	this.root.addTab(new CUText("File list"), this.tabFilelist, NOT_CLOSABLE);
}

void parseFileAsync(BinFile binfile)
{
	Tasks.scheduleTask(() -> {
		try {
			Thread.sleep(2000); // TODO remove me
		} catch (InterruptedException e) {
		}
		var result = this.gamefileParser.parseFileSync(binfile);
		if (!result.isOk) {
			EventQueue.invokeLater(() -> this.showError(result.err));
		}
	});
}

void openFile(String path)
{
}
}
