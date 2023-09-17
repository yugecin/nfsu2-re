package u2gfe;

import java.awt.BorderLayout;
import java.awt.Color;
import java.io.PrintWriter;
import java.io.StringWriter;

import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.border.EmptyBorder;

class TabError extends JScrollPane
{
TabError(Throwable error)
{
	super(new JPanel(new BorderLayout()));

	StringWriter sw = new StringWriter();
	error.printStackTrace(new PrintWriter(sw));

	JTextArea ta = new JTextArea(sw.toString());
	ta.setBorder(new EmptyBorder(5, 5, 5, 5));
	ta.setEditable(false);
	ta.setBackground(Color.red.darker());
	ta.setForeground(Color.white);

	((JPanel) this.getViewport().getView()).add(ta);
}
}
