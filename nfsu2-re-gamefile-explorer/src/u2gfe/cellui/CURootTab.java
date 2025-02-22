package u2gfe.cellui;

import java.awt.*;
import java.util.function.*;

import static u2gfe.cellui.CUGfx.*;

class CURootTab implements CUComponent.Clickable
{
CURoot root;
boolean closable;
CUComponent header, content;
int headerMarqueeOffset, headerMarqueeDirection;
Point offset, maxOffset;

CURootTab(CURoot root, CUComponent header, CUComponent content, boolean closable)
{
	this.root = root;
	this.header = header;
	this.content = content;
	this.closable = closable;
	this.headerMarqueeDirection = -1;
	this.offset = new Point(0, 0);
	this.maxOffset = new Point(0, 0);
}

void paintHeader(CUGfx g, int w, boolean isTabTitleTopOfScreen)
{
	// all the +1's in the width are because tabs take extra space of the window that are not full grid cells

	int closeButtonWidth = this.closable ? 2 : 0;
	g.clickable(w + 1, 1, this);
	// bg
	g.setColor(g.isHovered(closeButtonWidth, 0, w + 1 - closeButtonWidth, 1) ? colAccent : colTabBg);
	g.fillRect(0, 0, w + 1, 1);
	// close btn
	if (this.closable) {
		g.setColor(g.isHovered(0, 0, closeButtonWidth, 1) ? colRedHover : colRedStatic);
		g.fillRect(0, 0, closeButtonWidth, 1);
	}
	// header content
	int reservedWidth = 3;
	g.translate(reservedWidth, 0);
	int aw = w - reservedWidth;
	g.pushClip(aw, 1);
	g.pushTranslate();
	{
		// marquee the header contents if it doesn't fit
		int overflowWidth = this.header.getWidth(aw) - aw;
		if (overflowWidth > 0) {
			g.requireTimerRepaint = true;
			if (g.isTimerTick && (g.tickCount % 2) == 0) {
				this.headerMarqueeOffset += this.headerMarqueeDirection;
				if (this.headerMarqueeOffset > 0) {
					this.headerMarqueeOffset = 0;
					this.headerMarqueeDirection = -1;
				} else if (this.headerMarqueeOffset < -overflowWidth) {
					this.headerMarqueeOffset = -overflowWidth;
					this.headerMarqueeDirection = 1;
				}
			}
			g.translate(this.headerMarqueeOffset, 0);
		}
	}
	this.header.paint(g, aw, 1);
	g.popTranslate();
	g.popClip();
	g.translate(-reservedWidth, 0);
	// border-bottom (or top if below/after active tab) (render after header is rendered so it's always on top)
	g.setColor(colAccent);
	g.awtg.fillRect(0, isTabTitleTopOfScreen ? fy - 1 : 0, fx * (w + 1), 1);
}

void paintContent(CUGfx g, int givenWidth, int givenHeight, Point awtExtraSize)
{
	boolean vscroll = false, hscroll = false;
	int rh = this.content.calculateHeight(givenWidth, givenHeight);
	if (rh > givenHeight) {
		rh = this.content.calculateHeight(givenWidth - 2, givenHeight);
		vscroll = true;
	}
	int rw = this.content.getWidth(givenWidth);
	if (rw > (vscroll ? givenWidth - 2 : givenWidth)) {
		if (rh > givenHeight - 1) {
			vscroll = true;
		}
		rw = this.content.getWidth(givenHeight - 1);
		hscroll = true;
	}

	if (vscroll) {
		g.setColor(colScroll);
		g.awtg.fillRect(0, 0, fx * 2, fy * givenHeight + awtExtraSize.y);
	}
	if (hscroll) {
		g.setColor(colScroll);
		g.awtg.fillRect(0, fy * givenHeight + awtExtraSize.y - fy, fx * givenWidth + awtExtraSize.x, fy);
	}

	g.pushTranslate();
	if (vscroll) {
		givenWidth -= 2;
		g.translate(2, 0);
	}
	if (hscroll) {
		givenHeight -= 1;
	}
	this.maxOffset.x = hscroll ? rw - givenWidth : 0;
	this.maxOffset.y = vscroll ? rh - givenHeight : 0;
	this.offset.x = hscroll ? Math.min(this.offset.x, this.maxOffset.x) : 0;
	this.offset.y = vscroll ? Math.min(this.offset.y, this.maxOffset.y) : 0;
	if (vscroll) {
		g.setColor(colBg);
		int awtgivenHeight = fy * givenHeight + awtExtraSize.y;
		int extent = Math.max(fy, (int) (awtgivenHeight * (float) givenHeight / rh));
		int offset = (int) ((awtgivenHeight - extent) * ((float) this.offset.y / this.maxOffset.y));
		g.awtg.fillRect(fx * -2, offset, fx * 2, extent);
		g.setColor(colScroll);
		g.awtg.fillRect(-1, 0, 1, awtgivenHeight);
	}
	if (hscroll) {
		int y = fy * givenHeight + awtExtraSize.y;
		g.setColor(colBg);
		int awtgivenWidth = fx * givenWidth + awtExtraSize.x;
		int extent = Math.max(fx, (int) (awtgivenWidth * (float) givenWidth / rw));
		int offset = (int) ((awtgivenWidth - extent) * ((float) this.offset.x / this.maxOffset.x));
		g.awtg.fillRect(offset, y, extent, fy);
		g.setColor(colScroll);
		g.awtg.fillRect(0, y, fx * givenWidth + awtExtraSize.x, 1);
	}
	g.pushClipRaw(fx * givenWidth + awtExtraSize.x, fy * givenHeight + awtExtraSize.y);
	g.translate(-this.offset.x, -this.offset.y);
	this.content.paint(g, givenWidth + 1, givenHeight + 1);
	g.popClip();
	g.popTranslate();
}

/** @return false if unchanged */
boolean applyScrollValue(int unitsScrolled, ToIntFunction<Point> getV, IntConsumer setV)
{
	var ev = getV.applyAsInt(this.offset);
	var nv = Math.max(0, Math.min(ev + unitsScrolled, getV.applyAsInt(this.maxOffset)));
	setV.accept(nv);
	return nv != ev;
}

@Override
public int onClick(int grid, int button, Object userdata)
{
	if (this.closable && (gridx(grid) < 2 || button == 2)) {
		this.root.removeTab(this.content);
		return ONCLICK_REPAINT;
	}
	if (this.root.activeTab != this) {
		this.root.activeTab = this;
		return ONCLICK_REPAINT;
	}
	return 0;
}
} /*CURootTab*/