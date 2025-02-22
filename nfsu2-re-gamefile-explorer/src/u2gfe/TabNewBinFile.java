package u2gfe;

import java.awt.*;
import u2gfe.WindowMain.*;
import u2gfe.cellui.*;

class TabNewBinFile extends CUComponent implements CUComponent.Clickable, WindowMain.Listener, Disposable
{
private static final String TXT_LOADING = "loading...";

WindowMain main;
BinFile file;

TabNewBinFile(WindowMain main, BinFile file)
{
	this.main = main;
	this.file = file;

	this.main.addListener(this);
}

@Override
public void dispose()
{
	this.main.removeListener(this);
}

@Override
public void onBinFileParseResult(BinFile file, Result<BinFile, Throwable> result)
{
	if (file == this.file) {
		this.main.root.repaintIfActiveTab(this);
	}
}

@Override
public int calculateHeight(int givenWidth, int availableHeight)
{
	if (!this.file.isParsed) {
		return Math.max(1, availableHeight);
	}

	return 0;
}

@Override
public int getWidth(int availableWidth)
{
	if (!this.file.isParsed) {
		return Math.max(TXT_LOADING.length(), availableWidth);
	}

	return 0;
}

@Override
public void paint(CUGfx g, int givenWidth, int givenHeight)
{
	if (!this.file.isParsed) {
		int x = (givenWidth - TXT_LOADING.length()) / 2;
		int y = givenHeight / 2;
		g.setColor(Color.black);
		g.drawString(x, y, TXT_LOADING);
	} else {
		g.drawString(0, 0, "parsed");
	}
}

@Override
public int onClick(int grid, int button, Object userdata)
{
	return 0;
}
}
