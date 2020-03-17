#include "gte.h"

#define GTE_BIND(x) std::bind(&GTE::x, this)

void GTE::register_opcodes()
{
	lookup[0x6] = GTE_BIND(op_nclip);
	lookup[0x30] = GTE_BIND(op_rtpt);
	lookup[0x13] = GTE_BIND(op_ncds);
	lookup[0x2d] = GTE_BIND(op_avsz3);
}