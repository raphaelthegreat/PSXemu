#include <stdafx.hpp>
#include "gte.h"

#define GTE_BIND(x) std::bind(&GTE::x, this)

void GTE::register_opcodes()
{
	lookup[0x1] = GTE_BIND(op_rtps);
	lookup[0x6] = GTE_BIND(op_nclip);
	lookup[0xc] = GTE_BIND(op_op);
	lookup[0x12] = GTE_BIND(op_mvmva);
	lookup[0x10] = GTE_BIND(op_dpcs);
	lookup[0x11] = GTE_BIND(op_intpl);
	lookup[0x1c] = GTE_BIND(op_cc);
	lookup[0x30] = GTE_BIND(op_rtpt);
	lookup[0x13] = GTE_BIND(op_ncds);
	lookup[0x2d] = GTE_BIND(op_avsz3);
	lookup[0x3f] = GTE_BIND(op_ncct);
	lookup[0x3d] = GTE_BIND(op_gpf);
	lookup[0x28] = GTE_BIND(op_sqr);
	lookup[0x2e] = GTE_BIND(op_avsz4);
	lookup[0x20] = GTE_BIND(op_nct);
	lookup[0x16] = GTE_BIND(op_ncdt);
	lookup[0x1b] = GTE_BIND(op_nccs);
	lookup[0x1e] = GTE_BIND(op_ncs);
}