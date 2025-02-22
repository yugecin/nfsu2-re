package u2gfe;

import java.io.*;
import java.nio.file.*;
import java.util.*;

class GamefileParser
{
/**
 * Files that contain data that is essential while interpreting data from other files.
 *
 * The only real essential file to parse is a language file (we use English.bin ofc),
 * because parsing other data use strings from the language file to annotate data.
 *
 * The others are mainly interesting or very important files that form the backbone of data.
 * (maybe GLOBALB.BUN is also kind of essential because other files reference things
 * that are defined in there)
 */
static String ESSENTIAL_FILES_RELATIVEPATHS[] = {
	"Languages/English.bin", // should be first so we have strings :)
	"GLOBAL/GLOBALB.BUN",
	"GLOBAL/InGameRace.bun",
	"TRACKS/ROUTESL4RA/Paths4001.bin",
};

/**
 * key is relative filepath
 * entries might still be parsing, see {@link BinFile#isParsed}
 */
HashMap<String, BinFile> activeFiles = new HashMap<>();
FileWrapper rootWrapper;
final File gamedir;

/**
 * @parma gamedir must be set to the result of {@link File#getCanonicalFile}
 */
GamefileParser(File gamedir)
{
	this.gamedir = gamedir;
}

void collectFiles()
{
	Messages.PARSER_COLLECT_FILES_START.send(null);
	try {
		throw new IOException("hi");
		//rootWrapper = new FileWalker(gamedir).walk();
		//Messages.PARSER_COLLECT_FILES_DONE.send(Result.ok(rootWrapper));
	} catch (IOException e) {
		Messages.PARSER_COLLECT_FILES_DONE.send(Result.err(e));
	}
}

void parseEssentialFiles()
{
}

BinFile getOrCreateBinFileFor(File file)
{
	String rootPath = gamedir.getAbsolutePath();
	String filepath = file.getAbsolutePath();
	String relpath = filepath.substring(rootPath.length());
	if (relpath.charAt(0) == '\\' || relpath.charAt(0) == '/') {
		relpath = relpath.substring(1);
	}
	var binfile = activeFiles.get(relpath);
	if (binfile == null) {
		activeFiles.put(relpath, binfile = new BinFile(file, relpath));
	}
	return binfile;
}

Result<BinFile, Throwable> parseFileSync(BinFile binfile)
{
	if (!binfile.isParsed) {
		byte content[];
		try {
			content = Files.readAllBytes(binfile.file.toPath());
		} catch (Throwable t) {
			this.activeFiles.remove(binfile.path);
			return Result.err(new Exception("failed to read " + binfile.path, t));
		}
		try {
			binfile.parse(content);
		} catch (Throwable t) {
			return Result.err(new Exception("failed to parse " + binfile.path, t));
		}
		Symbol.distributeReferences();
	}
	return Result.ok(binfile);
}
} /*GameFiles*/

class FileWrapper implements Comparable<FileWrapper>
{
static final FileWrapper[] EMPTY_ARRAY = new FileWrapper[0];

File root;
File file;
FileWrapper[] children;

FileWrapper(File root, File file, FileWrapper[] children)
{
	this.root = root;
	this.file = file;
	this.children = children;
}

@Override
public int compareTo(FileWrapper that)
{
	if (this.file.isDirectory() == that.file.isDirectory()) {
		return this.file.getName().compareTo(that.file.getName());
	}
	return this.file.isDirectory() ? 1 : -1;
}
} /*FileWrapper*/

class FileWalker
{
HashSet<String> listedCanonicalPaths;
File root;

FileWalker(File root)
{
	this.root = root;
}

FileWrapper walk() throws IOException
{
	listedCanonicalPaths = new HashSet<>();
	return walk(root);
}

private FileWrapper walk(File folder) throws IOException
{
	var children = new ArrayList<FileWrapper>();
	for (File child : folder.listFiles((FileFilter) null)) {
		if (listedCanonicalPaths.add(child.getCanonicalPath())) {
			if (child.isDirectory()) {
				children.add(this.walk(child));
			} else if (child.isFile()) {
				children.add(new FileWrapper(root, child, FileWrapper.EMPTY_ARRAY));
			}
		}
	}
	children.sort(null);
	return new FileWrapper(root, folder, children.toArray(FileWrapper[]::new));
}
} /*FileWalker*/