package u2gfe;

import java.awt.Container;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;

import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JTabbedPane;
import javax.swing.border.Border;

class Util
{
static ByteBuffer bb = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN);

static int i32(byte data[], int at)
{
	bb.rewind();
	bb.put(data, at, 4);
	bb.rewind();
	return bb.getInt();
}

static int i16(byte data[], int at)
{
	bb.rewind();
	bb.put(data, at, 2);
	bb.rewind();
	return bb.getShort();
}

static int i8(byte data[], int at)
{
	return data[at] & 0xFF;
}

static float f32(byte data[], int at)
{
	bb.rewind();
	bb.put(data, at, 4);
	bb.rewind();
	return bb.getFloat();
}

static String cstr(byte data[], int at, int to)
{
	int end = at;
	for (; end < to && end < data.length && data[end] != 0; end++);
	return new String(data, at, end - at, StandardCharsets.US_ASCII);
}

static <T extends JComponent> T bordered(T component, Border border)
{
	component.setBorder(border);
	return component;
}

static void makeTabLabelMMBClosable(JTabbedPane tabpane, int index)
{
	// this will mess up the hover though... (hovering the text does show the tab as hovered)
	JLabel lblTab = new JLabel(tabpane.getTitleAt(index));
	lblTab.addMouseListener(tabLabelMMBCloseMouseListener);
	tabpane.setTabComponentAt(index, lblTab);
}

static final MouseAdapter tabLabelMMBCloseMouseListener = new MouseAdapter() {
	@Override
	public void mouseReleased(MouseEvent e)
	{
		JComponent src = (JComponent) e.getSource();
		Container par = src.getParent();
		for (; !(par instanceof JTabbedPane); par = par.getParent());
		JTabbedPane tabpane = (JTabbedPane) par;
		int idx = tabpane.indexOfTabComponent(src);
		if (e.getButton() == MouseEvent.BUTTON2) {
			tabpane.removeTabAt(idx);
		} else if (e.getButton() == MouseEvent.BUTTON1) {
			tabpane.setSelectedIndex(idx);
		}
	}
};
}
