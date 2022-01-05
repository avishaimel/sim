#pragma once

#include "files_handler.h"
#pragma warning(disable:4996)


//Private Functions:

FILE* generalOpenFile(int coreNum, int argc, char** argv, int base_argv_index, char** filepath_options, char* mode) {
	FILE* fd = NULL;
	char* filepath = filepath_options[0];
	if (argc > 1)
		filepath = argv[base_argv_index + coreNum];
	else if (coreNum != 0) {
		filepath = filepath_options[coreNum];
	}
	fd = fopen(filepath, mode);

	if (fd == NULL) {
		printf("An error occurred while trying to open the trace file for writing");
		exit(1);
	}
	return fd;
}

FILE* open_Memin(int argc, char** argv) {
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

FILE* open_MemOut(int argc, char** argv) {
	FILE* fd = NULL;
	char* filepath = MEMOUT;
	if (argc > 1)
		filepath = argv[6];
	fd = fopen(filepath, "w");
	if (fd == NULL) {
		printf("An error occurred while trying to open the mem out file for writing");
		exit(1);
	}
	return fd;
}

FILE* open_BusTrace(int argc, char** argv) {
	FILE* fd = NULL;
	char* filepath = BUS_TRACE;
	if (argc > 1)
		filepath = argv[15];
	fd = fopen(filepath, "w");
	if (fd == NULL) {
		printf("An error occurred while trying to open the trace file for writing");
		exit(1);
	}
	return fd;
}

FILE* open_Trace(int coreNum, int argc, char** argv) {
	char *filepath_options[4] = { CORE_0_TRACE_FILE, CORE_1_TRACE_FILE, CORE_2_TRACE_FILE, CORE_3_TRACE_FILE };
	return generalOpenFile(coreNum, argc, argv, 11, filepath_options, "w");
}

FILE* open_Imem(int coreNum, int argc, char** argv) {
	char *imem_options[4] = { CORE_0_IMEM_FILE, CORE_1_IMEM_FILE, CORE_2_IMEM_FILE, CORE_3_IMEM_FILE };
	return generalOpenFile(coreNum, argc, argv, 1, imem_options, "r");
}

FILE* open_Regout(int coreNum, int argc, char** argv) {
	char *regout_options[4] = { CORE_0_REGOUT_FILE, CORE_1_REGOUT_FILE, CORE_2_REGOUT_FILE, CORE_3_REGOUT_FILE };
	return generalOpenFile(coreNum, argc, argv, 7, regout_options, "w");
}

FILE* open_Stats(int coreNum, int argc, char** argv) {
	char* stats_options[4] = { CORE_0_STATS_FILE, CORE_1_STATS_FILE, CORE_2_STATS_FILE, CORE_3_STATS_FILE };
	return generalOpenFile(coreNum, argc, argv, 24, stats_options, "w");
}

FILE* open_Tsrams(int coreNum, int argc, char** argv) {
	char* stats_options[4] = { CORE_0_TSTATS_FILE, CORE_1_TSTATS_FILE, CORE_2_TSTATS_FILE, CORE_3_TSTATS_FILE };
	return generalOpenFile(coreNum, argc, argv, 20, stats_options, "w");
}

FILE* open_Dsrams(int coreNum, int argc, char** argv) {
	char* stats_options[4] = { CORE_0_DSTATS_FILE, CORE_1_DSTATS_FILE, CORE_2_DSTATS_FILE, CORE_3_DSTATS_FILE };
	return generalOpenFile(coreNum, argc, argv, 16, stats_options, "w");
}

void write_RegOut(Core *cores, FILE **regout) {
	for (int i = 0; i < CORE_NUM; i++) {
		for (int reg_idx = 2; reg_idx < NUM_OF_REGISTERS; reg_idx++) {
			fprintf(regout[i], "%08X\n", cores[i].current_state_Reg->privateRegisters[reg_idx]);
		}
	}
}

void write_Rams(FILE* tram, FILE* dram, struct cache* cache) {
	for (int j = 0; j < NUM_OF_BLOCKS; j++) {
		for (int i = 0; i < BLOCK_SIZE; ++i) {
			fprintf(dram, "%08X\n", cache->dsram[j][i]);
		}
		fprintf(tram, "%05X", cache->tsram[j]->mesi_state);
		fprintf(tram, "%03X\n", cache->tsram[j]->tag);
	}
}

void write_Statistics(Core* cores, FILE** stats) {
	for (int i = 0; i < CORE_NUM; i++) {
		fprintf(stats[i], "cycles %d\n", cores[i].coreStatistics.cycles);
		fprintf(stats[i], "instructions %d\n", cores[i].coreStatistics.instructions);
		fprintf(stats[i], "read_hit %d\n", cores[i].Cache->read_hit);
		fprintf(stats[i], "write_hit %d\n", cores[i].Cache->write_hit);
		fprintf(stats[i], "read_miss %d\n", cores[i].Cache->read_miss);
		fprintf(stats[i], "write_miss %d\n", cores[i].Cache->write_miss);
		fprintf(stats[i], "decode_stall %d\n", cores[i].coreStatistics.decode_stall);
		fprintf(stats[i], "mem_stall %d", cores[i].Cache->mem_stall);
	}
}

void close_all_files(FILE **trace, FILE **imem, FILE **regout, FILE** stats, FILE** tsrams, FILE** dsrams, FILE* memin_file, FILE* memout_file, FILE* bus_trace) {
	for (int i = 0; i < CORE_NUM; i++) {
		fclose(trace[i]);
		fclose(imem[i]);
		fclose(regout[i]);
		fclose(stats[i]);
		fclose(tsrams[i]);
		fclose(dsrams[i]);
	}
	fclose(memin_file);
	fclose(memout_file);
	fclose(bus_trace);
}