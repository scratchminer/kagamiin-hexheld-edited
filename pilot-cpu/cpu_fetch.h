#ifndef __CPU_FETCH_H__
#define __CPU_FETCH_H__

#include "types.h"
#include "pilot.h"

typedef struct {
	Pilot_system *sys;
	
	uint16_t queue_words[5];
	bool queue_words_full[5];
	
	uint32_t mem_addr;
	
	bool mem_access_waiting;
	
	enum
	{
		FETCH_HALF1_READY,
		FETCH_HALF1_MEM_WAIT,
		FETCH_HALF1_DEQUEUE,
		
		FETCH_HALF2_READY,
		FETCH_HALF2_BRANCH,
		FETCH_HALF2_MEM_ASSERT
	} fetch_phase;
} pilot_fetch_state;

void pilot_fetch_half1 (pilot_fetch_state *state);
void pilot_fetch_half2 (pilot_fetch_state *state);

#endif
