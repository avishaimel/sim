#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

#define MAX_WORDS 0x100000 //2**20 words in main memory
#define WORD 10





// Public functions:

int* memory_initiation();
void set_memin_array(FILE* memin, int* main_memory);
void write_Memout(FILE *memout, int* main_memory);
void free_memory(int* main_memory);