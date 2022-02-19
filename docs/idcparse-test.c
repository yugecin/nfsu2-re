#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "idcparse.c"

#define IDCPATH "../../NeedForSpeed U2/SPEED2.idc"

int main(int argc, char **argv)
{
	struct idcparse *res;
	FILE *in;
	char *buf;
	int size;

	res = malloc(sizeof(struct idcparse));
	assert(((void)"failed to malloc for idcparse", res));
	in = fopen(IDCPATH, "rb");
	if (!in) {
		printf("failed to open file "IDCPATH"\n");
		exit(1);
	}
	fseek(in, 0l, SEEK_END);
	size = ftell(in);
	rewind(in);
	buf = (char*) malloc(size);
	assert(buf);
	fread(buf, 1, size, in);
	fclose(in);
	if (size > 2 && buf[0] == (char) 0xEF) {
		/*assume UTF8 BOM*/
		idcparse(res, buf + 3, size - 3);
	} else {
		idcparse(res, buf, size);
	}
	free(buf);
	return 0;
}
