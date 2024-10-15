#ifndef __CPU_DECODE_RM_H__
#define __CPU_DECODE_RM_H__

#include "types.h"
#include "cpu_decode.h"

// Decodes an RM specifier.
void decode_rm_specifier (pilot_decode_state *state, rm_spec rm, bool is_dest, bool src_is_left, data_size_spec size);

#endif
