package u2gfe;

import java.util.ArrayList;

import static u2gfe.Util.i32;

class BinFile
{
String name;
String path;
byte data[];
ArrayList<Section> sections = new ArrayList<>();
/** Also contains errors of all {@link #sections} */
ArrayList<Throwable> errors = new ArrayList<>();

BinFile(String name, String path, byte data[])
{
	this.name = name;
	this.path = path;
	this.data = data;

	int offset = 0;
	while (offset < data.length - 8) {
		int magic = i32(data, offset);
		int size = i32(data, offset + 4);
		Section section = new Section(this, data, offset + 8, magic, size);
		this.sections.add(section);
		for (Throwable t : section.errors) {
			String msg = String.format("section %Xh@%Xh: %s", magic, offset, t.getMessage());
			this.errors.add(new Throwable(msg, t));
		}
		offset += size + 8;
	}
	if (offset != data.length) {
		this.errors.add(new Throwable(String.format("%Xh trailing bytes from offset %Xh", data.length - offset, offset)));
	}
}

@Override
public String toString()
{
	return String.format("%s: %d sections, size %Xh", this.name, this.sections.size(), this.data.length);
}

static class Section
{
int magic, size;
/** offset in {@link #data} where the data of this section starts (so without 8 bytes header) */
int offset;
/** header for this section starts at index {@link #offset} - 8 */
byte data[];
ArrayList<Section> subsections;
/** Also contains errors of all {@link #subsections} */
ArrayList<Throwable> errors = new ArrayList<>();
Object[] fields;

Section(BinFile file, byte data[], int offset, int magic, int size)
{
	this.data = data;
	this.offset = offset;
	this.magic = magic;
	this.size = size;

	if ((magic & 0x80000000) != 0) {
		this.subsections = new ArrayList<>();
		offset = 0;
		while (offset < this.size) {
			magic = i32(data, this.offset + offset);
			size = i32(data, this.offset + offset + 4);
			Section sub = new Section(file, data, this.offset + offset + 8, magic, size);
			this.subsections.add(sub);
			for (Throwable t : sub.errors) {
				String msg = String.format("subsection %Xh@%Xh+%Xh: %s", magic, this.offset, offset, t.getMessage());
				this.errors.add(new Throwable(msg, t));
			}
			offset += size + 8;
		}
		if (offset != this.size) {
			String msg = String.format("%Xh trailing bytes from offset %Xh", this.size - offset, offset);
			this.errors.add(new Throwable(msg));
		}
	}
	ParseState ps = Structures.parse(file, this.magic, this.data, this.offset, this.size);
	this.fields = ps.result.toArray();
	this.errors.addAll(ps.errors);
}

@Override
public String toString()
{
	if (this.subsections.isEmpty()) {
		return String.format("%Xh (%Xh)", this.magic, this.size);
	}
	return String.format("%Xh (%Xh, %d subsections)", this.magic, this.size, this.subsections.size());
}
} /*Section*/
}
