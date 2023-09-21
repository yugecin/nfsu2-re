package u2gfe;

import static u2gfe.Util.*;

import java.util.ArrayList;
import java.util.HashMap;

class Symbol implements Comparable<Symbol>
{
static HashMap<Integer, Symbol> symbols = new HashMap<>();
static ArrayList<Symbol> symbolsInNaturalOrder = new ArrayList<>();

static void put(BinFile file, int offset, String kind)
{
	int id = i32(file.data, offset);
	Symbol sym = new Symbol(id, file, offset, kind);
	Symbol previous = symbols.put(id, sym);
	if (previous != null) {
		// ... yeah some sections may be partially duplicated in game data, and it seems
		// like the first section is the one that is used. So for duplicate definitions,
		// put the original back and discard the new one.
		symbols.put(id, previous);
	} else {
		symbolsInNaturalOrder.add(sym);
	}
}

int id;
BinFile file;
int definitionOffset;
String name;

Symbol(int id, BinFile file, int definitionOffset, String name)
{
	this.id = id;
	this.file = file;
	this.definitionOffset = definitionOffset;
	this.name = name;
}

@Override
public String toString()
{
	return String.format("%8X: %s", this.id, this.name);
}

/*Comparable*/
@Override
public int compareTo(Symbol o)
{
	// unsiged compare yay
	return (this.id & 0xFFFFFFFFL) > (o.id & 0xFFFFFFFFL) ? 1 : -1;
}
}
