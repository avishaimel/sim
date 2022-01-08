#include "files_handler.h"
#include "pipeline.h"
#include "bus.h"
#include "memory.h"

#pragma warning(disable:4996)

#define NUM_OF_ARGUMENTS 28
#define MAXIMUM_CHARS_IN_LINE 10

/*Functions Declarations*/
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
	bus_initiation(main_memory, bus_trace);


	for (int i = 0; i < CORE_NUM; i++) {
		caches[i] = cache_initiation(i);
	}
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

	write_RegOut(cores_array, regout); /* write the data to the regout files */

	for (int i = 0; i < CORE_NUM; i++) { /* write the data to the tsram and dsram files */
		write_Rams(tsram[i], dsram[i], cores_array[i].Cache);
	}

	write_Statistics(cores_array, stats); /* write the data to the stats files */


	// write modified data from caches to main memory
	/*for (int i = 0; i < NUM_OF_CORES; i++) {
		for (int j = 0; j < NUM_OF_BLOCKS; j++) {
			if (cores_array[i].Cache->tsram[j]->mesi_state == MODIFIED) {
				int address = (cores_array[i].Cache->tsram[j]->tag << 8) + BLOCK_SIZE * j;
				for (int k = 0; k < BLOCK_SIZE; ++k) {
					main_bus->main_memory[address + k] = cores_array[i].Cache->dsram[j][k];
				}
			}
		}
	}*/

	write_Memout(memoutf, main_memory); /* write the data to the memout file */

	close_all_files(core_trace, imem, regout, stats, tsram, dsram, meminf, memoutf, bus_trace);/* close the files we wrote to */
	/*free the allocated objects*/
	free_Cores(cores_array);
	free_bus(main_bus);
	return 0;
}

/**
 * Function that loads the instructions from the imem file to the core
 * @param commands - array that will contain the lines from imem
 * @param imem - file pointer to the relevant imem file
 * @param current_core - the core that the instructions will be loaded to
 */
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
 * Function that returns a flag that indicates whether to stop the program or not
 * @param cores - array of all cores
 * @return bool variable result - indicates whether all the cores are at a halt or not
 */
bool stopProgram(Core* cores) {
	bool result = true;
	for (int i = 0; i < CORE_NUM && result; i++) {
		result = result && (cores[i].state.haltProgram);
	}
	return result;
}

/**
 * Function that executes the pipeline cycles
 * @param cores_array - array of all cores
 * @param numOfCycle - the number of cycle to start from
 * @param trace - the trace files of the cores
 */
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
			update_core_registers(&(cores_array[i]));
			cores_array[i].state.PC = cores_array[i].current_state_Reg->if_id->New_PC;
		}
		run_bus(numOfCycle);
		for (int i = 0; i < CORE_NUM; ++i) {
			snoop(cores_array[i].Cache, i);
		}
	}
}