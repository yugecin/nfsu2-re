package u2gfe;

import java.awt.EventQueue;

import javax.swing.UIManager;

public class U2gfeMain
{
public static void main(String args[])
{
	try {
		UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
	} catch (Throwable t) {}

	new WindowMain();

	Enum.init();
	Messages.GAME_DIR_SELECTED.subscribe(dir -> {
		scheduleTask(() -> {
			Messages.START_PARSING.send(null);
			GameFileParser.parseEssentialFiles(dir);
			GameFileParser.collectFiles(dir);
			Messages.DONE_PARSING.send(null);
		});
	});

	// task mechanism below

	currentTask = new ScheduledTask(null);
	currentTask.done = true;
	lastTask = currentTask;
	for (;;) {
		if (!currentTask.done) {
			currentTask.done = true;
			try {
				currentTask.task.run();
			} catch (Throwable t) {
				System.err.printf("task error (%s)%n", currentTask.task);
				t.printStackTrace();
			}
		}
		if (currentTask.next != null) {
			currentTask = currentTask.next;
			continue;
		}
		try {
			Thread.sleep(1000);
		} catch (InterruptedException e) {}
	}
}

static Thread taskThread = Thread.currentThread();
static ScheduledTask currentTask, lastTask;

private static void scheduleTask(Runnable body)
{
	assert EventQueue.isDispatchThread();
	lastTask.next = new ScheduledTask(body);
	lastTask = lastTask.next;
	taskThread.interrupt();
}

private static class ScheduledTask
{
	Runnable task;
	boolean done;
	ScheduledTask next;

	ScheduledTask(Runnable task)
	{
		this.task = task;
	}
}
}
