package u2gfe;

import java.awt.*;
import javax.swing.*;

public class U2gfeMain
{
public static void main(String args[])
{
	try {
		UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
	} catch (Throwable t) {}

	Tasks.init();
	Enum.init();
	new WindowMain();
	Tasks.execPump();
}
} /*U2gfeMain*/

class Tasks
{
static Thread taskThread;
static ScheduledTask lastTask, currentTask;

static void init()
{
	taskThread = Thread.currentThread();
	currentTask = new ScheduledTask(null);
	currentTask.done = true;
	lastTask = currentTask;
}

/** takes over the current thread to become the task thread */
static void execPump()
{
	for (;;) {
		if (!currentTask.done) {
			currentTask.done = true;
			try {
				currentTask.task.run();
			} catch (Throwable t) {
				System.err.printf("task error (%s)%n", currentTask.task);
				t.printStackTrace();
				currentTask.scheduleStack.printStackTrace();
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

static void scheduleTask(Runnable body)
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
Throwable scheduleStack;

ScheduledTask(Runnable task)
{
	this.task = task;
	this.scheduleStack = new Exception("<task schedule stacktrace>");
}
} /*ScheduledTask*/
} /*Tasks*/
