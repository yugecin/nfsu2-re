package u2gfe;

import java.util.HashMap;

import static u2gfe.Util.*;

class Common
{
static HashMap<Integer, String> carnames = new HashMap<>();

static
{
	carnames.put(cshash("PEUGOT"), "PEUGOT");
	carnames.put(cshash("FOCUS"), "FOCUS");
	carnames.put(cshash("COROLLA"), "COROLLA");
	carnames.put(cshash("240SX"), "240SX");
	carnames.put(cshash("MIATA"), "MIATA");
	carnames.put(cshash("CIVIC"), "CIVIC");
	carnames.put(cshash("PEUGOT106"), "PEUGOT106");
	carnames.put(cshash("CORSA"), "CORSA");
	carnames.put(cshash("HUMMER"), "HUMMER");
	carnames.put(cshash("NAVIGATOR"), "NAVIGATOR");
	carnames.put(cshash("ESCALADE"), "ESCALADE");
	carnames.put(cshash("TIBURON"), "TIBURON");
	carnames.put(cshash("SENTRA"), "SENTRA");
	carnames.put(cshash("CELICA"), "CELICA");
	carnames.put(cshash("IS300"), "IS300");
	carnames.put(cshash("SUPRA"), "SUPRA");
	carnames.put(cshash("GOLF"), "GOLF");
	carnames.put(cshash("A3"), "A3");
	carnames.put(cshash("RSX"), "RSX");
	carnames.put(cshash("ECLIPSE"), "ECLIPSE");
	carnames.put(cshash("TT"), "TT");
	carnames.put(cshash("RX8"), "RX8");
	carnames.put(cshash("350Z"), "350Z");
	carnames.put(cshash("G35"), "G35");
	carnames.put(cshash("3000GT"), "3000GT");
	carnames.put(cshash("GTO"), "GTO");
	carnames.put(cshash("MUSTANGGT"), "MUSTANGGT");
	carnames.put(cshash("SKYLINE"), "SKYLINE");
	carnames.put(cshash("LANCEREVO8"), "LANCEREVO8");
	carnames.put(cshash("RX7"), "RX7");
	carnames.put(cshash("IMPREZAWRX"), "IMPREZAWRX");
	carnames.put(cshash("TAXI"), "TAXI");
	carnames.put(cshash("4DR_SEDAN02"), "4DR_SEDAN02");
	carnames.put(cshash("AMBULANCE"), "AMBULANCE");
	carnames.put(cshash("TAXI02"), "TAXI02");
	carnames.put(cshash("PANELVAN"), "PANELVAN");
	carnames.put(cshash("COUPE"), "COUPE");
	carnames.put(cshash("FIRETRUCK"), "FIRETRUCK");
	carnames.put(cshash("PARCELVAN"), "PARCELVAN");
	carnames.put(cshash("PICKUP"), "PICKUP");
	carnames.put(cshash("4DR_SEDAN"), "4DR_SEDAN");
	carnames.put(cshash("BUS"), "BUS");
	carnames.put(cshash("SUV"), "SUV");
	carnames.put(cshash("MINIVAN"), "MINIVAN");
	carnames.put(cshash("HATCHBACK"), "HATCHBACK");
	carnames.put(cshash("HATCHBACK02"), "HATCHBACK02");
}

static String carNameByHash(int hash)
{
	String name = carnames.get(hash);
	if (name == null) {
		return String.format("%08Xh", hash);
	}
	return name;
}
}
