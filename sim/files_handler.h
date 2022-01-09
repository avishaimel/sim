#include "core.h"

// Files names:

#define MEMIN "memin.txt"
#define MEMOUT "memout.txt"
#define BUS_TRACE "bustrace.txt"
#define CORE_0_TRACE_FILE "core0trace.txt"
#define CORE_1_TRACE_FILE "core1trace.txt"
#define CORE_2_TRACE_FILE "core2trace.txt"
#define CORE_3_TRACE_FILE "core3trace.txt"
#define CORE_0_IMEM_FILE "imem0.txt"
#define CORE_1_IMEM_FILE "imem1.txt"
#define CORE_2_IMEM_FILE "imem2.txt"
#define CORE_3_IMEM_FILE "imem3.txt"
#define CORE_0_REGOUT_FILE "regout0.txt"
#define CORE_1_REGOUT_FILE "regout1.txt"
#define CORE_2_REGOUT_FILE "regout2.txt"
#define CORE_3_REGOUT_FILE "regout3.txt"
#define CORE_0_STATS_FILE "stats0.txt"
#define CORE_1_STATS_FILE "stats1.txt"
#define CORE_2_STATS_FILE "stats2.txt"
#define CORE_3_STATS_FILE "stats3.txt"
#define CORE_0_TSTATS_FILE "tsram0.txt"
#define CORE_1_TSTATS_FILE "tsram1.txt"
#define CORE_2_TSTATS_FILE "tsram2.txt"
#define CORE_3_TSTATS_FILE "tsram3.txt"
#define CORE_0_DSTATS_FILE "dsram0.txt"
#define CORE_1_DSTATS_FILE "dsram1.txt"
#define CORE_2_DSTATS_FILE "dsram2.txt"
#define CORE_3_DSTATS_FILE "dsram3.txt"



// Public functions:
//Documnetation in files_handler.c
FILE* open_Memin(int argc, char** argv);
FILE* open_MemOut(int argc, char** argv);
FILE* open_BusTrace(int argc, char** argv);
FILE* open_Trace(int coreNum, int argc, char** argv);
FILE* open_Imem(int coreNum, int argc, char** argv);
FILE* open_Regout(int coreNum, int argc, char** argv);
FILE* open_Stats(int coreNum, int argc, char** argv);
FILE* open_Tsrams(int coreNum, int argc, char** argv);
FILE* open_Dsrams(int coreNum, int argc, char** argv);
void write_RegOut(Core *cores, FILE **regout);
void write_Rams(FILE* tram, FILE* dram, struct cache* cache);
void write_Statistics(Core* cores, FILE** stats);
void close_all_files(FILE **trace, FILE **imem, FILE **regout, FILE** stats, FILE** tsrams, FILE** dsrams, FILE* memin_file, FILE* memout_file, FILE* bus_trace);
