#ifndef __CPU_DECODE_H__
#define __CPU_DECODE_H__

#include "types.h"
#include "pilot.h"

typedef struct {
	Pilot_system *sys;
	
	inst_decoded_flags work_regs;
	inst_decoded_flags decoded_inst;
	uint32_t pgc;
	
	uint8_t inst_length;
	uint8_t words_to_read;
	enum
	{
		DECODER_HALF1_DISPATCH_WAIT = 0,
		DECODER_HALF1_READY,
		DECODER_HALF1_READ_INST_WORD,
		
		DECODER_HALF2_READ_OPERANDS,
		DECODER_HALF2_DISPATCH
	} decoding_phase;
	
	// Number of RM operands in current instruction
	uint8_t rm_ops;
} pilot_decode_state;

void decode_unreachable_ (void);

// Queues in a word read from the fetch unit
void decode_queue_read_word (pilot_decode_state *state);

void pilot_decode_half1 (pilot_decode_state *state);
void pilot_decode_half2 (pilot_decode_state *state);

#endif
