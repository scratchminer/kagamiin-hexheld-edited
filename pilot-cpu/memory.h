#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>
#include <stddef.h>
#include "pilot.h"

void Pilot_memctl_tick (Pilot_system *sys);

bool Pilot_mem_addr_read_assert (Pilot_system *sys, bool is_16bit, uint32_t addr);
bool Pilot_mem_addr_write_assert (Pilot_system *sys, bool is_16bit, uint32_t addr, uint16_t data);
bool Pilot_mem_data_wait (Pilot_system *sys);
uint16_t Pilot_mem_get_data (Pilot_system *sys);

#endif
