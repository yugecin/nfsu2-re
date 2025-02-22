package u2gfe.cellui;

import java.util.*;

/** Vertical flow layout */
public class CUVFlow extends CUComponent
{
public ArrayList<CUComponent> children;

public CUVFlow(CUComponent...children)
{
	this.children = new ArrayList<>(children.length);
	for (var c : children) {
		this.children.add(c);
	}
}

@Override
public int calculateHeight(int givenWidth, int availableHeight)
{
	int h = 0;
	for (var c : children) {
		// TODO: well this is wrong, not every child should be given the full availableHeight but.... what else to do
		h += c.calculateHeight(givenWidth, availableHeight);
	}
	return h;
}

@Override
public int getWidth(int availableWidth)
{
	int mw = 0;
	for (var c : children) {
		mw = Math.max(mw, c.getWidth(availableWidth));
	}
	return mw;
}

@Override
public void paint(CUGfx g, int givenWidth, int givenHeight)
{
	g.pushTranslate();
	for (var c : children) {
		// TODO: well this is wrong, not every child should be given the full givenHeight but.... what else to do
		int h = c.calculateHeight(givenWidth, givenHeight);
		//if (g.isHeightWithinViewport(h)) {
			c.paint(g, givenWidth, h);
		//}
		g.translate(0, h);
	}
	g.popTranslate();
}
} /*CUVFlow*/
