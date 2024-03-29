package u2gfe;

import java.io.File;
import java.nio.file.Files;

class GameFileParser implements Runnable
{
private String files[] = {
	"Languages/English.bin", // should be first so we have strings :)
	"GLOBAL/GLOBALB.BUN",
	"GLOBAL/InGameRace.bun",
	"TRACKS/ROUTESL4RA/Paths4001.bin",
};
private File gamedir;

GameFileParser()
{
	Messages.GAME_DIR_SELECTED.subscribe(dir -> {
		this.gamedir = dir;
		Messages.START_PARSING.send(null);
		new Thread(this).start();
	});
}

@Override
public void run()
{
	File gamedir = this.gamedir;
	try {
		Enum.init();
		for (String relPath : files) {
			File file = new File(gamedir, relPath);
			byte content[] = Files.readAllBytes(file.toPath());
			Messages.GAME_FILE_PARSED.send(new BinFile(file.getName(), content));
		}
	} catch (Throwable t) {
		Messages.SHOW_ERROR.send(new Exception("Parse failure", t));
	}
	Symbol.distribute_references();
	Messages.DONE_PARSING.send(null);
}
}
