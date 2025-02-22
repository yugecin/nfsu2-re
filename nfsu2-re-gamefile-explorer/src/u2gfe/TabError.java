package u2gfe;

import java.awt.*;
import java.io.*;
import u2gfe.cellui.*;

import static u2gfe.Util.*;

class TabError extends CUComponent
{
String[] lines;
int maxwidth;

TabError(Throwable error)
{
	StringWriter sw = new StringWriter();
	error.printStackTrace(new PrintWriter(sw));
	this.lines = sw.toString().split("\n");
	this.maxwidth = 0;
	int indent = 0;
	for (String line : lines) {
		this.maxwidth = max(this.maxwidth, indent + line.length());
		indent = 2;
	}
}

@Override
public int calculateHeight(int givenWidth, int availableHeight)
{
	return this.lines.length;
}

@Override
public int getWidth(int availableWidth)
{
	return this.maxwidth;
}

@Override
public void paint(CUGfx g, int givenWidth, int givenHeight)
{
	g.setColor(Color.black);
	int indent = 0;
	for (int i = 0; i < this.lines.length; i++) {
		g.drawString(indent, i, this.lines[i]);
		indent = 2;
	}
}
}
