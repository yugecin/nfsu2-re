package u2gfe.cellui;

import java.awt.*;
import java.util.*;

public class CUGfx
{
public static Color colBg = new Color(255, 255, 234);
public static Color colScroll = new Color(153, 153, 76);
public static Color colTabBg = new Color(234, 255, 255);
public static Color colAccent = new Color(136, 136, 204);
public static Color colRedStatic = new Color(0xff5555);
public static Color colRedHover = new Color(0xff8888);
public static Color colBtnDefBg = new Color(0x99ffff);
public static Color colBtnHovBg = new Color(0x66cccc);
public static Color colBtnDisabledBg = new Color(0xcccccc);

public static int fx, fy, fmaxascend;

static Font monospace;
static boolean hasFontMetrics;

static
{
	monospace = new Font("Courier New", Font.PLAIN, 12);
}

static void updateFontMetrics(Graphics g)
{
	if (!hasFontMetrics) {
		hasFontMetrics = true;
		FontMetrics metrics = ((Graphics2D) g).getFontMetrics(monospace);
		fx = metrics.charWidth('m');
		fy = metrics.getHeight() + 2;
		fmaxascend = metrics.getMaxAscent();
	}
}

static int grid(int x, int y) { return (x << 16) | y; }
static int gridx(int grid) { return grid >>> 16; }
static int gridy(int grid) { return grid & 0xFFFF; }

private final Point translation;
/** in grid coordinates */
private final ArrayDeque<Point> translateStack;
/** in awt coordinates */
private final ArrayDeque<Rectangle> clipStack;
private final HashMap<Integer, String[]> hoverData;
private final HashMap<Integer, Object> clickData;
private final HashMap<Integer, Object> linkData;

public final boolean isTimerTick;
public final int tickCount;
public final Graphics awtg;
public final Point mouse;

public boolean requireTimerRepaint;

CUGfx(
	Graphics awtg, Point mouse, HashMap<Integer, String[]> hoverData,
	HashMap<Integer, Object> clickData, HashMap<Integer, Object> linkData,
	int tickCount, boolean isTimerTick
) {
	this.awtg = awtg;
	this.awtg.setFont(monospace);
	this.mouse = mouse;
	this.translation = new Point();
	this.translateStack = new ArrayDeque<>();
	this.clipStack = new ArrayDeque<>();
	this.hoverData = hoverData;
	this.clickData = clickData;
	this.linkData = linkData;
	this.tickCount = tickCount;
	this.isTimerTick = isTimerTick;
}

public void drawString(int x, int y, String str)
{
	this.awtg.drawString(str, fx * x, fy * y + 1 + fmaxascend);
}

public void fillRect(int x, int y, int w, int h)
{
	this.awtg.fillRect(fx * x, fy * y, fx * w, fy * h);
}

public void setColor(Color c)
{
	this.awtg.setColor(c);
}

public void translate(int x, int y)
{
	this.translation.x += x;
	this.translation.y += y;
	this.awtg.translate(fx * x, fy * y);
}

public boolean isHovered(int x, int y, int w, int h)
{
	x += translation.x; y += translation.y;
	return x <= this.mouse.x && this.mouse.x < x + w && y <= this.mouse.y && this.mouse.y < y + h;
}

public void pushTranslate()
{
	this.translateStack.push(new Point(this.translation));
}

/** @return true when empty after this operation */
public boolean popTranslate()
{
	var t = this.translateStack.pop();
	this.awtg.translate((t.x - this.translation.x) * fx, (t.y - this.translation.y) * fy);
	this.translation.setLocation(t.x, t.y);
	return this.translateStack.isEmpty();
}

public void pushClip(int w, int h)
{
	this.pushClipRaw(fx * w, fy * h);
}

public void pushClipRaw(int awtw, int awth)
{
	this.clipStack.push(new Rectangle(fx * this.translation.x, fy * this.translation.y, awtw, awth));
	this.awtg.setClip(0, 0, awtw, awth);
}

/** @return true when empty after this operation */
public boolean popClip()
{
	this.clipStack.pop();
	var clip = this.clipStack.peek();
	if (clip != null) {
		// unsure why, but first need to undo X translation for clipping to work correctly. And not Y?
		this.awtg.translate(fx * -this.translation.x, 0);
		this.awtg.setClip(0, 0, clip.width, clip.height);
		this.awtg.translate(fx * this.translation.x, 0);
		return false;
	} else {
		this.awtg.setClip(null);
		return true;
	}
}

public boolean rowsInViewport(int h)
{
	// TODO: LOL CHECK CLIP (?)
	int minY = 0, maxY = Short.MAX_VALUE;
	var clip = this.clipStack.peek();
	if (clip != null) {
		minY = (clip.y + fy - 1) / fy;
		maxY = minY + (clip.height + fy - 1) / fy;
	}
	int y = this.translation.y;
	if (y >= CURoot.BOTTOM_TAB_HACK_GRID_START_Y) { // fucking hacks
		minY += CURoot.BOTTOM_TAB_HACK_GRID_START_Y;
		maxY += CURoot.BOTTOM_TAB_HACK_GRID_START_Y;
	}
	return minY <= y && y < maxY || minY <= y + h && y + h < maxY;
}

private <T> void putdata(HashMap<Integer, T> target, int w, int h, T data)
{
	int minX = 0, maxX = Short.MAX_VALUE;
	int minY = 0, maxY = Short.MAX_VALUE;
	var clip = this.clipStack.peek();
	if (clip != null) {
		minX = (clip.x + fx - 1) / fx;
		maxX = minX + (clip.width + fx - 1) / fx;
		minY = (clip.y + fy - 1) / fy;
		maxY = minY + (clip.height + fy - 1) / fy;
	}
	for (int j = 0; j < h; j++) {
		int y = this.translation.y + j;
		if (y >= CURoot.BOTTOM_TAB_HACK_GRID_START_Y) { // fucking hacks
			minY += CURoot.BOTTOM_TAB_HACK_GRID_START_Y;
			maxY += CURoot.BOTTOM_TAB_HACK_GRID_START_Y;
		}
		for (int i = 0; i < w; i++) {
			int x = this.translation.x + i;
			if (minX <= x && x < maxX && minY <= y && y < maxY) {
				target.put(grid(x, y), data);
			}
		}
	}
}

public void clickable(HashMap<Integer, Object> into, int w, int h, CUComponent.Clickable owner, Object clickuserdata)
{
	this.putdata(into, w, h, clickuserdata == null ? owner : new ClickData(owner, clickuserdata));
}

public void clickable(int w, int h, CUComponent.Clickable owner)
{
	this.clickable(this.clickData, w, h, owner, null);
}

public void tooltip(int w, int h, String[] text)
{
	this.putdata(this.hoverData, w, h, text);
}

public void button(CUComponent.Clickable owner, String text, Color normalColor, Color hoverColor, boolean enabled)
{
	button(owner, null, text, normalColor, hoverColor, enabled);
}

public void button(CUComponent.Clickable owner, Object clickuserdata, String text, Color normalColor, Color hoverColor, boolean enabled)
{
	int w = text.length() + 2;
	if (enabled) {
		this.setColor(Color.black);
		this.fillRect(0, 0, w, 1);
		this.setColor(this.isHovered(0, 0, w, 1) ? hoverColor : normalColor);
		this.awtg.fillRect(1, 1, fx * w - 2, fy - 2);
	} else {
		this.setColor(this.isHovered(0, 0, w, 1) ? hoverColor : normalColor);
		this.fillRect(0, 0, w, 1);
	}
	this.setColor(Color.black);
	this.drawString(1, 0, text);
	if (enabled) {
		this.clickable(this.clickData, w, 1, owner, clickuserdata);
	}
}

public void link(CUComponent.Clickable owner, Object clickuserdata, String text)
{
	int w = text.length();
	this.clickable(this.linkData, w, 1, owner, clickuserdata);
	var colorToRestore = this.awtg.getColor();
	this.awtg.setColor(Color.blue);
	if (this.isHovered(0, 0, w, 1)) {
		this.awtg.setColor(colAccent);
		this.fillRect(0, 0, w, 1);
		this.awtg.setColor(Color.white);
	}
	this.drawString(0, 0, text);
	this.awtg.drawLine(0, fmaxascend + 2, w * fx, fmaxascend + 2);
	this.awtg.setColor(colorToRestore);
}

class ClickData
{
CUComponent.Clickable owner;
Object userdata;
ClickData(CUComponent.Clickable owner, Object userdata)
{
	this.owner = owner;
	this.userdata = userdata;
}
} /*ClickData*/
} /*CUGfx*/
