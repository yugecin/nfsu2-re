package u2gfe;

import java.awt.*;
import u2gfe.cellui.*;

import static u2gfe.cellui.CUGfx.*;
import static u2gfe.Util.*;

class TabNewstartpage extends CUComponent implements CUComponent.Clickable
{
private static String TXT_BTN = "Select game dir";
private static String TXT_BAD1 = "bad game dir selected";
private static String TXT_BAD2 = "game dir should contain SPEED2.EXE or SPEED2DEMO.EXE";

private final WindowMain main;

boolean badGameDirSelected;

TabNewstartpage(WindowMain main)
{
	this.main = main;
}

@Override
public int getWidth(int availableWidth)
{
	var btnLen = TXT_BTN.length() + 2;
	return this.badGameDirSelected ? max(btnLen, TXT_BAD1.length(), TXT_BAD2.length()) : btnLen;
}

@Override
public int calculateHeight(int givenWidth, int availableHeight)
{
	return this.badGameDirSelected ? 3 : 1;
}

@Override
public void paint(CUGfx g, int givenWidth, int givenHeight)
{
	int h = this.badGameDirSelected ? 3 : 1;
	int y = (givenHeight - h) / 2;
	g.pushTranslate();
	int x = Math.max(0, (givenWidth - (TXT_BTN.length() + 2)) / 2);
	g.translate(x, y);
	g.button(this, TXT_BTN, colBtnDefBg, colBtnHovBg, true);
	if (this.badGameDirSelected) {
		g.translate(-x, 1);
		x = Math.max(0, (givenWidth - TXT_BAD1.length()) / 2);
		g.drawString(x, 0, TXT_BAD1);
		x = Math.max(0, (givenWidth - TXT_BAD2.length()) / 2);
		g.drawString(x, 1, TXT_BAD2);
	}
	g.popTranslate();
}

@Override
public int onClick(int grid, int button, Object userdata)
{
	EventQueue.invokeLater(this.main::showGamedirSelectorFiledialog);
	return ONCLICK_REPAINT;
}
}
