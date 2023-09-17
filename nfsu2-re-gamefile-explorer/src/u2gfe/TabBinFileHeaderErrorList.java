package u2gfe;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.util.ArrayList;

import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.border.MatteBorder;

class TabBinFileHeaderErrorList extends Box
{
TabBinFileHeaderErrorList(ArrayList<Throwable> errors)
{
	super(BoxLayout.Y_AXIS);

	Dimension minLblSize = new Dimension(1, 1);

	for (Throwable error : errors) {
		JButton btn = new JButton("details");
		btn.addActionListener(e -> Messages.SHOW_ERROR.send(error));

		JLabel lbl = new JLabel(error.getMessage());
		lbl.setMinimumSize(minLblSize);
		lbl.setPreferredSize(minLblSize);
		lbl.setForeground(Color.white);

		JPanel pnl = new JPanel(new GridBagLayout());
		pnl.setBorder(new MatteBorder(0, 0, 1, 0, Color.red));
		pnl.setBackground(Color.red.darker());
		GridBagConstraints c = new GridBagConstraints();
		c.gridx = 1;
		pnl.add(btn, c);
		c.gridx = 0;
		c.weightx = 1d;
		c.insets.right = c.insets.left = 5;
		c.fill = GridBagConstraints.BOTH;
		pnl.add(lbl, c);
		this.add(pnl);
	}
}
}
