#pragma once
#include "io.h"
#pragma warning(disable:4996)



FILE* openMeminFile(int argc, char** argv) {
	FILE* fd = NULL;
	char* filepath = MEMIN;
	if (argc > 1)
		filepath = argv[5];
	fd = fopen(filepath, "r");
	if (fd == NULL) {
		printf("An error occurred while trying to open the mem in file for reading");
		exit(1);
	}
	return fd;
}