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
	
	// used only for DJNZ / REPI to save the zero flag
	bool temp_z;
	
	// used only for REPI / REPR to tell when to break
	bool used_z;
	
	// used only for HALT to disable the clock until an interrupt or exception
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
