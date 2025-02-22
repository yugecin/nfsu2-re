package u2gfe.cellui;

import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.util.function.*;
import javax.swing.*;
import javax.swing.Timer;

import static u2gfe.cellui.CUGfx.*;

public class CURoot extends JComponent
	implements MouseListener, MouseMotionListener, MouseWheelListener, ComponentListener, ActionListener
{
public static final int BOTTOM_TAB_HACK_GRID_START_Y = 20000; // must be lt 2^15 and allow many more rows
public static boolean CLOSABLE = true;
public static boolean NOT_CLOSABLE = false;

Point awtmouse, gridmouse;
String tooltipText[];
LinkedList<CURootTab> tabs;
CURootTab activeTab;
int lastHoveredGrid = grid(-100, -100), mousePressedGrid = grid(-100, -100);
Point awtExtraSize;
HashMap<Integer, String[]> hoverData = new HashMap<>();
HashMap<Integer, Object> clickData = new HashMap<>();
Timer timer;
int tickCount;
int lastPaintTickCount;
boolean requiresTimerRepaint;

public CURoot()
{
	this.tabs = new LinkedList<>();
	this.addMouseListener(this);
	this.addMouseMotionListener(this);
	this.addMouseWheelListener(this);
	this.awtExtraSize = new Point(0, 0);
	this.awtmouse = new Point(-100, -100);
	this.gridmouse = new Point(-100, -100);
	this.lastHoveredGrid = grid(-100, -100);
	this.timer = new Timer(20, this);
	this.timer.start();
}

/**
 * @param header may only render a single unit height
 * @param closable prefer to use {@link #CLOSABLE} or {@link #NOT_CLOSABLE}
 */
public void addTab(CUComponent header, CUComponent content, boolean closable)
{
	this.tabs.add(new CURootTab(this, header, content, closable));
	this.activeTab = this.tabs.getLast();
}

public void repaintIfActiveTab(CUComponent tabContent)
{
	if (this.activeTab != null && this.activeTab.content == tabContent) {
		this.repaint();
	}
}

public void removeTab(CUComponent withContent)
{
	for (var iter = this.tabs.listIterator(this.tabs.size()); iter.hasPrevious();) {
		var tab = iter.previous();
		if (tab.content == withContent) {
			iter.remove();
			if (tab == this.activeTab) {
				if (iter.hasPrevious()) {
					this.activeTab = iter.previous();
				} else if (iter.hasNext()) {
					this.activeTab = iter.next();
				} else {
					this.activeTab = null;
				}
			}
			return;
		}
	}
}

private void updateMousePosition(MouseEvent e)
{
	this.awtmouse.x = e.getX(); this.awtmouse.y = e.getY();
	this.gridmouse.x = this.awtmouse.x / fx;

	// hack: bottom tabs are pushed way down the grid. See paint fun for more explanation.
	int bottomTabsStartY = this.getHeight();
	for (var li = this.tabs.listIterator(this.tabs.size()); li.hasPrevious() && li.previous() != this.activeTab; bottomTabsStartY -= fy);
	if (e.getY() >= bottomTabsStartY) {
		this.gridmouse.y = (this.awtmouse.y - this.awtExtraSize.y) / fy + BOTTOM_TAB_HACK_GRID_START_Y;
	} else {
		this.gridmouse.y = this.awtmouse.y / fy;
	}
}

@Override
protected void paintComponent(Graphics _awtg)
{
	updateFontMetrics(_awtg);
	this.clickData.clear();
	this.hoverData.clear();

	int awtw = this.getWidth(), awth = this.getHeight();
	awtExtraSize.x = awtw % fx; awtExtraSize.y = awth % fy;
	int w = (awtw - awtExtraSize.x) / fx, h = (awth - awtExtraSize.y) / fy;
	var isTimerTick = this.lastPaintTickCount != this.tickCount;
	var g = new CUGfx(_awtg, gridmouse, this.hoverData, this.clickData, this.tickCount, isTimerTick);
	this.lastPaintTickCount = this.tickCount;

	g.pushTranslate();
	g.pushClipRaw(awtw, awth);
	g.setColor(colBg);
	g.fillRect(0, 0, w + 1, h + 1);

	for (int i = 0, isTabTitleTopOfScreen = 1; i < this.tabs.size(); i++) {
		var tab = this.tabs.get(i);
		tab.paintHeader(g, w, isTabTitleTopOfScreen == 1);
		g.translate(0, 1);
		if (tab == this.activeTab) {
			int availableHeight = h - this.tabs.size();
			tab.paintContent(g, w, availableHeight, awtExtraSize);

			g.translate(0, availableHeight);
			g.awtg.translate(0, awtExtraSize.y);
			// hack: The last line of content may only be a partial line, so this breaks the
			// grid. In order to not have unwanted overlapping in clickdata/hoverdata for
			// the two rows that overlap, push the grid an insane amount by translating
			// and then sneakily undo the translation. The click/hover listeners then
			// also need to adjust the grid calculation if it happens below the position
			// where the grid was broken.
			g.translate(0, BOTTOM_TAB_HACK_GRID_START_Y);
			g.awtg.translate(0, fy * -BOTTOM_TAB_HACK_GRID_START_Y);

			isTabTitleTopOfScreen = 0;
		}
	}

	if (!g.popClip() || !g.popTranslate()) {
		throw new RuntimeException("unbalanced clip or translate stack");
	}

	if (this.tooltipText != null) {
		int width = 0, height = (this.tooltipText.length * fy) + 4;
		for (String line : this.tooltipText) {
			width = Math.max(width, line.length());
		}
		width = width * fx + 4;
		int fromX = this.awtmouse.x - width / 2, fromY = this.awtmouse.y - 4 - height;
		if (fromY < 0) {
			fromY = this.awtmouse.y + 20;
		}
		if (fromX < 2) {
			fromX = 2;
		}
		if (fromX + width > awtw - 2) {
			fromX = awtw - 2 - width;
		}
		g.awtg.translate(fromX, fromY);
		g.awtg.setColor(Color.black);
		g.awtg.drawRect(0, 0, width - 1, height - 1);
		g.awtg.translate(1, 1);
		g.awtg.setColor(Color.yellow);
		g.awtg.fillRect(0, 0, width - 2, height - 2);
		g.awtg.translate(1, 1);
		g.awtg.setColor(Color.black);
		for (String line : this.tooltipText) {
			g.drawString(0, 0, line);
			g.awtg.translate(0, fy);
		}
	}

	this.requiresTimerRepaint = g.requireTimerRepaint;
}

/*ActionListener*/
@Override
public void actionPerformed(ActionEvent e)
{
	if (e.getSource() == this.timer) {
		this.tickCount = (int) (System.currentTimeMillis() / 100);
		if (this.requiresTimerRepaint) {
			this.repaint();
		}
	}
}

/*MouseListener*/
@Override
public void mouseClicked(MouseEvent e)
{
}

/*MouseListener*/
@Override
public void mousePressed(MouseEvent e)
{
	updateMousePosition(e);

	this.mousePressedGrid = grid(this.gridmouse.x, this.gridmouse.y);
}

/*MouseListener*/
@Override
public void mouseReleased(MouseEvent e)
{
	updateMousePosition(e);

	int grid = grid(this.gridmouse.x, this.gridmouse.y);
	if (grid == this.mousePressedGrid) {
		Object clickData = this.clickData.get(grid);
		CUComponent.Clickable comp = null;
		Object data = null;
		if (clickData instanceof CUComponent.Clickable) {
			comp = (CUComponent.Clickable) clickData;
		} else if (clickData instanceof CUGfx.ClickData) {
			comp = ((CUGfx.ClickData) clickData).owner;
			data = ((CUGfx.ClickData) clickData).userdata;
		} else if (clickData != null) {
			throw new RuntimeException("invalid click listener");
		}
		if (comp != null && (comp.onClick(grid, e.getButton(), data) & CUComponent.Clickable.ONCLICK_REPAINT) != 0) {
			this.repaint();
		}
	}
	this.mousePressedGrid = grid(-100, -100);
}

/*MouseListener*/
@Override
public void mouseEntered(MouseEvent e)
{
}

/*MouseListener*/
@Override
public void mouseExited(MouseEvent e)
{
	this.awtmouse.x = -100; this.awtmouse.y = -100;
	this.gridmouse.x = -100; this.gridmouse.y = -100;
	this.lastHoveredGrid = grid(-100, -100);
	this.tooltipText = null;
	this.repaint();
}

/*MouseMotionListener*/
@Override
public void mouseDragged(MouseEvent e)
{
	this.mouseMoved(e);
}

/*MouseMotionListener*/
@Override
public void mouseMoved(MouseEvent e)
{
	updateMousePosition(e);

	int grid = grid(this.gridmouse.x, this.gridmouse.y);
	String[] tooltipText = this.hoverData.get(grid);
	if (tooltipText != null) {
		this.tooltipText = tooltipText;
	} else if (this.tooltipText != null) {
		this.tooltipText = null;
	} else if (this.lastHoveredGrid == grid) {
		return;
	}
	this.lastHoveredGrid = grid;
	this.repaint();
}

/*MouseWheelListener*/
@Override
public void mouseWheelMoved(MouseWheelEvent e)
{
	int units = e.getUnitsToScroll();
	var tab = this.activeTab;
	if (tab != null) {
		ToIntFunction<Point> getV = e.isShiftDown() ? p -> p.x : p -> p.y;
		IntConsumer setV = e.isShiftDown() ? v -> tab.offset.x = v : v -> tab.offset.y = v;
		var changed = tab.applyScrollValue(units, getV, setV);
		if (changed) {
			this.repaint();
		}
	}
}

/*ComponentListener*/
@Override
public void componentResized(ComponentEvent e)
{
	this.repaint();
}

/*ComponentListener*/
@Override
public void componentMoved(ComponentEvent e)
{
}

/*ComponentListener*/
@Override
public void componentShown(ComponentEvent e)
{
}

/*ComponentListener*/
@Override
public void componentHidden(ComponentEvent e)
{
}
} /*CURoot*/