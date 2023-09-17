package u2gfe;

import javax.swing.UIManager;

public class U2gfeMain
{
public static void main(String args[])
{
	try {
		UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
	} catch (Throwable t) {}

	new GameFileParser();
	new WindowMain();
}
}
