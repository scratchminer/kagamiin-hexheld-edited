#include "memory.h"
#include <stdio.h>
#include <stddef.h>

#define WRAM_END	0x007fff
#define VRAM_END	0x00ffff
#define CART_CS1_END	0x0fffff
#define CART_CS2_END	0x1fffff
#define CART_ROM_END	0xffdfff
#define TMRAM_END	0xffefff
#define OAM_END		0xfff27f
#define HCIO_START	0xfff300
#define HCIO_END	0xfff3ff
#define HRAM_START	0xfff400
#define HRAM_END	0xffffff

bool
mem_read (Pilot_system *sys)
{
	if (sys->memctl.is_16bit) printf("RD16 %06x\n", sys->memctl.addr_reg);
	else printf("RD08 %06x\n", sys->memctl.addr_reg);
	
	uint32_t addr = sys->memctl.addr_reg;
	if (addr <= WRAM_END)
	{
		// try to read WRAM
	}
	else if (addr <= VRAM_END)
	{
		// try to read VRAM
	}
	else if (addr <= CART_CS2_END)
	{
		// call cartridge chip select mapper
	}
	else if (addr <= CART_ROM_END)
	{
		// call cartridge ROM mapper
	}
	else if (addr <= TMRAM_END)
	{
		// tilemap RAM
	}
	else if (addr <= OAM_END)
	{
		// sprite attribute RAM
	}
	else if (HCIO_START <= addr && addr <= HCIO_END)
	{
		// memory mapped I/O
	}
	else if (addr <= HRAM_END)
	{
		sys->memctl.data_reg_in = sys->hram[addr - HRAM_START] | (sys->memctl.is_16bit ? (sys->hram[addr - HRAM_START + 1] << 8) : 0);
		return TRUE;
	}
	
	return FALSE;
}

bool
mem_write (Pilot_system *sys)
{
	uint32_t addr = sys->memctl.addr_reg;
	
	if (sys->memctl.is_16bit) printf("WR16 %04x -> %06x\n", sys->memctl.data_reg_out, sys->memctl.addr_reg);
	else printf("WR08 %02x -> %06x\n", sys->memctl.data_reg_out & 0xff, sys->memctl.addr_reg);
	
	if (addr <= WRAM_END)
	{
		// try to write WRAM
	}
	else if (addr <= VRAM_END)
	{
		// try to write VRAM
	}
	else if (addr <= CART_CS2_END)
	{
		// call cartridge chip select mapper
	}
	else if (addr <= CART_ROM_END)
	{
		// call cartridge ROM mapper
	}
	else if (addr <= TMRAM_END)
	{
		// tilemap RAM
	}
	else if (addr <= OAM_END)
	{
		// sprite attribute RAM
	}
	else if (HCIO_START <= addr && addr <= HCIO_END)
	{
		// memory mapped I/O
	}
	else if (addr <= HRAM_END)
	{
		sys->hram[addr - HRAM_START] = sys->memctl.data_reg_out & 0xff;
		if (sys->memctl.is_16bit) sys->hram[addr - HRAM_START + 1] = sys->memctl.data_reg_out >> 8;
		return TRUE;
	}
	
	return FALSE;
}

/*
 * Memory accesses through the memory controller need to be carried out as such:
 * 
 * Reads:
 * Tick 0: Pilot_mem_addr_read_assert - assert the address to be accessed
 * Tick 1: Pilot_mem_data_wait - wait for the access slot to be conceded
 * Tick 1+n, n >= 0: Pilot_mem_get_data - read data latch
 * 
 * Writes:
 * Tick 0: Pilot_mem_addr_write_assert - assert the address to be accessed
 * 
 */
bool
Pilot_mem_addr_read_assert (Pilot_system *sys, bool is_16bit, uint32_t addr)
{
	if (sys->memctl.state == MCTL_READY)
	{
		sys->memctl.addr_reg = addr;
		sys->memctl.is_16bit = is_16bit;
		sys->memctl.state = MCTL_MEM_R_BUSY;
		sys->memctl.data_valid = FALSE;
		
		return TRUE;
	}
	
	return FALSE;
}

bool
Pilot_mem_addr_write_assert (Pilot_system *sys, bool is_16bit, uint32_t addr, uint16_t data)
{
	if (sys->memctl.state == MCTL_READY)
	{
		sys->memctl.addr_reg = addr;
		sys->memctl.data_reg_out = data;
		sys->memctl.is_16bit = is_16bit;
		sys->memctl.state = MCTL_MEM_W_BUSY;
		sys->memctl.data_valid = FALSE;
		
		return TRUE;
	}
	
	return FALSE;
}

bool
Pilot_mem_data_wait (Pilot_system *sys)
{
	if (sys->memctl.data_valid)
	{
		return TRUE;
	}
	
	return FALSE;
}

void
Pilot_memctl_tick (Pilot_system *sys)
{
	sys->memctl.data_valid = FALSE;
	if (sys->memctl.state == MCTL_MEM_R_BUSY && mem_read(sys))
	{
		sys->memctl.state = MCTL_READY;
		sys->memctl.data_valid = TRUE;
	}
	if (sys->memctl.state == MCTL_MEM_W_BUSY && mem_write(sys))
	{
		sys->memctl.state = MCTL_READY;
		sys->memctl.data_valid = TRUE;
	}
}

uint16_t
Pilot_mem_get_data (Pilot_system *sys)
{
	return sys->memctl.data_reg_in;
}
