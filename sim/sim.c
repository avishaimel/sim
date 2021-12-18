#include "files_handler.h"
#include "pipeline.h"
#include "bus.h"
#include "bridge.h"
#include "sim.h"
#include "memory.h"

#pragma warning(disable:4996)

#define NUM_OF_ARGUMENTS 28
#define MAXIMUM_CHARS_IN_LINE 10

/*declaration of functions*/
void cyclesSimulation(Core* core, int cycleNumber, FILE** trace);
void Load_inst_into_Core(char commands[10], FILE* imem, Core* core);

int main(int argc, char* argv[])
{
	int index = 0;
	FILE* imem[CORE_NUM];
	FILE* meminf;
	FILE* memoutf;
	FILE* regout[CORE_NUM];
	FILE* core_trace[CORE_NUM];
	FILE* bus_trace;
	FILE* dsram[CORE_NUM];
	FILE* tsram[CORE_NUM];
	FILE* stats[CORE_NUM];
	char commands[MAXIMUM_CHARS_IN_LINE];
	int cycleNumber = -1;
	int* main_memory;
	bus* main_bus;
	Cache* caches[4];
	if (argc > 1 && argc != NUM_OF_ARGUMENTS)
	{
		printf("Wrong number of arguments");
		return 1;
	}
	meminf = open_Memin(argc, argv);
	memoutf = open_MemOut(argc, argv);
	bus_trace = open_BusTrace(argc, argv);
	main_memory = memory_initiation();
	set_memin_array(meminf, main_memory);
	main_bus = bus_initiation(main_memory);


	for (int i = 0; i < CORE_NUM; i++){
		caches[i] = cache_initiation(i);
	}
	main_bridge = (Bridge *)malloc(sizeof(Bridge));
	if (main_bridge == NULL)
	{
		printf("Couldn't allocate space for the bridge");
		return 1;
	}
	bridge_initiation(bus_trace, caches[0], caches[1], caches[2], caches[3], main_bus);
	Core* cores_array = cores_initiation(caches);
	for (int i = 0; i < CORE_NUM; i++)
	{
		core_trace[i] = open_Trace(i, argc, argv);
		imem[i] = open_Imem(i, argc, argv);
		regout[i] = open_Regout(i, argc, argv);
		stats[i] = open_Stats(i, argc, argv);
		tsram[i] = open_Tsrams(i, argc, argv);
		dsram[i] = open_Dsrams(i, argc, argv);
		Load_inst_into_Core(commands, imem[i], &(cores_array[i]));
	}
	cyclesSimulation(cores_array, cycleNumber, core_trace);

	write_RegOut(cores_array, regout); /* writing the data to the registers file */

	for (int i = 0; i < CORE_NUM; i++) { /* writing the data to the rams files - tsram and dsram */
		write_Rams(tsram[i], dsram[i], cores_array[i].Cache);
	}

	write_Statistics(cores_array, stats); /* writing the statistics to the stats files */


	// code for loading the modified data in the cache to check if our assembly code is valid
	/*for (int i = 0; i < NUM_OF_CORES; i++) {
		for (int j = 0; j < 256; j++) {
			if (main_bridge->caches[i]->tsram[j]->msi_state == 2) {
				int address = (main_bridge->caches[i]->tsram[j]->tag << 8) + j;
				main_bridge->main_bus->main_memory[address] = main_bridge->caches[i]->dsram[j];
			}
		}

	}*/

	write_Memout(memoutf, main_memory); /* writing the memory data to the memout file */

	colse_all_Files(core_trace, imem, regout, stats, tsram, dsram, meminf, memoutf, bus_trace);/* closes the files for writing */
	/*free the allocated objects*/
	free_Cores(cores_array);
	free_bus(main_bus);
	free_bridge(main_bridge);
	return 0;
}

void Load_inst_into_Core(char commands[10], FILE* imem, Core* current_core)
{
	int i = 0;
	while (fgets(commands, MAXIMUM_CHARS_IN_LINE, imem) != NULL)
	{
		int commandExtract = strtol(commands, NULL, 16);
		current_core->instructions[i].imm = commandExtract & 0x00000FFF;
		current_core->instructions[i].rt = (commandExtract & 0x0000F000) >> 0xC;
		current_core->instructions[i].rs = (commandExtract & 0x000F0000) >> 0x10;
		current_core->instructions[i].rd = (commandExtract & 0x00F00000) >> 0x14;
		current_core->instructions[i].opcode = (commandExtract & 0xFF000000) >> 0x18;
		i += 1;
	}
}

/**
 * The function is responsible for returning flag that will tell us whether to stop the program from running or not
 * @param cores - all the cores objects
 * @return bool variable - will tell the program whether to finish all the cores cycles or not
 */
bool stopProgram(Core *cores) {
	bool result = true;
	for (int i = 0; i < CORE_NUM; i++) {
		result = result && (cores[i].state.haltProgram);
	}
	return result;
}

void cyclesSimulation(Core* cores_array, int numOfCycle, FILE** trace)
{
	while (!stopProgram(cores_array))
	{
		numOfCycle++;
		for (int i = 0; i < CORE_NUM; i++)
		{
			if (cores_array[i].state.haltProgram)
				continue;
			WriteBack(cores_array[i].current_state_Reg, &(cores_array[i]), cores_array[i].stallData, numOfCycle);
			MEM(cores_array[i].current_state_Reg, &(cores_array[i]), cores_array[i].stallData, numOfCycle);
			EX(cores_array[i].current_state_Reg, &(cores_array[i]), cores_array[i].stallData);
			Decode(cores_array[i].current_state_Reg, &(cores_array[i]), cores_array[i].stallData);
			Fetch(cores_array[i].current_state_Reg, &(cores_array[i]), cores_array[i].stallData);
			write_Core_Trace(trace[i], &(cores_array[i]), numOfCycle);
			moveDtoQ(&(cores_array[i]));
			cores_array[i].state.PC = cores_array[i].current_state_Reg->if_id->New_PC;
		}
	}
}
