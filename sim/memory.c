#include "memory.h"
#pragma warning(disable:4996)

#define MEM_SIGN_EXTEND_NUM 32


int* memory_initiation() {
	int* memory_array = NULL;
	memory_array = (int *)malloc(sizeof(int) * MAX_WORDS);
	if (memory_array == NULL) {
		printf("An error occurred while allocating memory for the bus");
		exit(1); /*exiting the program after an error occured */
	}
	for (int i = 0; i < MAX_WORDS; i++) {
		memory_array[i] = 0;
	}
	return memory_array;
}

void set_memin_array(FILE* memin, int* main_memory) {
	char buffer[WORD];
	char *bufferPointer = buffer;
	int i = 0;
	while (true) {  //place input file lines into array
		fgets(bufferPointer, WORD, memin);
		main_memory[i] = (int)strtoll(bufferPointer, NULL, 16);
		if (feof(memin)) {
			break;
		}
		i += 1;
	}
	while (i < MAX_WORDS) { // continuing placing lines until limit
		main_memory[i] = 0;
		i += 1;
	}
}

int index_for_memout(int* main_memory) {
	int i = MAX_WORDS - 1;
	for (; i > 0; i--) {
		if (main_memory[i] != 0) {
			return i;
		}
	}
	return i;
}

void write_Memout(FILE *memout, int* main_memory) {
	int max_index = index_for_memout(main_memory);
	for (int j = 0; j < max_index + 1; j++) {
		fprintf(memout, "%08X\n", main_memory[j]);
	}
}

void free_memory(int* main_memory) {
	free(main_memory);
}
