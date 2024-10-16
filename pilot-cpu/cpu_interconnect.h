#ifndef __CPU_INTERCONNECT_H__
#define __CPU_INTERCONNECT_H__

#include "types.h"

typedef struct
{
	// Fetch-decode interface
	bool fetch_word_semaph;
	uint32_t fetch_addr;
	uint16_t fetch_word;
	
	// Decode-execute interface
	bool decoded_inst_semaph;
	inst_decoded_flags *decoded_inst;
	
	// Execute branch feedback
	bool execute_branch;
	uint32_t execute_branch_addr;
	
	// Goes high if the execute unit is going to access memory; tells the fetch unit to pre-emptively back off
	bool execute_memory_backoff;
} pilot_interconnect;

#endif
