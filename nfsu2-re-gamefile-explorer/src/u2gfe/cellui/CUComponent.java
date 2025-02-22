package u2gfe.cellui;

import java.awt.*;

public abstract class CUComponent
{
/** called right before paint (and possibly at other times)
    @return the required height for this component, may exceed passed availableHeight */
public abstract int calculateHeight(int givenWidth, int availableHeight);
/** called right before paint (and possibly at other times)
    @return the required width for this component, may exceed passed availableWidth*/
public abstract int getWidth(int availableWidth);
public abstract void paint(CUGfx g, int givenWidth, int givenHeight);

public interface Clickable
{
static int ONCLICK_REPAINT = 1;
/**
 * Note that some actions you may want to
 * do in a {@link EventQueue#invokeLater} in order
 * to let the mouse event dispatch and handle everything first.
 *
 * @return combination of {@code ONCLICK_} variables
 */
int onClick(int grid, int button, Object userdata);
} /*Clickable*/
} /*CUComponent*/
