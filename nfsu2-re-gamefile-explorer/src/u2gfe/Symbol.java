package u2gfe;

import java.util.*;

import static u2gfe.Util.*;

class Symbol implements Comparable<Symbol>
{
static ArrayList<Reference> outstandingRefs = new ArrayList<>();
static HashMap<Integer, Symbol> symbols = new HashMap<>();
static ArrayList<Symbol> symbolsInNaturalOrder = new ArrayList<>();

static void clearSymbols()
{
	outstandingRefs.clear();
	symbols.clear();
	symbolsInNaturalOrder.clear();
}

static void put(BinFile file, int offset, String kind)
{
	int id = i32(file.data, offset);
	if (id == 0) {
		return;
	}
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

static void reference(String referencerName, BinFile file, int offset)
{
	int to = i32(file.data, offset);
	outstandingRefs.add(new Reference(to, referencerName, file, offset));
}

static void distributeReferences()
{
	for (Reference ref : outstandingRefs) {
		Symbol sym = symbols.get(ref.to);
		if (sym != null) {
			sym.references.add(ref);
		}
	}
	outstandingRefs.clear();
}

ArrayList<Reference> references = new ArrayList<>();
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
	return String.format("%8X: %s (%d refs)", this.id, this.name, this.references.size());
}

/*Comparable*/
@Override
public int compareTo(Symbol o)
{
	// unsiged compare yay
	return (this.id & 0xFFFFFFFFL) > (o.id & 0xFFFFFFFFL) ? 1 : -1;
}

static class Reference
{
String referencerName;
BinFile file;
int offset;
int to;
Reference(int to, String referencerName, BinFile file, int offset)
{
	this.to = to;
	this.referencerName = referencerName;
	this.file = file;
	this.offset = offset;
}

@Override
public String toString()
{
	return this.referencerName;
}
} /*Reference*/
} /*Symbol*/
