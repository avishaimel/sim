#include "core.h"
#include "bus.h"




int request(Cache* cache, int opcode, int address, int rd_value, int coreID, int cycle, bool* isStall);

typedef struct Bridge {
	Cache *cache_array[4];
	bus* main_bus;
	int** stalls; // [0] --> taking care (1), occupied(-1), free(0), [1] --> number of stalls to update (subtract) each time
	FILE* bus_trace;
}Bridge;

typedef struct Cache_instruction {
	int coreID;
	int opcode;
	int rd; //To store values of these registers
	int address;
	int tag;
	int index;
	int mesi_state;
	char* mesi_transaction;
}Cache_inst;

Cache_inst current_request;
Bridge* main_bridge;

//Functions:
void bridge_initiation(FILE* bus_trace, Cache* Cache0, Cache* Cache1, Cache* Cache2, Cache* Cache3, bus* main_bus);
void free_bridge(Bridge* main_bridge);
int connection_between_bus_and_cache(Cache_inst current_request, int coreID, int cycle, int * stall_counter, int * result_LL, int tagConflict);



