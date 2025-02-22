package u2gfe.cellui;

import java.awt.Color;

public class CUText extends CUComponent
{
String text;

public CUText(String text)
{
	this.text = text;
}

@Override
public int calculateHeight(int givenWidth, int availableHeight)
{
	return 1;
}

@Override
public int getWidth(int availableWidth)
{
	return this.text.length();
}

@Override
public void paint(CUGfx g, int givenWidth, int givenHeight)
{
	g.setColor(Color.black);
	g.drawString(0, 0, this.text);
}
}
