package u2gfe;

import java.io.File;
import java.nio.file.Files;

class GameFileParser
{
/**
 * The only real essential file to parse is a language file (we use English.bin ofc),
 * because parsing other data use strings from the language file to annotate data.
 *
 * The others are mainly interesting or very important files that form the backbone of data.
 */
static String essentialFiles[] = {
	"Languages/English.bin", // should be first so we have strings :)
	"GLOBAL/GLOBALB.BUN",
	"GLOBAL/InGameRace.bun",
	"TRACKS/ROUTESL4RA/Paths4001.bin",
};

public static void parseEssentialFiles(File gamedir)
{
	String rootPath = gamedir.getAbsolutePath();
	for (String relPath : essentialFiles) {
		File file = new File(gamedir, relPath);
		String filepath = file.getAbsolutePath();
		String relpath = filepath.substring(rootPath.length());
		byte content[];
		try {
			content = Files.readAllBytes(file.toPath());
		} catch (Throwable t) {
			Messages.SHOW_ERROR.send(new Exception("failed to read " + relpath, t));
			continue;
		}
		try {
			Messages.GAME_FILE_PARSED.send(new BinFile(file.getName(), relPath, content));
		} catch (Throwable t) {
			Messages.SHOW_ERROR.send(new Exception("failed to parse " + relpath, t));
		}
	}
	Symbol.distribute_references();
}

public static void collectFiles(File gamedir)
{
}
}