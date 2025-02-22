package u2gfe;

import java.awt.*;
import java.util.*;
import u2gfe.cellui.*;

import static u2gfe.Util.*;
import static u2gfe.cellui.CUGfx.*;

class TabFilelist extends CUComponent implements CUComponent.Clickable
{
static final String TXT_OPEN = "open", TXT_LOADING = "loading file list...", TXT_HDR = "Essential Files:";

final WindowMain main;

boolean isLoading;
Throwable collectFilesError;
ArrayList<FileLine> essentialFileLines;
int maxLineLength;

TabFilelist(WindowMain main, ArrayList<String> essentialFiles)
{
	this.main = main;

	this.isLoading = true;
	this.maxLineLength = TXT_HDR.length();
	this.essentialFileLines = new ArrayList<>(essentialFiles.size());
	for (var iter = essentialFiles.iterator(); iter.hasNext();) {
		var isLast = !iter.hasNext();
		var fileline = new FileLine(iter.next(), 1, isLast);
		this.essentialFileLines.add(fileline);
		this.maxLineLength = max(this.maxLineLength, fileline.width);
	}
}

void onFilesCollected()
{
	this.isLoading = false;
	this.repaintIfActive();
}

void repaintIfActive()
{
	this.main.root.repaintIfActiveTab(this);
}

@Override
public int calculateHeight(int givenWidth, int availableHeight)
{
	int essentials = 1 + this.essentialFileLines.size() + 1;
	if (this.isLoading) {
		return essentials + 1;
	}
	return essentials;
}

@Override
public int getWidth(int availableWidth)
{
	return this.maxLineLength;
}

@Override
public void paint(CUGfx g, int givenWidth, int givenHeight)
{
	g.setColor(Color.black);
	if (g.rowsInViewport(1)) {
		g.drawString(0, 0, TXT_HDR);
	}
	g.translate(0, 1);
	for (FileLine entry : this.essentialFileLines) {
		if (g.rowsInViewport(1)) {
			entry.paint(g, givenWidth, 1);
		}
		g.translate(0, 1);
	}
	if (g.rowsInViewport(1)) {
		g.awtg.drawLine(0, fy / 2, fx * max(givenWidth, this.getWidth(givenWidth)), fy / 2);
	}
	g.translate(0, 1);
	if (this.isLoading) {
		g.drawString(0, 0, TXT_LOADING);
		return;
	}
	if (collectFilesError != null) {
		g.drawString(0, 0, "error while collecting files");
		g.translate(29, 0);
		g.button(this, "SHOW_ERROR", "show error", colBtnDefBg, colBtnHovBg, true);
		g.translate(13, 0);
		g.button(this, "RETRY", "retry", colBtnDefBg, colBtnHovBg, true);
	}
}

@Override
public int onClick(int grid, int button, Object userdata)
{
	if (userdata == "SHOW_ERROR") {
		Messages.SHOW_ERROR.send(collectFilesError);
	} else if (userdata == "RETRY") {
		collectFilesError = null;
		Messages.PARSER_REQUEST_BASICS.send(null);
		return ONCLICK_REPAINT;
	}
	return 0;
}

class FileLine extends CUComponent implements CUComponent.Clickable
{
static final String TXT_OPEN = "open";

String path;
int indent;
int width;
boolean isLast;

FileLine(String path, int indent, boolean isLast)
{
	this.path = path;
	this.indent = indent;
	this.isLast = isLast;
	this.width = indent + 1 + path.length() + 1 + TXT_OPEN.length() + 2;
}

@Override
public int calculateHeight(int givenWidth, int availableHeight)
{
	return 0;
}

@Override
public int getWidth(int availableWidth)
{
	return this.width;
}

@Override
public void paint(CUGfx g, int givenWidth, int givenHeight)
{
	g.pushTranslate();
	g.setColor(Color.black);
	for (int i = 0; i < indent; i++) {
		if (isLast && i == indent - 1) {
			g.awtg.drawLine(fx / 2, 0, fx / 2, fy / 2);
		} else {
			g.awtg.drawLine(fx / 2, 0, fx / 2, fy);
		}
		g.awtg.drawLine(fx / 2, fy / 2, fx, fy / 2);
		g.translate(1, 0);
	}
	g.translate(1, 0);
	g.link(this, null, path);
	/*
	g.drawString(1, 0, path);
	g.translate(2 + path.length(), 0);
	g.button(this, null, TXT_OPEN, colBtnDefBg, colBtnHovBg, true);
	*/
	g.popTranslate();
}

@Override
public int onClick(int grid, int button, Object userdata)
{
	main.openFile(path);
	return ONCLICK_REPAINT;
}
} /*FileLine*/
} /*TabFileList*/