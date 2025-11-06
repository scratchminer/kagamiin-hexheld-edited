#ifndef __CPU_REGS_H__
#define __CPU_REGS_H__

#include "types.h"

typedef struct {
	uint32_t regs[8];
	uint16_t wf;
	
	// Program counter
	uint32_t pgc;
	
	// Internal states
	uint8_t repi;
	uint8_t repr;
	
	// used for MULS / MULU / DIVS / DIVU to store the factor (A) and RM operand (B)
	uint32_t factor_a;
	uint32_t factor_b;
	
	// used for DJNZ / REPI / REPR to save the zero flag, and MULS / MULU / DIVS / DIVU to save the carry flag
	bool latch_aux;
	
	// used only for HALT to disable the clock until an interrupt happens
	bool disable_clk;
} Pilot_cpu_regs;

const enum
{
	// Extend carry/borrow flag
	F_EXTEND   = 1 << 0,
	// Decimal flag
	F_DECIMAL  = 1 << 1,
	// Overflow/parity flag
	F_OVERFLOW = 1 << 2,
	// Carry/borrow flag
	F_CARRY    = 1 << 3,
	// Zero flag
	F_ZERO     = 1 << 6,
	// Sign (negative) flag
	F_SIGN     = 1 << 7,
	// Interrupt request level
	F_IRL      = 0x7 << 8
} flag_masks;

#endif
