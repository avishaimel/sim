#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

#define MAX_WORDS 0x100000 //2**20 words in main memory
#define WORD 10

// Public functions:

 /**
 * Function that copies the memin.txt contents to the main memory array
 * @param memin: file of memory
 * @param main_memory: array of main memory
 */
void set_memin_array(FILE* memin, int* main_memory);
/**
 * Function that copies the content of the main memory to memout.txt
 * @param memout: file of memory out
 * @param main_memory: array of main memory
 */
void write_Memout(FILE *memout, int* main_memory);
/**
 * Function that initializes main memory and allocates the space needed to store it
 * @return Initialized main memory
 */
int* memory_initiation();
/**
 * Function that frees the main memory and the space allocated for it
 * @param main_memory
 */
void free_memory(int* main_memory);